/******************************************************************************
 *     Copyright (C) by ETHZ/SED                                              *
 *                                                                            *
 *   This program is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU Affero General Public License as published *
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
		virtual bool store(const Record *rec);
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
