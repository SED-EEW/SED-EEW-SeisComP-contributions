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


#include "config.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FilterBankRecord::FilterBankRecord(size_t n_, const Record& rec)
: GenericRecord(rec) {
	n = n_;
	filteredData = new DoubleArrayPtr[n];
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
FilterBankRecord::~FilterBankRecord() {
	if ( filteredData )
		delete [] filteredData;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Config::Config() {
	// ----------------------------------------------------------------------
	//  General configuration
	// ----------------------------------------------------------------------
	dumpRecords = false;
	saturationThreshold = 80;
	baseLineCorrectionBufferLength = 60.0;
	taperLength = 60.0;

	for ( int i = 0; i < WaveformProcessor::SignalUnit::Quantity; ++i )
		wantSignal[i] = false;

	horizontalBufferSize = Core::TimeSpan(60,0);
	horizontalMaxDelay = Core::TimeSpan(30,0);
	maxDelay = Core::TimeSpan(5,0);

	// ----------------------------------------------------------------------
	//  VS and FinDer configuration
	// ----------------------------------------------------------------------
	vsfndr.enable = false;
	vsfndr.envelopeInterval = Core::TimeSpan(1,0);

	vsfndr.filterAcc = false;
	vsfndr.filterVel = false;
	vsfndr.filterDisp = true;
	vsfndr.filterCornerFreq = 1.0/3.0;

	// ----------------------------------------------------------------------
	//  Gutenberg configuration
	// ----------------------------------------------------------------------
	gba.enable = false;
	gba.bufferSize = Core::TimeSpan(10,0);
	gba.cutOffTime = Core::TimeSpan(10,0);

	double hiFreq = 48.0;
	for ( int i = 0; i < 9; ++i ) {
		double loFreq = hiFreq*0.5;
		gba.passbands.push_back(PassBand(loFreq, hiFreq));
		hiFreq = loFreq;
	}

	// ----------------------------------------------------------------------
	//  Onsite magnitude processing configuration
	// ----------------------------------------------------------------------
	omp.enable = false;
	omp.tauPDeadTime = Core::TimeSpan(0,0);
	omp.cutOffTime = Core::TimeSpan(3,0);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Config::~Config() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
