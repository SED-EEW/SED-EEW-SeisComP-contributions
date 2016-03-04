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
#include <seiscomp3/processing/operator/ncomps.h>
#include <seiscomp3/io/records/mseedrecord.h>
#include <seiscomp3/io/recordfilter/demux.h>
#include <seiscomp3/io/recordfilter/iirfilter.h>
#include <seiscomp3/math/filter/iirintegrate.h>
#include <seiscomp3/math/filter/chainfilter.h>

#include "filter/diffcentral.h"
#include "processors/envelope.h"
#include "processors/gba.h"
#include "processors/onsitemag.h"

#include "preprocessor.h"
#include "config.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


template <typename T>
class HPreProcessor::L2Norm {
	public:
		L2Norm(HPreProcessor *parent, PreProcessor::Component first, PreProcessor::Component second)
		: _parent(parent), _first(first), _second(second) {}

		void operator()(const Record *rec, T *data[2], int n, const Core::Time &stime, double sfreq) const {
			for ( int i = 0; i < n; ++i )
				data[0][i] = sqrt(data[0][i] * data[0][i] +
				                  data[1][i] * data[1][i]);
		}

		bool publish(int c) const { return c == 0; }

		int compIndex(const std::string &code) const {
			if ( _parent->_streamConfig[_first].code() == code )
				return 0;
			else if ( _parent->_streamConfig[_second].code() == code )
				return 1;
			return -1;
		}

		const std::string &translateChannelCode(int c, const std::string &code) {
			if ( _channelCode.empty() )
				_channelCode = code.substr(0,code.size()-1) + "X";
			return _channelCode;
		}


	private:
		HPreProcessor            *_parent;
		HPreProcessor::Component  _first;
		HPreProcessor::Component  _second;
		std::string               _channelCode;
};



namespace {


template <int N, typename PROC>
class StreamOperator : public NCompsOperator<double,N,PROC> {
	public:
		StreamOperator(const PROC &proc) : NCompsOperator<double,N,PROC>(proc) {}


	public:
		/**
		 * @brief Sets the number of records to be buffered for each component
		 * @param n Number of records
		 */
		void setRingBufferSize(int n) {
			for ( int i = 0; i < N; ++i )
				this->_states[i].buffer = RingBuffer(n);
		}

		/**
		 * @brief Sets the number of seconds to buffer for each component
		 * @param ts time span
		 */
		void setRingBufferSize(const Core::TimeSpan &ts) {
			for ( int i = 0; i < N; ++i )
				this->_states[i].buffer = RingBuffer(ts);
		}

		/**
		 * @brief Returns the current largest delay of one component with
		 *        respect to all other components.
		 * @return The delay as time span
		 */
		const Core::TimeSpan currentDelay() const { return _currentDelay; }


	protected:
		WaveformProcessor::Status feed(const Record *rec) {
			if ( rec->data() == NULL ) return WaveformProcessor::WaitingForData;

			int i = this->_proc.compIndex(rec->channelCode());
			if ( i >= 0 ) {
				this->_states[i].buffer.feed(rec);
				return process(i, rec);
			}

			return WaveformProcessor::WaitingForData;
		}


		WaveformProcessor::Status process(int comp, const Record *rec) {
			WaveformProcessor::Status stat = NCompsOperator<double,N,PROC>::process(comp, rec);

			_currentDelay = Core::TimeSpan();

			bool hasCommonEndTime = true;
			for ( int i = 0; i < N; ++i ) {
				if ( !this->_states[i].endTime.valid() ) {
					hasCommonEndTime = false;
					break;
				}
			}

			if ( hasCommonEndTime ) {
				for ( int i = 0; i < N; ++i ) {
					if ( this->_states[i].buffer.empty() ) continue;
					Core::TimeSpan delay = this->_states[i].buffer.back()->endTime() - this->_states[i].endTime;
					if ( delay > _currentDelay )
						_currentDelay = delay;
				}
			}
			else {
				for ( int i = 0; i < N; ++i ) {
					if ( this->_states[i].buffer.empty() ) continue;
					Core::TimeSpan delay = this->_states[i].buffer.back()->endTime() - this->_states[i].buffer.front()->startTime();
					if ( delay > _currentDelay )
						_currentDelay = delay;
				}
			}

			return stat;
		}


	private:
		Core::TimeSpan _currentDelay;
};


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RoutingProcessor::RoutingProcessor(const Config *config, SignalUnit unit)
: _config(config), _unit(unit) {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RoutingProcessor::~RoutingProcessor() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool RoutingProcessor::compile(const DataModel::WaveformStreamID &id) {
	if ( _config->vsfndr.enable )
		_impls.push_back(new EnvelopeProcessor(_config, _unit));

	if ( _config->gba.enable )
		_impls.push_back(new GbAProcessor(_config, _unit));

	if ( _config->omp.enable )
		_impls.push_back(new OnsiteMagnitudeProcessor(_config, _unit));

	// Sanity check
	bool allGood = true;
	Implementations::iterator it;
	for ( it = _impls.begin(); it != _impls.end(); ) {
		BaseProcessor *proc = it->get();
		if ( proc->isFinished() ) {
			SEISCOMP_WARNING("Remove proc on %s.%s.%s.%s with unit %s: %s (%f)",
			                 id.networkCode().c_str(),
			                 id.stationCode().c_str(),
			                 id.locationCode().c_str(),
			                 id.channelCode().c_str(),
			                 proc->signalUnit().toString(),
			                 proc->status().toString(),
			                 proc->statusValue());
			it = _impls.erase(it);
			allGood = false;
		}
		else {
			// Setup processor
			++it;
			proc->setUsedComponent(usedComponent());
			proc->setWaveformID(id);
		}
	}

	if ( _impls.empty() ) {
		SEISCOMP_WARNING("Terminate processor");
		setStatus(Terminated,0);
	}

	return allGood;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool RoutingProcessor::handle(const DataModel::Pick *pick) {
	bool handled = false;

	// Forward it to the implementations
	Implementations::iterator it;
	for ( it = _impls.begin(); it != _impls.end(); ++it )
		if ( (*it)->handle(pick) )
			handled = true;

	return handled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RoutingProcessor::process(const Record *rec, const DoubleArray &) {
	if ( _config->dumpRecords ) {
		// Debug: write miniseed to stdout
		IO::MSeedRecord mseed(*rec);
		mseed.write(std::cout);
	}

	// Forward it to the implementations
	Implementations::iterator it;
	for ( it = _impls.begin(); it != _impls.end(); ++it )
		(*it)->feed(rec);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
HRoutingProcessor::HRoutingProcessor(const Config *config, SignalUnit unit)
: RoutingProcessor(config, unit) {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool HRoutingProcessor::feed(const Record *rec) {
	if ( _config->dumpRecords ) {
		// Debug: write miniseed to stdout
		IO::MSeedRecord mseed(*rec);
		mseed.write(std::cout);
	}

	return RoutingProcessor::feed(rec);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PreProcessor::PreProcessor(const Config *config)
: RoutingProcessor(config, SignalUnit::Quantity) {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PreProcessor::~PreProcessor() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PreProcessor::compile(const DataModel::WaveformStreamID &id) {
	const std::string *gainUnit = NULL;

	_coLocatedFilter = NULL;
	_displacementFilter = NULL;

	_coLocatedProc = NULL;
	_displacementProc = NULL;

	// Initialize filters and basic settings
	switch ( usedComponent() ) {
		case Vertical:
			gainUnit = &streamConfig(VerticalComponent).gainUnit;
			break;
		case FirstHorizontal:
			gainUnit = &streamConfig(FirstHorizontalComponent).gainUnit;
			break;
		case SecondHorizontal:
			gainUnit = &streamConfig(SecondHorizontalComponent).gainUnit;
			break;
		default:
			setStatus(Error, -1);
			break;
	}

	if ( !_unit.fromString(*gainUnit) ) {
		SEISCOMP_ERROR("Invalid unit: %s", gainUnit->c_str());
		setStatus(IncompatibleUnit, 0);
	}
	else {
		switch ( _unit ) {
			case MeterPerSecond:
				_coLocatedLocationCode = "PA";
				if ( _config->wantSignal[MeterPerSecondSquared] ) {
					if ( usedComponent() == Vertical )
						_coLocatedFilter = new IO::RecordIIRFilter<double>(new Math::Filtering::DiffCentral<double>());
					else
						_coLocatedFilter = new IO::RecordDemuxFilter(new IO::RecordIIRFilter<double>(new Math::Filtering::DiffCentral<double>()));
				}
				break;
			case MeterPerSecondSquared:
				_coLocatedLocationCode = "PV";
				if ( _config->wantSignal[MeterPerSecond] || _config->wantSignal[Meter] ) {
					Math::Filtering::ChainFilter<double> *filter;
					filter = new Math::Filtering::ChainFilter<double>;
					filter->add(new Math::Filtering::IIR::ButterworthHighpass<double>(4,0.075));
					filter->add(new Math::Filtering::IIRIntegrate<double>());
					if ( usedComponent() == Vertical )
						_coLocatedFilter = new IO::RecordIIRFilter<double>(filter);
					else
						_coLocatedFilter = new IO::RecordDemuxFilter(new IO::RecordIIRFilter<double>(filter));
				}
				break;
			default:
				SEISCOMP_ERROR("Unsupported unit: %s", _unit.toString());
				setStatus(IncompatibleUnit, 1);
				break;
		}

		if ( _config->wantSignal[Meter] ) {
			Math::Filtering::ChainFilter<double> *filter;
			filter = new Math::Filtering::ChainFilter<double>;
			filter->add(new Math::Filtering::IIR::ButterworthHighpass<double>(4,0.075));
			filter->add(new Math::Filtering::IIRIntegrate<double>());
			if ( usedComponent() == Vertical ) {
				// Integrate to displacement in any case
				_displacementFilter = new IO::RecordIIRFilter<double>(filter);
			}
			else {
				// Integrate to displacement in any case
				_displacementFilter = new IO::RecordDemuxFilter(new IO::RecordIIRFilter<double>(filter));
			}
		}
	}

	RoutingProcessor::compile(id);

	return !isFinished();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PreProcessor::handle(const DataModel::Pick *pick) {
	bool handled = false;

	if ( RoutingProcessor::handle(pick) )
		handled = true;

	if ( _coLocatedProc && _coLocatedProc->handle(pick) )
		handled = true;

	if ( _displacementProc && _displacementProc->handle(pick) )
		handled = true;

	return handled;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PreProcessor::reset() {
	WaveformProcessor::reset();

	if ( _coLocatedFilter )
		_coLocatedFilter = _coLocatedFilter->clone();
	if ( _displacementFilter )
		_displacementFilter = _displacementFilter->clone();

	if ( _coLocatedProc )
		_coLocatedProc->reset();
	if ( _displacementProc )
		_displacementProc->reset();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PreProcessor::feed(const Record *rec) {
	// Feed this
	bool res = false;

	if ( _config->wantSignal[_unit] ) {
		if ( !RoutingProcessor::feed(rec) )
			return false;
		res = true;
	}

	RecordPtr coLocatedRec, displacementRec;

	if ( _coLocatedFilter ) {
		coLocatedRec = _coLocatedFilter->feed(rec);
		coLocatedRec->setLocationCode(_coLocatedLocationCode);
	}

	if ( _coLocatedProc ) {
		if ( _coLocatedProc->feed(coLocatedRec.get()) )
			res = true;
	}

	if ( _displacementFilter ) {
		// Route to displacement filter
		switch ( _unit ) {
			case MeterPerSecond:
				displacementRec = _displacementFilter->feed(rec);
				break;
			case MeterPerSecondSquared:
				if ( coLocatedRec )
					displacementRec = _displacementFilter->feed(coLocatedRec.get());
				break;
			default:
				break;
		}
	}

	if ( _displacementProc && displacementRec ) {
		displacementRec->setLocationCode("PD");
		if ( _displacementProc->feed(displacementRec.get()) )
			res = true;
	}

	return res;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PreProcessor::handleGap(Filter *, const Core::TimeSpan &ts, double, double,
                          size_t missingSamples) {
	SEISCOMP_WARNING("%s: detected gap of %.6f secs or %lu samples: reset processing",
	                 _stream.lastRecord->streamID().c_str(), (double)ts,
	                 (unsigned long)missingSamples);
	// Reset the processor
	reset();
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
VPreProcessor::VPreProcessor(const Config *config) : PreProcessor(config) {
	setUsedComponent(Vertical);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
VPreProcessor::~VPreProcessor() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool VPreProcessor::compile(const DataModel::WaveformStreamID &id) {
	if ( !PreProcessor::compile(id) )
		return false;

	if ( _coLocatedFilter ) {
		if ( _unit == MeterPerSecond ) {
			if ( _config->wantSignal[WaveformProcessor::MeterPerSecondSquared] )
				_coLocatedProc = new RoutingProcessor(_config, MeterPerSecondSquared);
		}
		else {
			if ( _config->wantSignal[WaveformProcessor::MeterPerSecond] )
				_coLocatedProc = new RoutingProcessor(_config, MeterPerSecond);
		}

		if ( _coLocatedProc ) {
			_coLocatedProc->setUsedComponent(Vertical);
			_coLocatedProc->compile(id);
		}
	}

	if ( _displacementFilter && _config->wantSignal[WaveformProcessor::Meter] ) {
		_displacementProc = new RoutingProcessor(_config, Meter);
		_displacementProc->setUsedComponent(Vertical);
		_displacementProc->compile(id);
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
HPreProcessor::HPreProcessor(const Config *config) : PreProcessor(config) {
	typedef HPreProcessor::L2Norm<double> OpWrapper;
	typedef StreamOperator<2,OpWrapper> L2NormOperator;

	// We use the configuration (e.g. gainUnit) from the first horizontal
	// since both horizontals are combined into a single component
	setUsedComponent(FirstHorizontal);

	Core::SmartPointer<L2NormOperator>::Impl op = new L2NormOperator(OpWrapper(this, FirstHorizontalComponent, SecondHorizontalComponent));
	op->setRingBufferSize(_config->horizontalBufferSize);
	_l2norm = op;
	setOperator(_l2norm.get());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
HPreProcessor::~HPreProcessor() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool HPreProcessor::feed(const Record *rec) {
	if ( _config->dumpRecords && _config->wantSignal[_unit] ) {
		// Debug: write miniseed to stdout
		IO::MSeedRecord mseed(*rec);
		mseed.write(std::cout);
	}

	typedef HPreProcessor::L2Norm<double> OpWrapper;
	typedef StreamOperator<2,OpWrapper> L2NormOperator;

	bool res = PreProcessor::feed(rec);

	if ( static_cast<L2NormOperator*>(_l2norm.get())->currentDelay() > _config->horizontalMaxDelay )
		SEISCOMP_WARNING("%s: horizontal gap too high: %fs",
		                 rec->streamID().c_str(),
		                 (double)static_cast<L2NormOperator*>(_l2norm.get())->currentDelay());

	return res;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool HPreProcessor::compile(const DataModel::WaveformStreamID &id) {
	if ( !PreProcessor::compile(id) )
		return false;

	typedef HPreProcessor::L2Norm<double> OpWrapper;
	typedef StreamOperator<2,OpWrapper> L2NormOperator;
	Core::SmartPointer<L2NormOperator>::Impl op;

	if ( _coLocatedFilter ) {
		if ( _unit == MeterPerSecond ) {
			if ( _config->wantSignal[WaveformProcessor::MeterPerSecondSquared] )
				_coLocatedProc = new HRoutingProcessor(_config, MeterPerSecondSquared);
		}
		else {
			if ( _config->wantSignal[WaveformProcessor::MeterPerSecond] )
				_coLocatedProc = new HRoutingProcessor(_config, MeterPerSecond);
		}

		if ( _coLocatedProc ) {
			_coLocatedProc->setUsedComponent(FirstHorizontal);
			_coLocatedProc->compile(id);

			op = new L2NormOperator(OpWrapper(this, FirstHorizontalComponent, SecondHorizontalComponent));
			op->setRingBufferSize(_config->horizontalBufferSize);

			_coLocatedProc->setOperator(op.get());
		}
	}

	if ( _displacementFilter && _config->wantSignal[WaveformProcessor::Meter] ) {
		_displacementProc = new HRoutingProcessor(_config, Meter);
		_displacementProc->setUsedComponent(FirstHorizontal);
		_displacementProc->compile(id);

		op = new L2NormOperator(OpWrapper(this, FirstHorizontalComponent, SecondHorizontalComponent));
		op->setRingBufferSize(_config->horizontalBufferSize);

		_displacementProc->setOperator(op.get());
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
