/***************************************************************************
 *   Copyright (C) by ETHZ/SED                                             *
 *                                                                         *
 * This program is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU Affero General Public License as published*
 * by the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU Affero General Public License for more details.                     *
 *                                                                         *
 * Authors of the Software: Jan Becker, Yannik Behr and Stefan Heimers     *
 ***************************************************************************/

#define SEISCOMP_COMPONENT VsMagnitude

#include <seiscomp/logging/filerotator.h>
#include <seiscomp/logging/channel.h>
#include <seiscomp/logging/fd.h>
#include <seiscomp/logging/log.h>

#include <seiscomp/core/datamessage.h>
#include <seiscomp/core/record.h>
#include <seiscomp/core/timewindow.h>

#include <seiscomp/datamodel/pick.h>
#include <seiscomp/datamodel/origin.h>
#include <seiscomp/datamodel/event.h>
#include <seiscomp/datamodel/publicobjectcache.h>
#include <seiscomp/datamodel/vs/vs_package.h>
#include <seiscomp/datamodel/magnitude.h>
#include <seiscomp/datamodel/stationmagnitude.h>

#include <seiscomp/client/inventory.h>
#include <seiscomp/client/application.h>

#include <seiscomp/io/recordstream.h>
#include <seiscomp/io/recordinput.h>

#include <seiscomp/processing/application.h>

#include <seiscomp/math/geo.h>

#include <algorithm>
#include <sstream>
#include <vector>

#include "util.h"
#include "timeline.h"
#include "Vs30Mapping.h"
#include "VsSiteCondition.h"

namespace Seiscomp {

class VsTimeWindow: public Core::TimeWindow {
private:
	Core::Time _pickTime;
public:
	Core::Time pickTime() const {
		return _pickTime;
	}
	void setPickTime(const Core::Time &t)
	{
		_pickTime = t;
	}
};

class VsMagnitude: public Client::Application {
public:
	VsMagnitude(int argc, char **argv);
	~VsMagnitude();

protected:
	typedef std::map<Timeline::StationID, VsTimeWindow> VsWindows;
	typedef std::vector<DataModel::StationMagnitudeCPtr> StaMagArray;

	DEFINE_SMARTPOINTER(VsEvent);
	struct VsEvent: Core::BaseObject {
		double lat;
		double lon;
		double dep;
		Core::Time time;
		Core::Time expirationTime;
		VsWindows stations;
		OPT(double) vsMagnitude;
		Core::Time originArrivalTime;
		Core::Time originCreationTime;
		StaMagArray staMags;
		int vsStationCount;
		Timeline::StationList pickedStations; // all stations contributing picks to an origin
		int pickedStationsCount;
		int pickedThresholdStationsCount;
		int allThresholdStationsCount;
		bool isValid; // set to true or false by quality control in VsMagnitude::process(VsEvent *evt)
		double dthresh;
		double azGap;
		double maxAzGap;
		int update;
		double likelihood;
	};

	typedef std::map<std::string, VsEventPtr> VsEvents;
	typedef std::map<std::string, Core::Time> EventIDBuffer;
	typedef DataModel::PublicObjectTimeSpanBuffer Cache;

	void createCommandLineDescription();
	bool initConfiguration();
	bool init();
	bool validateParameters();

	void handleMessage(Core::Message* msg);
	void handleTimeout();

	void updateObject(const std::string &parentID, DataModel::Object *obj);
	void addObject(const std::string &parentID, DataModel::Object *obj);
	void removeObject(const std::string &parentID, DataModel::Object *obj);

	void handleEvent(DataModel::Event *event);
	void handlePick(DataModel::Pick *pk);
	void handleOrigin(DataModel::Origin *og);

	void processEvents();
	void process(VsEvent *vsevt, DataModel::Event *event);
	void updateVSMagnitude(DataModel::Event *event, VsEvent *vsevt);
	template<typename T>
	bool setComments(DataModel::Magnitude *mag, const std::string id,
			const T value);

	// implemented in qualitycontrol.cpp
	bool isEventValid(double stmag, VsEvent *evt, double &likelihood,
			double &deltamag, double &deltapick);
	double deltaMag(double vsmag, double stmag); // for Quality Control
	double deltaPick(VsEvent *evt); // ratio between the stations that reported a pick to the overall number of stations within a given radius.
	double valid_magnitude_error(double stmag);

	/**
	 Returns the site correction scale.
	 */
	ch::sed::VsSiteCondition::SafList _saflist;
	float siteEffect(double lat, double lon, double MA, ValueType valueType,
			SoilClass &soilClass);

private:
	Cache _cache;
	EventIDBuffer _publishedEvents;
	Timeline _timeline;
	VsEvents _events;
	Core::Time _currentTime;
	Core::Time _lastProcessingTime;
	DataModel::CreationInfo _creationInfo;

	Logging::Channel* _processingInfoChannel;
	Logging::Output* _processingInfoFile;
	Logging::FdOutput* _processingInfoOutput;
	Logging::Channel* _envelopeInfoChannel;
	Logging::Output* _envelopeInfoFile;

	// Configuration
	std::string _vs30filename;
	std::string _proclogfile;
	int _backSlots;
	int _headSlots;
	bool _realtime;
	int _twstarttime;
	int _twendtime;
	int _eventExpirationTime; // number seconds after origin time the calculation of vsmagnitudes is stopped
	std::string _expirationTimeReference;
	double _vs30default;
	int _clipTimeout;
	int _timeout;
	bool _siteEffect; // turn site effects on or off
	double _maxepicdist;
	double _maxazgap;
	bool _logenvelopes;
};

}
