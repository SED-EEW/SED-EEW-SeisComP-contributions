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


#ifndef SEISCOMP_PROCESSING_EEWAMPS_PROCESSORS_ONSITEMAG_H
#define SEISCOMP_PROCESSING_EEWAMPS_PROCESSORS_ONSITEMAG_H


#include <seiscomp/core/recordsequence.h>
#include <seiscomp/core/version.h>
#include <seiscomp/math/filter/butterworth.h>
#include <seiscomp/math/filter/iirintegrate.h>
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
#if SC_API_VERSION < SC_API_VERSION_CHECK(17,0,0)
		typedef Core::SmartPointer<Filter>::Impl FilterPtr;
#else
		typedef Core::SmartPointer<Filter> FilterPtr;
#endif
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
