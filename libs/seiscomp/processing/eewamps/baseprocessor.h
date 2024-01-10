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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_BASEPROCESSOR_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_BASEPROCESSOR_H__


#include <seiscomp/datamodel/waveformstreamid.h>
#include <seiscomp/processing/waveformprocessor.h>
#include <seiscomp/processing/eewamps/api.h>


// Forward declaration
namespace Seiscomp {
namespace DataModel {

class Pick;

}
}


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


class Config;


DEFINE_SMARTPOINTER(BaseProcessor);
/**
 * @brief The BaseProcessor class implements the EEW processing on one
 *        particular channel, either vertical or combined horizontals.
 *
 * The channel type actually does not matter. This is handled in parent classes.
 * This class focuses only on the bare metal processing of one incoming data
 * stream.
 *
 * Incoming data are expected to be sensitivity corrected and demeaned.
 *
 * Derived classes must implement the process method.
 */
class SC_LIBEEWAMPS_API BaseProcessor : public WaveformProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		explicit BaseProcessor(const Config *config, SignalUnit unit);

		//! D'tor
		~BaseProcessor();


	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		virtual bool handle(const DataModel::Pick *pick);

		SignalUnit signalUnit() const;

		void setWaveformID(const DataModel::WaveformStreamID &id);
		DataModel::WaveformStreamID waveformID() const;

		const std::string &streamID() const;


	// ----------------------------------------------------------------------
	//  Protected members
	// ----------------------------------------------------------------------
	protected:
		const Config                *_config;
		SignalUnit                   _unit;
		DataModel::WaveformStreamID  _waveformID;
		std::string                  _strWaveformID;
};


inline BaseProcessor::SignalUnit BaseProcessor::signalUnit() const {
	return _unit;
}

inline DataModel::WaveformStreamID BaseProcessor::waveformID() const {
	return _waveformID;
}

inline const std::string &BaseProcessor::streamID() const {
	return _strWaveformID;
}


}
}
}


#endif
