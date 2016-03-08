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
#include <seiscomp3/datamodel/pick.h>
#include <seiscomp3/datamodel/utils.h>

#include "preprocessor.h"
#include "router.h"


using namespace Seiscomp::DataModel;


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Router::Router() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Router::~Router() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PreProcessor *Router::route(const Record *rec) {
	std::string sid = rec->streamID();

	RoutingTable::iterator it = _routingTable.find(sid);
	if ( it != _routingTable.end() ) {
		if ( it->second )
			it->second->feed(rec);
		return it->second.get();
	}

	if ( _inventory == NULL ) {
		SEISCOMP_ERROR("[%s] no inventory set: cannot route record", sid.c_str());
		return NULL;
	}

	// Create a binding
	SensorLocation *loc = getSensorLocation(_inventory,
	                                        rec->networkCode(),
	                                        rec->stationCode(),
	                                        rec->locationCode(),
	                                        rec->startTime());

	if ( loc == NULL ) {
		SEISCOMP_WARNING("[%s] no metadata for sensor location: cannot route record",
		                 sid.c_str());
		return NULL;
	}

	ThreeComponents tc;
	if ( !getThreeComponents(tc, loc, rec->channelCode().substr(0,2).c_str(), rec->startTime()) ) {
		SEISCOMP_WARNING("[%s] could not query three components: cannot route record",
		                 sid.c_str());
		return NULL;
	}

	SEISCOMP_DEBUG("Created new three component routing for %s.%s.%s.%s",
	               rec->networkCode().c_str(),
	               rec->stationCode().c_str(),
	               rec->locationCode().c_str(),
	               rec->channelCode().substr(0,2).c_str());

	DataModel::WaveformStreamID wid;
	wid.setNetworkCode(rec->networkCode());
	wid.setStationCode(rec->stationCode());
	wid.setLocationCode(rec->locationCode());

	VPreProcessorPtr vproc;
	HPreProcessorPtr hproc;

	vproc = new VPreProcessor(_config);
	hproc = new HPreProcessor(_config);

	vproc->streamConfig(PreProcessor::VerticalComponent).init(tc.comps[ThreeComponents::Vertical]);
	vproc->streamConfig(PreProcessor::FirstHorizontalComponent).init(tc.comps[ThreeComponents::FirstHorizontal]);
	vproc->streamConfig(PreProcessor::SecondHorizontalComponent).init(tc.comps[ThreeComponents::SecondHorizontal]);

	wid.setChannelCode(tc.comps[ThreeComponents::Vertical]->code());

	if ( !vproc->compile(wid) ) {
		SEISCOMP_ERROR("Failed to compile vertical processor on %s.%s.%s.%s",
		               rec->networkCode().c_str(),
		               rec->stationCode().c_str(),
		               rec->locationCode().c_str(),
		               rec->channelCode().substr(0,2).c_str());
		vproc = NULL;
	}

	// Remove component code
	wid.setChannelCode(rec->channelCode().substr(0,2));

	hproc->streamConfig(PreProcessor::VerticalComponent).init(tc.comps[ThreeComponents::Vertical]);
	hproc->streamConfig(PreProcessor::FirstHorizontalComponent).init(tc.comps[ThreeComponents::FirstHorizontal]);
	hproc->streamConfig(PreProcessor::SecondHorizontalComponent).init(tc.comps[ThreeComponents::SecondHorizontal]);

	if ( !hproc->compile(wid) ) {
		SEISCOMP_ERROR("Failed to compile horizontal processor on %s.%s.%s.%s",
		               rec->networkCode().c_str(),
		               rec->stationCode().c_str(),
		               rec->locationCode().c_str(),
		               rec->channelCode().substr(0,2).c_str());
		hproc = NULL;
	}

	std::string vid, hid1, hid2;

	vid  = rec->networkCode() + "." + rec->stationCode() + "." + loc->code() + "." +
	       tc.comps[ThreeComponents::Vertical]->code();
	hid1 = rec->networkCode() + "." + rec->stationCode() + "." + loc->code() + "." +
	       tc.comps[ThreeComponents::FirstHorizontal]->code();
	hid2 = rec->networkCode() + "." + rec->stationCode() + "." + loc->code() + "." +
	       tc.comps[ThreeComponents::SecondHorizontal]->code();

	_routingTable[vid]  = vproc;
	_routingTable[hid1] = hproc;
	_routingTable[hid2] = hproc;

	if ( vproc )
		_stationIndexTable.insert(StationIndexTable::value_type(rec->networkCode() + "." + rec->stationCode(), vproc));
	if ( hproc )
		_stationIndexTable.insert(StationIndexTable::value_type(rec->networkCode() + "." + rec->stationCode(), hproc));

	if ( vid == sid ) {
		if ( vproc )
			vproc->feed(rec);
		return vproc.get();
	}
	else if ( hid1 == sid || hid2 == sid ) {
		if ( hproc )
			hproc->feed(rec);
		return hproc.get();
	}
	else {
		SEISCOMP_WARNING("[%s] channel code does not match any of the three "
		                      "components for %s",
		                 sid.c_str(), rec->channelCode().substr(0,2).c_str());
		return NULL;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Router::route(const Pick *pick) {
	bool routed = false;

	if ( pick == NULL )
		return routed;

	std::string sid = pick->waveformID().networkCode() + "." + pick->waveformID().stationCode();
	std::pair<StationIndexTable::iterator, StationIndexTable::iterator> itq = _stationIndexTable.equal_range(sid);
	for ( StationIndexTable::iterator it = itq.first; it != itq.second; ++it ) {
		if ( it->second->handle(pick) )
			routed = true;
	}

	return routed;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Router::setInventory(const DataModel::Inventory *inv) {
	_inventory = inv;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Router::setConfig(const Config *cfg) {
	_config = cfg;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Router::reset() {
	_routingTable.clear();
	_stationIndexTable.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
