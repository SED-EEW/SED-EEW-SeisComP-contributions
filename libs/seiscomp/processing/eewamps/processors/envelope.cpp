/******************************************************************************
 *     Copyright (C) by ETHZ/SED                                              *
 *                                                                            *
 *   This program is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE as published *
 *   by the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Affero General Public License for more details.                      *
 *                                                                            *
 *                                                                            *
 *   Jan Becker, gempa GmbH <jabe@gempa.de>                                   *
 ******************************************************************************/


#define SEISCOMP_COMPONENT EEWAMPS


#include <seiscomp/logging/log.h>
#include <seiscomp/math/filter/butterworth.h>
#include <seiscomp/math/mean.h>

#include "envelope.h"
#include "../config.h"

#include <seiscomp/core/exceptions.h>

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
				setFilter(new Math::Filtering::IIR::ButterworthHighpass<double>(4, _config->vsfndr.filterCornerFreq));
			break;
		case MeterPerSecond:
			if ( _config->vsfndr.filterVel )
				setFilter(new Math::Filtering::IIR::ButterworthHighpass<double>(4, _config->vsfndr.filterCornerFreq));
			break;
		case MeterPerSecondSquared:
			if ( _config->vsfndr.filterAcc )
				setFilter(new Math::Filtering::IIR::ButterworthHighpass<double>(4, _config->vsfndr.filterCornerFreq));
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
bool EnvelopeProcessor::store(const Record *rec) {
	if ( _stream.initialized
	  && rec->samplingFrequency() != _stream.fsamp ) {
		SEISCOMP_WARNING("%s: mismatching sampling frequency (%f != %f): reset",
		                 rec->streamID().c_str(), _stream.fsamp,
		                 rec->samplingFrequency());
		reset();
	}

	return BaseProcessor::store(rec);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EnvelopeProcessor::process(const Record *rec, const DoubleArray &data) {
	if ( !_stream.initialized ) {
		SEISCOMP_INFO("%s: initializing envelope processor", rec->streamID().c_str());

		_samplePool.reset((int)(rec->samplingFrequency()*_config->vsfndr.envelopeInterval+0.5)+1);
		_dt = Core::TimeSpan(1.0 / rec->samplingFrequency());

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

				
		try {
			double r = floor(((double)ref / (double)_config->vsfndr.envelopeInterval));
			_currentStartTime = r * _config->vsfndr.envelopeInterval;
		}
		catch ( const Core::OverflowException& ) { 
			std::cout << "Record start time (" << ref.iso().c_str() << ") cannot be converted to double and TimeWindow unchanged" << std::endl;
		}
		catch ( const std::exception &e ) {
			SEISCOMP_WARNING("%s", e.what());
			std::cout << "Unknown issue:" << e.what() << std::endl;

		}

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
		_config->vsfndr.publish(this, amp, _currentEndTime, _samplePool.clipped);

	_samplePool.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
