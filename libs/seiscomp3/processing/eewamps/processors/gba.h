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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_PROCESSORS_GBA_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_PROCESSORS_GBA_H__


#include <seiscomp3/core/recordsequence.h>
#include "../baseprocessor.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


/**
 * @brief The GbAProcessor class implements processing for the Gutenberg
 *        algorithm.
 *
 * This algorithms only takes velocity streams into account, either native
 * velocity data or data integrated from acceleration. It filters the data
 * in nine pass bands and requires a minimum sampling rate of 100 sps.
 *
 * A result is only emitted if a trigger (Pick) is available. A trigger is
 * only taken into account if it is not older than a configurable cutoff time.
 * (gba.cutOffTime). For any new waveform package the nine pass bands are
 * calculated and the peak amplitudes within the cutoff time after the trigger
 * are measured and published.
 */
class SC_LIBEEWAMPS_API GbAProcessor : public BaseProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		explicit GbAProcessor(const Config *config, SignalUnit unit);

		//! D'tor
		~GbAProcessor();


	// ----------------------------------------------------------------------
	//  Processor interface
	// ----------------------------------------------------------------------
	public:
		virtual bool store(const Record *rec);
		virtual void reset();
		virtual bool handle(const DataModel::Pick *pick);


	protected:
		virtual void process(const Record *rec, const DoubleArray &filteredData);


	// ----------------------------------------------------------------------
	//  Private methods
	// ----------------------------------------------------------------------
	private:
		DEFINE_SMARTPOINTER(Trigger);
		class Trigger : public Core::BaseObject {
			public:
				Trigger(size_t n, const std::string &pid, const Core::Time &t)
				: publicID(pid), time(t), clipped(false), _n(n) {
					amplitudes = new double[_n];
					for ( size_t i = 0; i < _n; ++i ) amplitudes[i] = 0.0;
				}
				~Trigger() { delete [] amplitudes; }

				std::string publicID;
				Core::Time  time;
				double     *amplitudes;
				Core::Time  maxTime;
				bool        clipped;

			private:
				// Non-copyable
				Trigger(const Trigger &);
				size_t      _n;

			public:
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
		typedef std::deque<TriggerPtr> TriggerBuffer;

		FilterPtr     *_filterBank;
		RingBuffer    *_amplitudeBuffer;
		TriggerBuffer  _triggerBuffer;
};


}
}
}


#endif
