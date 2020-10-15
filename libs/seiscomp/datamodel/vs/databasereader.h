/***************************************************************************
 *   Copyright (C) by ETHZ/SED                                             *
 *                                                                         *
 * This program is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE as published*
 * by the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This software is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU Affero General Public License for more details.                     *
 *                                                                         *
 *   Developed by gempa GmbH                                               *
 ***************************************************************************/

// This file was created by a source code generator.
// Do not modify the contents. Change the definition and run the generator
// again!

#ifndef SEISCOMP_DATAMODEL_STRONGMOTION_DATABASEREADER_H
#define SEISCOMP_DATAMODEL_STRONGMOTION_DATABASEREADER_H


#include <seiscomp/datamodel/object.h>
#include <seiscomp/datamodel/databasequery.h>
#include <seiscomp/datamodel/vs/api.h>


namespace Seiscomp {
namespace DataModel {
namespace VS {

class VS;
class Envelope;
class EnvelopeChannel;
class EnvelopeValue;

DEFINE_SMARTPOINTER(StrongMotionReader);

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
/** \brief Database reader class for the scheme classes
 *  This class uses a database interface to read objects from a database.
 *  Different database backends can be implemented by creating a driver
 *  in seiscomp/services/database.
 */
class SC_VS_API VSReader : public DatabaseQuery {
	// ----------------------------------------------------------------------
	//  Xstruction
	// ----------------------------------------------------------------------
	public:
		//! Constructor
		VSReader(Seiscomp::IO::DatabaseInterface* dbDriver);

		//! Destructor
		~VSReader();


	// ----------------------------------------------------------------------
	//  Read methods
	// ----------------------------------------------------------------------
	public:
		VS* loadVS();
		int load(VS*);
		int loadEnvelopes(VS*);
		int load(Envelope*);
		int loadEnvelopeChannels(Envelope*);
		int load(EnvelopeChannel*);
		int loadEnvelopeValues(EnvelopeChannel*);

};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


}
}
}


#endif
