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
#include <seiscomp3/math/mean.h>

#include "envelope.h"
#include "../config.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EnvelopeProcessor::EnvelopeProcessor(const Config *config, SignalUnit unit)
: BaseProcessor(config, unit) {

	// Setup filter if requested by configuration
	switch ( _unit ) {
		case Meter:
			if ( _config->vsfndr.filterDisp )
				setFilter(new Math::Filtering::IIR::ButterworthHighpass<double>(4, 1.0/3.0));
			break;
		case MeterPerSecond:
			if ( _config->vsfndr.filterVel )
				setFilter(new Math::Filtering::IIR::ButterworthHighpass<double>(4, 1.0/3.0));
			break;
		case MeterPerSecondSquared:
			if ( _config->vsfndr.filterAcc )
				setFilter(new Math::Filtering::IIR::ButterworthHighpass<double>(4, 1.0/3.0));
			break;
		default:
			setStatus(IncompatibleUnit, 1);
			break;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EnvelopeProcessor::~EnvelopeProcessor() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EnvelopeProcessor::process(const Record *rec, const DoubleArray &data) {
	if ( !_stream.initialized ) {
		SEISCOMP_INFO("%s: initializing envelope processor", rec->streamID().c_str());

		_samplePool.reset((int)(_stream.fsamp*_config->vsfndr.envelopeInterval)+1);
		_dt = Core::TimeSpan(1.0 / _stream.fsamp);

		setupTimeWindow(rec->startTime());
	}
	else if ( rec->samplingFrequency() != _stream.fsamp ) {
		SEISCOMP_INFO("%s: mismatching sampling frequency (%f != %f): reset",
		              rec->streamID().c_str(), _stream.fsamp,
		              rec->samplingFrequency());
		reset();

		_samplePool.reset((int)(_stream.fsamp*_config->vsfndr.envelopeInterval)+1);
		_dt = Core::TimeSpan(1.0 / _stream.fsamp);

		setupTimeWindow(rec->startTime());
	}

	// Record time window is after the current time window -> flush
	// existing samples and setup the new interval
	if ( rec->startTime() >= _currentEndTime ) {
		flush(rec);
		setupTimeWindow(rec->startTime());
	}

	Core::Time ts = rec->startTime();
	const BitSet *clipMask = rec->clipMask();

	// Process all samples
	if ( clipMask == NULL ) {
		for ( int i = 0; i < data.size(); ++i ) {
			if ( ts >= _currentEndTime ) {
				// Flush existing pool
				flush(rec);
				// Step to next time span
				_currentStartTime = _currentEndTime;
				_currentEndTime = _currentStartTime + _config->vsfndr.envelopeInterval;
			}

			_samplePool.push(data[i]);

			ts += _dt;
		}
	}
	else {
		for ( int i = 0; i < data.size(); ++i ) {
			if ( ts >= _currentEndTime ) {
				// Flush existing pool
				flush(rec);
				// Step to next time span
				_currentStartTime = _currentEndTime;
				_currentEndTime = _currentStartTime + _config->vsfndr.envelopeInterval;
			}

			_samplePool.push(data[i]);
			if ( clipMask->test(i) )
				_samplePool.clipped = true;

			ts += _dt;
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EnvelopeProcessor::setupTimeWindow(const Core::Time &ref) {
	if ( _config->vsfndr.envelopeInterval.seconds() > 0 ) {
		double r = floor(((double)ref / (double)_config->vsfndr.envelopeInterval));
		_currentStartTime = r * _config->vsfndr.envelopeInterval;

		// Fix for possible rounding errors
		if ( ref.microseconds() == 0 )
			_currentStartTime.setUSecs(0);
	}
	else {
		_currentStartTime = ref;
		long r = ref.microseconds() / _config->vsfndr.envelopeInterval.microseconds();
		_currentStartTime.setUSecs(r*_config->vsfndr.envelopeInterval.microseconds());
	}

	_currentEndTime = _currentStartTime + _config->vsfndr.envelopeInterval;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EnvelopeProcessor::flush(const Record *rec) {
	size_t n = _samplePool.size();
	if ( n == 0 ) return;

	double *samples = _samplePool.samples;
	double amp = fabs(samples[find_absmax((int)n, samples, 0, n, 0.0)]);

	// Publish result
	if ( _config->vsfndr.publish )
		_config->vsfndr.publish(this, amp, _currentStartTime, _samplePool.clipped);

	_samplePool.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
