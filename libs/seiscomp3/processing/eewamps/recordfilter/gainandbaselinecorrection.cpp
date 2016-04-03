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
#include <seiscomp3/core/typedarray.h>
#include <seiscomp3/core/genericrecord.h>
#include <seiscomp3/core/exceptions.h>
#include <seiscomp3/datamodel/utils.h>

#include "gainandbaselinecorrection.h"


namespace Seiscomp {
namespace IO {

namespace {


template<typename T>
Array::DataType dispatchType() {
	throw Core::TypeConversionException("Unsupported type");
}

template<>
Array::DataType dispatchType<float>() {
	return Array::FLOAT;
}

template<>
Array::DataType dispatchType<double>() {
	return Array::DOUBLE;
}


template<typename T>
const char *dispatchTypeStr() {
	throw Core::TypeConversionException("Unsupported type");
}

template<>
const char *dispatchTypeStr<float>() {
	return "float";
}

template<>
const char *dispatchTypeStr<double>() {
	return "double";
}


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
GainAndBaselineCorrectionRecordFilter<T>::GainAndBaselineCorrectionRecordFilter(const DataModel::Inventory *inv)
: _inventory(inv)
, _gainCorrectionFactor(0)
, _saturationThreshold(-1)
, _baselineCorrectionLength(60.0)
, _taperLength(60)
{
	setBaselineCorrectionBufferLength(_baselineCorrectionLength);
	setTaperLength(_taperLength);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
GainAndBaselineCorrectionRecordFilter<T>::GainAndBaselineCorrectionRecordFilter(const GainAndBaselineCorrectionRecordFilter<T> &other)
: _inventory(other._inventory)
, _gainCorrectionFactor(0)
, _samplingFrequency(-1)
, _saturationThreshold(other._saturationThreshold)
, _baselineCorrectionLength(other._baselineCorrectionLength)
, _taperLength(other._taperLength){
	setBaselineCorrectionBufferLength(_baselineCorrectionLength);
	setTaperLength(_taperLength);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
GainAndBaselineCorrectionRecordFilter<T>::~GainAndBaselineCorrectionRecordFilter() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
void GainAndBaselineCorrectionRecordFilter<T>::setBaselineCorrectionBufferLength(double lengthInSeconds) {
#ifdef BASELINE_CORRECTION_WITH_HIGHPASS
	_baselineCorrection = BaselineRemoval(4,1.0/lengthInSeconds);
#else
	_baselineCorrection.setLength(lengthInSeconds);
#endif
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
void GainAndBaselineCorrectionRecordFilter<T>::setTaperLength(double lengthInSeconds) {
	_taper.setLength(lengthInSeconds);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<






// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
void GainAndBaselineCorrectionRecordFilter<T>::setSaturationThreshold(double threshold) {
	_saturationThreshold = threshold;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
Record *GainAndBaselineCorrectionRecordFilter<T>::feed(const Record *rec) {
	if ( !checkEpoch(rec) ) {
		if ( !queryEpoch(rec) ) return NULL;
	}

	// This comparison should be safe because we assigned that value explicitely
	if ( _gainCorrectionFactor == 0.0 )
		return NULL;

	// No data, no conversion
	const Array *sourceData = rec->data();
	if ( sourceData == NULL )
		return NULL;

	typename Core::SmartPointer< NumericArray<T> >::Impl correctedData;

	correctedData = (NumericArray<T>*)sourceData->copy(dispatchType<T>());
	if ( !correctedData ) {
		SEISCOMP_WARNING("[%s] cannot convert data to %s",
		                 rec->streamID().c_str(), dispatchTypeStr<T>());
		return NULL;
	}

	int n = correctedData->size();
	T *data = correctedData->typedData();

	BitSetPtr clipMask;

	// Check for clipped samples
	if ( _saturationThreshold > 0 ) {
		for ( int i = 0; i < n; ++i ) {
			if ( fabs(data[i]) > _saturationThreshold ) {
				if ( !clipMask )
					// The clip mask is initialized with zeros
					clipMask = new BitSet(n);
				clipMask->set(i, true);
			}
		}
	}

	if ( clipMask ) {
		std::cerr << rec->streamID() << ": set clip mask: clipped = " << clipMask->numberOfBitsSet() << std::endl;
	}

	*correctedData *= _gainCorrectionFactor;

	if ( _lastEndTime.valid() ) {
		// If the sampling frequency changed, reset the filter
		if ( _samplingFrequency != rec->samplingFrequency() ) {
			SEISCOMP_WARNING("[%s] sps change (%f != %f): reset filter",
			                 rec->streamID().c_str(), _samplingFrequency,
			                 rec->samplingFrequency());
#ifdef BASELINE_CORRECTION_WITH_TAPER
			_taper.reset();
#endif
			_baselineCorrection.reset();
			_lastEndTime = Core::Time();
		}
		else {
			Core::TimeSpan diff = rec->startTime() - _lastEndTime;
			// Overlap or gap does not matter, we need to reset the filter
			// for non-continuous records
			if ( fabs(diff) > (0.5/_samplingFrequency) ) {
				SEISCOMP_DEBUG("[%s] discontinuity of %fs: reset filter",
				               rec->streamID().c_str(), (double)diff);
				_baselineCorrection.reset();
				_lastEndTime = Core::Time();
			}
		}
	}

	if ( !_lastEndTime.valid() ) {
		_samplingFrequency = rec->samplingFrequency();
#ifdef BASELINE_CORRECTION_WITH_TAPER
		_taper.setSamplingFrequency(_samplingFrequency);
#endif
		_baselineCorrection.setSamplingFrequency(_samplingFrequency);
		_baselineCorrection.setStreamID(rec->networkCode(), rec->stationCode(), rec->locationCode(), rec->channelCode());
	}

	// Remove average
#ifdef BASELINE_CORRECTION_WITH_HIGHPASS
	_baselineCorrection.apply(n, data);
#else
	for ( int i = 0; i < n; ++i ) {
		T v = data[i];
		_baselineCorrection.apply(1, &v);
		data[i] -= v;
	}
#endif

#ifdef BASELINE_CORRECTION_WITH_TAPER
	// Apply taper
	_taper.apply(n, data);
#endif

	_lastEndTime = rec->endTime();

	GenericRecord *out = new GenericRecord(*rec);
	out->setData(correctedData.get());
	out->setClipMask(clipMask.get());

	return out;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
Record *GainAndBaselineCorrectionRecordFilter<T>::flush() {
	// Nothing to flush, each record is processed in-place
	return NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
void GainAndBaselineCorrectionRecordFilter<T>::reset() {
	_gainCorrectionFactor = 0.0;
	_currentEpoch = Core::TimeWindow();

	// Reset last end time
	_lastEndTime = Core::Time();
	_baselineCorrection.reset();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
RecordFilterInterface *GainAndBaselineCorrectionRecordFilter<T>::clone() const {
	return new GainAndBaselineCorrectionRecordFilter<T>(*this);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
bool GainAndBaselineCorrectionRecordFilter<T>::checkEpoch(const Record *rec) const {
	if ( !_currentEpoch.startTime().valid() )
		return false;

	Core::Time etime = rec->endTime();

	// Left outside
	if ( etime <= _currentEpoch.startTime() )
		return false;

	// Right outside
	if ( _currentEpoch.endTime().valid() &&
	     rec->startTime() >= _currentEpoch.endTime() )
		return false;

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename T>
bool GainAndBaselineCorrectionRecordFilter<T>::queryEpoch(const Record *rec) {
	SEISCOMP_DEBUG("[%s] Query inventory", rec->streamID().c_str());

	// No inventory, no gain correction
	if ( _inventory == NULL ) {
		SEISCOMP_ERROR("[%s] no inventory set, cannot correct data",
		               rec->streamID().c_str());
		return false;
	}

	// Query inventory for meta data of current records start time
	DataModel::Stream *model;
	model = DataModel::getStream(_inventory,
	                             rec->networkCode(), rec->stationCode(),
	                             rec->locationCode(), rec->channelCode(),
	                             rec->startTime());

	if ( model == NULL ) {
		SEISCOMP_WARNING("[%s] no metadata found for data starting at %s: discarded",
		                 rec->streamID().c_str(),
		                 rec->startTime().iso().c_str());
		return false;
	}

	// Set the current epoch
	_currentEpoch.setStartTime(model->start());
	try {
		_currentEpoch.setEndTime(model->end());
	}
	catch ( ... ) {
		_currentEpoch.setEndTime(Core::Time());
	}

	try {
		_gainCorrectionFactor = 1.0 / model->gain();
	}
	catch ( ... ) {
		SEISCOMP_WARNING("[%s] no gain set for epoch starting at %s",
		                 rec->streamID().c_str(),
		                 _currentEpoch.startTime().iso().c_str());
		_gainCorrectionFactor = 0.0;
		return false;
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Explicit template instantiation
template class SC_LIBEEWAMPS_API GainAndBaselineCorrectionRecordFilter<float>;
template class SC_LIBEEWAMPS_API GainAndBaselineCorrectionRecordFilter<double>;
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
