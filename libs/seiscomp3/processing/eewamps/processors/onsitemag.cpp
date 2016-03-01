/***************************************************************************
 *   Copyright (C) by ETHZ/SED                                             *
 *                                                                         *
 *   You can redistribute and/or modify this program under the             *
 *   terms of the SeisComP Public License.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   SeisComP Public License for more details.                             *
 *                                                                         *
 *   Jan Becker, gempa GmbH <jabe@gempa.de>                                *
 ***************************************************************************/


#define SEISCOMP_COMPONENT EEWAMPS


#include <seiscomp3/logging/log.h>
#include <seiscomp3/datamodel/pick.h>
#include <seiscomp3/io/records/mseedrecord.h>

#include "onsitemag.h"
#include "../config.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
OnsiteMagnitudeProcessor::OnsiteMagnitudeProcessor(const Config *config, SignalUnit unit)
: BaseProcessor(config, unit)
, _tauPBuffer(config->omp.cutOffTime)
, _tauCBuffer(config->omp.cutOffTime) {
	// Setup filter if requested by configuration
	switch ( _unit ) {
		case MeterPerSecond:
			break;
		default:
			setStatus(IncompatibleUnit, unit);
			return;
	}

	setFilter(new Math::Filtering::IIR::ButterworthHighpass<double>(4,0.075));
	_lowPassFilter = Math::Filtering::IIR::ButterworthLowpass<double>(4,3);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
OnsiteMagnitudeProcessor::~OnsiteMagnitudeProcessor() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void OnsiteMagnitudeProcessor::reset() {
	BaseProcessor::reset();
	_lowPassFilter = Math::Filtering::IIR::ButterworthLowpass<double>(4,3);
	_tauPFilter.reset();
	_displacementFilter.reset();
	_tauPBuffer.clear();
	_tauCBuffer.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void OnsiteMagnitudeProcessor::initFilter(double fsamp) {
	BaseProcessor::initFilter(fsamp);
	_lowPassFilter.setSamplingFrequency(fsamp);
	_tauPFilter.setSamplingFrequency(fsamp);
	_displacementFilter.setSamplingFrequency(fsamp);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool OnsiteMagnitudeProcessor::handle(const DataModel::Pick *pick) {
	try {
		if ( pick->phaseHint().code() != "P" )
			return false;
	}
	catch ( ... ) {}

	Core::Time now = Core::Time::GMT();
	Core::TimeSpan diff = now - pick->time().value();

	if ( diff >= _config->omp.cutOffTime ) {
		// TODO: Issue severe log
		return false;
	}

	Trigger trigger(pick->publicID(), pick->time().value());
	updateAndPublishTriggerAmplitudes(trigger);

	_triggerBuffer.push_back(trigger);
	std::sort(_triggerBuffer.begin(), _triggerBuffer.end());

	trimTriggerBuffer(now);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void OnsiteMagnitudeProcessor::process(const Record *rec, const DoubleArray &data) {
	Core::Time now = Core::Time::GMT();

	if ( !_stream.initialized ) {
		SEISCOMP_INFO("%s: initializing OMP processor", rec->streamID().c_str());

		// Gap tolerance is half a sample
		setGapTolerance(0.5 / _stream.fsamp);
		SEISCOMP_DEBUG("  fsamp = %fsps", _stream.fsamp);
		SEISCOMP_DEBUG("  gap tolerance = %fs", (double)gapTolerance());
	}

	DoubleArrayPtr copyData = new DoubleArray(data);
	_lowPassFilter.apply(copyData->size(), copyData->typedData());
	_tauPFilter.apply(copyData->size(), copyData->typedData());

	GenericRecordPtr genRec = new GenericRecord(*rec);
	genRec->setData(copyData.get());

	// Copy clip mask if available
	if ( rec->clipMask() != NULL ) {
		BitSetPtr clipMask = new BitSet(*rec->clipMask());
		genRec->setClipMask(clipMask.get());
	}

	genRec->setLocationCode("TP");
	if ( _config->dumpRecords ) {
		// Debug: write miniseed to stdout
		IO::MSeedRecord mseed(*genRec);
		mseed.write(std::cout);
	}

	_tauPBuffer.feed(genRec.get());

	// Save tauC record
	copyData = new DoubleArray(data);

	Core::SmartPointer<TauCRecord>::Impl tauCRecord = new TauCRecord(*rec);
	tauCRecord->setData(copyData.get());
	tauCRecord->displacement.setData(data.size(), data.typedData());
	_displacementFilter.apply(tauCRecord->displacement.size(), tauCRecord->displacement.typedData());

	// Copy clip mask if available
	if ( rec->clipMask() != NULL ) {
		BitSetPtr clipMask = new BitSet(*rec->clipMask());
		tauCRecord->setClipMask(clipMask.get());
	}

	tauCRecord->setLocationCode("TC");
	if ( _config->dumpRecords ) {
		// Debug: write miniseed to stdout
		IO::MSeedRecord mseed(*genRec);
		mseed.write(std::cout);
	}

	_tauCBuffer.feed(tauCRecord.get());

	updateAndPublishTriggerAmplitudes();
	trimTriggerBuffer(now);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void OnsiteMagnitudeProcessor::trimTriggerBuffer(const Core::Time &referenceTime) {
	// Trim trigger buffer
	while ( !_triggerBuffer.empty() ) {
		if ( referenceTime-_triggerBuffer.front().time > _config->omp.cutOffTime )
			_triggerBuffer.pop_front();
		else
			return;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void OnsiteMagnitudeProcessor::updateAndPublishTriggerAmplitudes(Trigger &trigger) {
	bool updated = false;
	bool clipped = false;
	RingBuffer::iterator it;

	Core::Time startTime = trigger.time + _config->omp.tauPDeadTime;
	Core::Time endTime = trigger.time + _config->omp.cutOffTime;
	Core::Time maxEvaluationTime;

	// Update tauP value
	for ( it = _tauPBuffer.begin(); it != _tauPBuffer.end(); ++it ) {
		const Record *rec = it->get();
		if ( rec->endTime() <= startTime ) continue;
		int startSample = (startTime-rec->startTime())*rec->samplingFrequency();
		if ( startSample < 0 ) startSample = 0;
		if ( startSample >= rec->sampleCount() ) continue;

		int endSample = (endTime-rec->startTime())*rec->samplingFrequency()+1;
		if ( endSample > rec->sampleCount() ) endSample = rec->sampleCount();

		maxEvaluationTime = rec->startTime() + Core::TimeSpan(endSample/rec->samplingFrequency());

		// If any sample is clipped, mark the amplitudes as clipped as well
		const BitSet *clipMask = rec->clipMask();
		if ( (clipMask != NULL) && clipMask->any() ) {
			for ( int i = startSample; i < endSample; ++i ) {
				if ( clipMask->test(i) ) {
					clipped = true;
					break;
				}
			}
		}

		const double *data = (const double*)rec->data()->data();

		for ( int i = startSample; i < endSample; ++i ) {
			double peak = data[i];
			if ( peak > trigger.tauPMax ) {
				updated = true;
				trigger.tauPTime = rec->startTime() + Core::TimeSpan(i/rec->samplingFrequency());
				trigger.tauPMax = peak;
			}
		}
	}

	// If something has updated, publish it
	if ( updated && _config->omp.publishTauP )
		_config->omp.publishTauP(this, trigger.publicID, trigger.tauPTime,
		                         startTime, maxEvaluationTime,
		                         trigger.tauPMax, clipped);

	// Check for tauC window
	if ( trigger.gotTauC || _tauCBuffer.empty() )
		return;

	endTime = trigger.time + _config->omp.cutOffTime;
	if ( _tauCBuffer.back()->endTime() < endTime )
		// Window no yet complete
		return;

	Core::Time lastEndTime;
	double integralVelocity = 0;
	double integralDisplacement = 0;

	double dt = 1.0 / _stream.fsamp;
	double fac = dt*0.5;

	double last_v2 = -1;
	double last_d2 = -1;

	double Pd = -1;

	clipped = false;

	for ( it = _tauCBuffer.begin(); it != _tauCBuffer.end(); ++it ) {
		const TauCRecord *rec = static_cast<const TauCRecord*>(it->get());
		if ( rec->endTime() <= trigger.time ) continue;

		int startSample;

		if ( lastEndTime.valid() ) {
			Core::TimeSpan diff = rec->startTime()-lastEndTime;
			if ( diff >= gapTolerance() ) {
				SEISCOMP_ERROR("%s: gap detected, abort tauC computation", rec->streamID().c_str());
				break;
			}

			startSample = (lastEndTime-rec->startTime())*rec->samplingFrequency();
		}
		else
			startSample = (trigger.time-rec->startTime())*rec->samplingFrequency();

		if ( startSample < 0 ) startSample = 0;
		if ( startSample >= rec->sampleCount() ) continue;

		int endSample = (endTime-rec->startTime())*rec->samplingFrequency()+1;
		if ( endSample > rec->sampleCount() ) endSample = rec->sampleCount();
		else {
			double r = integralVelocity / integralDisplacement;
			double tauC = 2*M_PI / sqrt(r);
			if ( _config->omp.publishTauCPd ) {
				_config->omp.publishTauCPd(this, trigger.publicID, trigger.time,
				                           endTime, tauC, Pd, clipped);
			}
		}

		lastEndTime = rec->endTime();

		// If any sample is clipped, mark the amplitudes as clipped as well
		const BitSet *clipMask = rec->clipMask();
		if ( (clipMask != NULL) && clipMask->any() ) {
			for ( int i = startSample; i < endSample; ++i ) {
				if ( clipMask->test(i) ) {
					clipped = true;
					break;
				}
			}
		}

		const double *data = (const double*)rec->data()->data();
		const double *disp = rec->displacement.typedData();

		if ( last_v2 < 0 ) {
			last_v2 = data[startSample]*data[startSample];
			last_d2 = disp[startSample]*disp[startSample];
			++startSample;

			Pd = fabs(disp[startSample]);
		}

		for ( int i = startSample; i < endSample; ++i ) {
			double v2 = data[i]*data[i];
			double d2 = disp[i]*disp[i];

			integralVelocity += (v2-last_v2)*fac;
			integralDisplacement += (d2-last_d2)*fac;

			last_v2 = v2;
			last_d2 = d2;

			if ( Pd < fabs(disp[i]) )
				Pd = fabs(disp[i]);
		}
	}

	// Flag computation for tauC as done
	trigger.gotTauC = true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void OnsiteMagnitudeProcessor::updateAndPublishTriggerAmplitudes() {
	TriggerBuffer::iterator it;
	for ( it = _triggerBuffer.begin(); it != _triggerBuffer.end(); ++it )
		updateAndPublishTriggerAmplitudes(*it);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
