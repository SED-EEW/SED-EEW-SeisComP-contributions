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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_ROUTER_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_ROUTER_H__


#include <seiscomp3/datamodel/inventory.h>
#include <map>
#include <string>


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


DEFINE_SMARTPOINTER(PreProcessor);
DEFINE_SMARTPOINTER(Router);

/**
 * @brief The Router class implements routing of records to waveform processors.
 *        It separates the vertical and horizontal components and feeds them
 *        into corresponding EEW processors.
 */
class SC_LIBEEWAMPS_API Router : public Core::BaseObject {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		Router();

		//! D'tor
		~Router();


	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		/**
		 * @brief Sets the inventory used to query for meta data. The pointer
		 *        is not managed by this class and the caller must ensure
		 *        that the passed inventory pointer is valid for each call
		 *        of route.
		 * @param inv
		 */
		void setInventory(const DataModel::Inventory *inv);

		/**
		 * @brief Sets the global configuration object that is forwarded to
		 *        all created processors.
		 * @param cfg The config pointer thats ownership is being not transferred.
		 */
		void setConfig(const Config *cfg);

		/**
		 * @brief Resets the routing table and removes all WaveformProcessor
		 *        instances that have been created so far.
		 */
		void reset();


	// ----------------------------------------------------------------------
	//  Router interface
	// ----------------------------------------------------------------------
	public:
		virtual PreProcessor *route(const Record *rec);
		virtual bool route(const DataModel::Pick *pick);


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		typedef std::map<std::string, PreProcessorPtr> RoutingTable;
		typedef std::multimap<std::string, PreProcessorPtr> StationIndexTable;

		const DataModel::Inventory *_inventory;
		const Config               *_config;
		RoutingTable                _routingTable;
		StationIndexTable           _stationIndexTable;
};


}
}
}


#endif
