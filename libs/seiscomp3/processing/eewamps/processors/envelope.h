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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_PROCESSORS_ENVELOPE_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_PROCESSORS_ENVELOPE_H__


#include "../baseprocessor.h"
#include "../samplepool.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


/**
 * @brief The EnvelopeProcessor class computes envelope values in a configurable
 *        interval (vsfndr.envelopeInterval).
 *
 * The data are optionally filtered with a low pass filter of 3s.
 */
class SC_LIBEEWAMPS_API EnvelopeProcessor : public BaseProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		explicit EnvelopeProcessor(const Config *config, SignalUnit unit);

		//! D'tor
		~EnvelopeProcessor();


	// ----------------------------------------------------------------------
	//  WaveformProcessor interface
	// ----------------------------------------------------------------------
	protected:
		virtual void process(const Record *rec, const DoubleArray &filteredData);


	// ----------------------------------------------------------------------
	//  Private methods
	// ----------------------------------------------------------------------
	private:
		//! Setup the current time window according to a reference time
		void setupTimeWindow(const Core::Time &ref);
		void flush(const Record *rec);


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		SamplePool      _samplePool;
		Core::TimeSpan  _dt;
		Core::Time      _currentStartTime;
		Core::Time      _currentEndTime;
};


}
}
}


#endif
