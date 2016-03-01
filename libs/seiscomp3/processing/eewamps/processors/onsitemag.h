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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_PROCESSORS_ONSITEMAG_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_PROCESSORS_ONSITEMAG_H__


#include <seiscomp3/core/recordsequence.h>
#include <seiscomp3/math/filter/butterworth.h>
#include <seiscomp3/math/filter/iirintegrate.h>
#include "../baseprocessor.h"
#include "../filter/taup.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


/**
 * @brief The OnsiteMagnitudeProcessor class implements basic onsite magnitude
 *        processing by calculating TauP, TauC and Pd.
 */
class SC_LIBEEWAMPS_API OnsiteMagnitudeProcessor : public BaseProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		explicit OnsiteMagnitudeProcessor(const Config *config, SignalUnit unit);

		//! D'tor
		~OnsiteMagnitudeProcessor();


	// ----------------------------------------------------------------------
	//  Processor interface
	// ----------------------------------------------------------------------
	public:
		virtual void reset();
		virtual void initFilter(double fsamp);
		virtual bool handle(const DataModel::Pick *pick);


	protected:
		virtual void process(const Record *rec, const DoubleArray &filteredData);


	// ----------------------------------------------------------------------
	//  Private methods
	// ----------------------------------------------------------------------
	private:
		struct Trigger {
			Trigger() {}
			Trigger(const std::string &pid, const Core::Time &t)
			: publicID(pid), time(t), tauPMax(-1), gotTauC(false) {}

			std::string publicID;
			Core::Time  time;
			double      tauPMax;
			Core::Time  tauPTime;
			bool        gotTauC;

			bool operator<(const Trigger &other) const {
				return time < other.time;
			}
		};

		void trimTriggerBuffer(const Core::Time &referenceTime);
		void updateAndPublishTriggerAmplitudes(Trigger &trigger);
		void updateAndPublishTriggerAmplitudes();


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		typedef Core::SmartPointer<Filter>::Impl FilterPtr;
		typedef std::deque<Trigger> TriggerBuffer;

		TriggerBuffer                                    _triggerBuffer;
		RingBuffer                                       _tauPBuffer;
		RingBuffer                                       _tauCBuffer;
		Math::Filtering::IIR::ButterworthLowpass<double> _lowPassFilter;
		Math::Filtering::TauP<double>                    _tauPFilter;
		Math::Filtering::IIRIntegrate<double>            _displacementFilter;
};


}
}
}


#endif
