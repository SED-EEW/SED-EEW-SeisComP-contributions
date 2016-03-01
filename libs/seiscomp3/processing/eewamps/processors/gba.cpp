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
#include <seiscomp3/math/filter/butterworth.h>
#include <seiscomp3/datamodel/pick.h>

#include "gba.h"
#include "../config.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GbAProcessor::GbAProcessor(const Config *config, SignalUnit unit)
: BaseProcessor(config, unit), _filterBank(NULL), _amplitudeBuffer(NULL) {

	// Setup filter if requested by configuration
	switch ( _unit ) {
		case MeterPerSecond:
			break;
		default:
			setStatus(IncompatibleUnit, unit);
			return;
	}

	setFilter(new Math::Filtering::IIR::ButterworthHighpass<double>(4,0.075));

	_filterBank = new FilterPtr[config->gba.passbands.size()];
	_amplitudeBuffer = new RingBuffer(_config->gba.bufferSize);

}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GbAProcessor::~GbAProcessor() {
	if ( _filterBank )
		delete [] _filterBank;

	if ( _amplitudeBuffer )
		delete _amplitudeBuffer;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool GbAProcessor::store(const Record *rec) {
	/*
	if ( rec->samplingFrequency() < 100.0 ) {
		SEISCOMP_ERROR("%s: sampling rate %f sps too low, at least 100 sps are required",
		               rec->streamID().c_str(), rec->samplingFrequency());
		return false;
	}
	*/

	return BaseProcessor::store(rec);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GbAProcessor::reset() {
	BaseProcessor::reset();

	// Delete all filters, they are generated again in initFilter(...).
	for ( size_t i = 0; i < _config->gba.passbands.size(); ++i )
		_filterBank[i] = NULL;

	if ( _amplitudeBuffer )
		_amplitudeBuffer->clear();

	_triggerBuffer.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool GbAProcessor::handle(const DataModel::Pick *pick) {
	try {
		if ( pick->phaseHint().code() != "P" )
			return false;
	}
	catch ( ... ) {}

	Core::Time now = Core::Time::GMT();
	Core::TimeSpan diff = now - pick->time().value();

	if ( diff >= _config->gba.cutOffTime ) {
		SEISCOMP_WARNING("%s: pick '%s' arrived too late: %fs",
		                 _strWaveformID.c_str(),
		                 pick->publicID().c_str(), (double)diff);
		return false;
	}

	TriggerPtr trigger = new Trigger(_config->gba.passbands.size(), pick->publicID(), pick->time().value());
	updateAndPublishTriggerAmplitudes(*trigger);

	_triggerBuffer.push_back(trigger);
	std::sort(_triggerBuffer.begin(), _triggerBuffer.end());

	trimTriggerBuffer(now);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GbAProcessor::process(const Record *rec, const DoubleArray &data) {
	Core::Time now = Core::Time::GMT();

	if ( !_stream.initialized ) {
		SEISCOMP_INFO("%s: initializing GbA processor", rec->streamID().c_str());

		// Gap tolerance is half a sample
		setGapTolerance(0.5 / _stream.fsamp);
		SEISCOMP_DEBUG("  fsamp = %fsps", _stream.fsamp);
		SEISCOMP_DEBUG("  gap tolerance = %fs", (double)gapTolerance());

		double loFreq = -1, hiFreq = -1;
		for ( size_t i = 0; i < _config->gba.passbands.size(); ++i ) {
			if ( (loFreq < 0) || (_config->gba.passbands[i].first < loFreq) )
				loFreq = _config->gba.passbands[i].first;
			if ( (hiFreq < 0) || (_config->gba.passbands[i].second > hiFreq) )
				hiFreq = _config->gba.passbands[i].second;

			_filterBank[i] = new Math::Filtering::IIR::ButterworthHighLowpass<double>(4, _config->gba.passbands[i].first, _config->gba.passbands[i].second);
			_filterBank[i]->setSamplingFrequency(_stream.fsamp);
		}

		SEISCOMP_DEBUG("  filter bank range %f-%fHz", loFreq, hiFreq);
	}

	Core::SmartPointer<GbARecord>::Impl gbaRec = new GbARecord(_config->gba.passbands.size(), *rec);
	gbaRec->setData(new DoubleArray(data));

	for ( size_t i = 0; i < _config->gba.passbands.size(); ++i ) {
		gbaRec->filteredData[i] = new DoubleArray(data);
		_filterBank[i]->apply(gbaRec->filteredData[i]->size(),
		                      gbaRec->filteredData[i]->typedData());
	}

	// Copy clip mask if available
	if ( rec->clipMask() != NULL ) {
		BitSetPtr clipMask = new BitSet(*rec->clipMask());
		gbaRec->setClipMask(clipMask.get());
	}

	_amplitudeBuffer->feed(gbaRec.get());

	updateAndPublishTriggerAmplitudes();
	trimTriggerBuffer(now);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GbAProcessor::trimTriggerBuffer(const Core::Time &referenceTime) {
	// Trim trigger buffer
	while ( !_triggerBuffer.empty() ) {
		if ( referenceTime-_triggerBuffer.front()->time > _config->gba.cutOffTime )
			_triggerBuffer.pop_front();
		else
			return;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GbAProcessor::updateAndPublishTriggerAmplitudes(Trigger &trigger) {
	RingBuffer::iterator it;

	trigger.clipped = false;

	Core::Time maxEvaluationTime;

	for ( it = _amplitudeBuffer->begin(); it != _amplitudeBuffer->end(); ++it ) {
		const GbARecord *rec = static_cast<const GbARecord*>(it->get());
		if ( rec->endTime() <= trigger.time ) continue;
		int startSample = (trigger.time-rec->startTime())*rec->samplingFrequency();
		if ( startSample < 0 ) startSample = 0;
		if ( startSample >= rec->sampleCount() ) continue;

		int endSample = (trigger.time+_config->gba.cutOffTime-rec->startTime())*rec->samplingFrequency()+1;
		if ( endSample > rec->sampleCount() ) endSample = rec->sampleCount();

		maxEvaluationTime = rec->startTime() + Core::TimeSpan(endSample/rec->samplingFrequency());

		// If any sample is clipped, mark the amplitudes as clipped as well
		const BitSet *clipMask = rec->clipMask();
		if ( (clipMask != NULL) && clipMask->any() ) {
			for ( int i = startSample; i < endSample; ++i ) {
				if ( clipMask->test(i) ) {
					trigger.clipped = true;
					break;
				}
			}
		}

		// Update peak amplitude for all filter bands
		for ( size_t f = 0; f < _config->gba.passbands.size(); ++f ) {
			const double *filtered = rec->filteredData[f]->typedData();

			for ( int i = startSample; i < endSample; ++i ) {
				double peak = fabs(filtered[i]);
				if ( peak > trigger.amplitudes[f] ) {
					trigger.amplitudes[f] = peak;
					trigger.maxTime = rec->startTime() + Core::TimeSpan(i/rec->samplingFrequency());
				}
			}
		}
	}

	// Publish it
	if ( _config->gba.publish )
		_config->gba.publish(this, trigger.publicID, trigger.amplitudes,
		                     trigger.maxTime, trigger.time, maxEvaluationTime,
		                     trigger.clipped);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GbAProcessor::updateAndPublishTriggerAmplitudes() {
	TriggerBuffer::iterator it;
	for ( it = _triggerBuffer.begin(); it != _triggerBuffer.end(); ++it )
		updateAndPublishTriggerAmplitudes(**it);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
