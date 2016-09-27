/******************************************************************************
 *   Copyright (C) by gempa GmbH                                              *
 *                                                                            *
 *   You can redistribute and/or modify this program under the                *
 *   terms of the SeisComP Public License.                                    *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   SeisComP Public License for more details.                                *
 *                                                                            *
 *   -----------------------------------------------------------------------  *
 *                                                                            *
 *   SeisComP3 wrapper for the FinDer algorithm using libFinDer written by    *
 *   Deborah E. Smith (deborahsmith@usgs.gov).                                *
 *                                                                            *
 *   -----------------------------------------------------------------------  *
 *                                                                            *
 *   Author: Jan Becker, gempa GmbH <jabe@gempa.de>                           *
 *                                                                            *
 ******************************************************************************/


#define SEISCOMP_COMPONENT SCFINDER

#include <seiscomp3/logging/log.h>
#include <seiscomp3/logging/output.h>
#include <seiscomp3/client/streamapplication.h>
#include <seiscomp3/client/inventory.h>
#include <seiscomp3/client/queue.h>
#include <seiscomp3/datamodel/eventparameters.h>
#include <seiscomp3/datamodel/origin.h>
#include <seiscomp3/datamodel/magnitude.h>
#include <seiscomp3/io/archive/xmlarchive.h>
#include <seiscomp3/processing/eewamps/processor.h>

#include "finder.h"
#include "finite_fault.h"


#define USE_FINDER
//#define LOG_AMPS

using namespace Seiscomp;
using namespace Seiscomp::DataModel;
using namespace FiniteFault;


namespace {


/**
 * @brief Logging output implementation to write messages to stdout.
 */
class StdoutOutput : public Logging::Output {
	public:
		StdoutOutput() {}

	protected:
		void log(const char */*channelName*/,
		         Logging::LogLevel /*level*/,
		         const char* msg,
		         time_t /*time*/) {
			cout << msg << endl;
		}
};


template <typename T>
bool epochMatch(const T *obj, const Core::Time &ref) {
	// Before start, does not match
	if ( ref < obj->start() )
		return false;


	try {
		if ( ref >= obj->end() )
			return false;
	}
	// Open end does always match
	catch ( ... ) {}

	return true;
}


/**
 * @brief The Ring class implements a simple ring buffer of any type which has
 *        an attribute timestamp. Elements are organized ordered by timestamp
 *        and max(timestamp) - min(timestamp) will never exceed the configured
 *        capacity.
 */
template <typename T>
class Ring : public std::deque<T> {
	public:
		Ring(const Core::TimeSpan &capacity = Core::TimeSpan(0,0))
		: _capacity(capacity) {}


	public:
		void setCapacity(const Core::TimeSpan &capacity) {
			_capacity = capacity;

			if ( std::deque<T>::empty() ) return;

			Core::Time tmin = std::deque<T>::back().timestamp - _capacity;
			while ( !std::deque<T>::empty()
			   &&   (std::deque<T>::front().timestamp < tmin) )
				std::deque<T>::pop_front();
		}


		bool feed(const T &v) {
			typename std::deque<T>::iterator it;

			// Find correct insert position

			// When empty, put it at the end
			if ( std::deque<T>::empty() )
				it = std::deque<T>::end();
			else {
				// When the last value is before value, append value
				const T &last = std::deque<T>::back();
				if ( v.timestamp >= last.timestamp )
					it = std::deque<T>::end();
				else {
					// When first value is after value, prepend value
					if ( v.timestamp < std::deque<T>::front().timestamp ) {
						// Value out of buffers capacity
						if ( v.timestamp < last.timestamp - _capacity )
							return false;

						it = std::deque<T>::begin();
					}
					else {
						// The most timeconsuming task is now to find a
						// proper position to insert the value
						// Precondition: v.timestamp < back.timestamp
						//               v.timestamp >= front.timestamp
						typename std::deque<T>::reverse_iterator current = std::deque<T>::rbegin();

						bool found = false;
						for ( ; current != std::deque<T>::rend(); ++current ) {
							if ( v.timestamp >= (*current).timestamp ) {
								it = (current).base();
								found = true;
								break;
							}
						}

						if ( !found )
							throw std::runtime_error("no buffer location found");
					}
				}
			}

			std::deque<T>::insert(it, v);

			Core::Time tmin = std::deque<T>::back().timestamp - _capacity;
			while ( !std::deque<T>::empty()
			   &&   (std::deque<T>::front().timestamp < tmin) )
				std::deque<T>::pop_front();

			return true;
		}


	private:
		Core::TimeSpan _capacity;
};


}



/**
 * @brief The scfinder application class. This class wraps envelope processing
 *        using the eewamps library and pushing the results to the FinDer
 *        algorithm.
 */
class App : public Client::StreamApplication {
	public:
		App(int argc, char** argv) : Client::StreamApplication(argc, argv) {
			setMessagingEnabled(true);
			setDatabaseEnabled(true, true);

			setLoadStationsEnabled(true);
			setLoadConfigModuleEnabled(false);

			setRecordDatatype(Array::FLOAT);
			setPrimaryMessagingGroup("LOCATION");
			_magnitudeGroup = "MAGNITUDE";

			_sentMessagesTotal = 0;
			_testMode = false;

			_bufferLength = Core::TimeSpan(120,0);
		}


		void createCommandLineDescription() {
			StreamApplication::createCommandLineDescription();

			commandline().addOption("Messaging", "test", "Test mode, no messages are sent");

			commandline().addGroup("Offline");
			commandline().addOption("Offline", "offline", "Do not connect to messaging, this implies --test");
			commandline().addOption("Offline", "dump-config", "Show configuration in debug log and exits");
			commandline().addOption("Offline", "ts", "Start time of data acquisition time window, requires also --te", &_strTs, false);
			commandline().addOption("Offline", "te", "End time of data acquisition time window, requires also --ts", &_strTe, false);
		}


		bool validateParameters() {
			if ( !StreamApplication::validateParameters() ) return false;

			if ( !_strTs.empty() || !_strTe.empty() ) {
				if ( _strTs.empty() ) {
					cerr << "--te requires also --ts" << endl;
					return false;
				}

				if ( _strTe.empty() ) {
					cerr << "--ts requires also --te" << endl;
					return false;
				}

				if ( !_startTime.fromString(_strTs.c_str(), "%F %T") ) {
					cerr << "Wrong start time format: expected %F %T, e.g. \"2010-01-01 12:00:00\"" << endl;
					return false;
				}

				if ( !_endTime.fromString(_strTe.c_str(), "%F %T") ) {
					cerr << "Wrong end time format: expected %F %T, e.g. \"2010-01-01 12:00:00\"" << endl;
					return false;
				}

				if ( _startTime >= _endTime ) {
					cerr << "Acquisition time window is empty or of negative length" << endl;
					return true;
				}
			}

			if ( !isInventoryDatabaseEnabled() )
				setDatabaseEnabled(false, false);

			_testMode = commandline().hasOption("test");

			if ( commandline().hasOption("offline") ) {
				setMessagingEnabled(false);
				_testMode = true;
			}

			return true;
		}


		bool init() {
			if ( !StreamApplication::init() )
				return false;
#ifdef USE_FINDER

			try {
				_finderConfig = configGetPath("finder.config");
			}
			catch ( ... ) {
				SEISCOMP_ERROR("finder.config is mandatory");
				return false;
			}
#endif
			try {
				_magnitudeGroup = configGetString("finder.magnitudeGroup");
			}
			catch ( ... ) {}

			try {
				_bufferLength = configGetDouble("finder.envelopeBufferSize");
			}
			catch ( ... ) {}

			_creationInfo.setAgencyID(agencyID());
			_creationInfo.setAuthor(author());

			Processing::EEWAmps::Config eewCfg;
			eewCfg.dumpRecords = commandline().hasOption("dump");

			eewCfg.vsfndr.enable = true;

			// Convert to all signal units
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecondSquared] = true;
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecond] = false;
			eewCfg.wantSignal[Processing::WaveformProcessor::Meter] = false;

			_eewProc.setConfiguration(eewCfg);
			_eewProc.setEnvelopeCallback(boost::bind(&App::handleEnvelope, this, _1, _2, _3, _4));
			_eewProc.setInventory(Client::Inventory::Instance()->inventory());

			if ( !_eewProc.init(configuration(), "") )
				return false;

			if ( commandline().hasOption("dump-config") )
				return true;

			_eewProc.showConfig();
			_eewProc.showRules();

			if ( _startTime.valid() ) recordStream()->setStartTime(_startTime);
			if ( _endTime.valid() ) recordStream()->setEndTime(_endTime);

			_eewProc.subscribeToChannels(recordStream(), Core::Time::GMT());

			// We do not need lookup objects by publicID
			PublicObject::SetRegistrationEnabled(false);

			_sentMessagesTotal = 0;

			return initFinder();
		}


		bool initFinder() {
			Coordinate_List station_coord_list;

			Inventory *inventory = Client::Inventory::Instance()->inventory();
			Core::Time refTime = Core::Time::GMT();

			for ( size_t n = 0; n < inventory->networkCount(); ++n ) {
				Network *net = inventory->network(n);
				if ( net->start() > refTime ) continue;
				try { if ( net->end() <= refTime ) continue; }
				catch ( ... ) {}

				for ( size_t s = 0; s < net->stationCount(); ++s ) {
					Station *sta = net->station(s);
					if ( sta->start() > refTime ) continue;
					try { if ( sta->end() <= refTime ) continue; }
					catch ( ... ) {}

					for ( size_t l = 0; l < sta->sensorLocationCount(); ++l ) {
						SensorLocation *loc = sta->sensorLocation(l);
						if ( loc->start() > refTime ) continue;
						try { if ( loc->end() <= refTime ) continue; }
						catch ( ... ) {}

						try {
							station_coord_list.push_back(Coordinate(loc->latitude(), loc->longitude()));
							SEISCOMP_DEBUG("+ %s.%s.%s  %f  %f",
							               net->code().c_str(), sta->code().c_str(),
						                   loc->code().c_str(), loc->latitude(), loc->longitude());
						}
						catch ( std::exception &e ) {
							SEISCOMP_WARNING("%s.%s: location '%s': failed to add coordinate: %s",
							                 net->code().c_str(), sta->code().c_str(),
							                 loc->code().c_str(), e.what());
						}
					}
				}
			}

#ifdef USE_FINDER
			try {
				Finder::Init(_finderConfig.c_str(), station_coord_list);
			}
			catch ( FiniteFault::Error &e ) {
				SEISCOMP_ERROR("Finder error: %s", e.what());
				return false;
			}
#endif

			// ­Set the static variable NFinder = 0.  This will be automatically
			// incremented in the Finder class constructor and decremented in
			// the Finder class destructor.
			Finder::Nfinder = 0;

			return true;
		}


		bool run() {
			if ( commandline().hasOption("dump-config") ) {
				StdoutOutput logChannel;
				logChannel.subscribe(Logging::getComponentDebugs("EEWAMPS"));

				_eewProc.showConfig();
				_eewProc.showRules();

				return true;
			}

			_appStartTime = Core::Time::GMT();
			return StreamApplication::run();
		}


		void done() {
			Core::Time now = Core::Time::GMT();
			int secs = (now-_appStartTime).seconds();
			if ( !_testMode )
				SEISCOMP_INFO("Sent %ld messages with an average of %d messages per second",
				              (long int)_sentMessagesTotal, (int)(secs > 0?_sentMessagesTotal/secs:_sentMessagesTotal));
			else {
				double runTime = (double)(now-_appStartTime);
				SEISCOMP_INFO("Generated %ld messages with an average of %d messages per second and %f ms per message",
				              (long int)_sentMessagesTotal, (int)(secs > 0?_sentMessagesTotal/secs:_sentMessagesTotal),
				              1000*runTime / _sentMessagesTotal);
			}

			StreamApplication::done();
		}


		void handleRecord(Record *rec) {
			RecordPtr tmp(rec);
			_eewProc.feed(rec);
		}


		void handleEnvelope(const Processing::EEWAmps::BaseProcessor *proc,
		                    double value, const Core::Time &timestamp,
		                    bool clipped) {
			if ( proc->signalUnit() != Processing::WaveformProcessor::MeterPerSecondSquared ) {
				SEISCOMP_WARNING("Unexpected envelope unit: %s",
				                 proc->signalUnit().toString());
				return;
			}

			string id = proc->waveformID().networkCode() + "." +
			            proc->waveformID().stationCode() + "." +
			            proc->waveformID().locationCode();

			LocationLookup::iterator it;
			it = _locationLookup.find(id);
			if ( it == _locationLookup.end() || !epochMatch(it->second->meta, timestamp) ) {
				SensorLocation *loc;
				loc = Client::Inventory::Instance()->getSensorLocation(
					proc->waveformID().networkCode(),
					proc->waveformID().stationCode(),
					proc->waveformID().locationCode(),
					timestamp
				);

				if ( loc == NULL ) {
					SEISCOMP_WARNING("%s.%s: no sensor location found for '%s' at time %s: ignore envelope value",
					                 proc->waveformID().networkCode().c_str(),
					                 proc->waveformID().stationCode().c_str(),
					                 proc->waveformID().locationCode().c_str(),
					                 timestamp.iso().c_str());
					return;
				}

				try {
					loc->latitude();
					loc->longitude();
				}
				catch ( std::exception &e ) {
					SEISCOMP_WARNING("%s.%s: location '%s': failed to add coordinate: %s",
					                 proc->waveformID().networkCode().c_str(),
					                 proc->waveformID().stationCode().c_str(),
					                 proc->waveformID().locationCode().c_str(),
					                 e.what());
					return;
				}

				if ( it == _locationLookup.end() ) {
					BuddyPtr buddy = new Buddy;
					buddy->pgas.setCapacity(_bufferLength),
					buddy->meta = loc;
					_locationLookup[id] = buddy;
					it = _locationLookup.find(id); 
				}
				else
					it->second->meta = loc;
			}

			// The reference time is the global ticker of the current time. That
			// should work for real-time as well as playbacks. It always points
			// to the latest timestamp of any envelope value.
			bool referenceTimeUpdated = false;

			if ( _referenceTime < timestamp ) {
				_referenceTime = timestamp;
				referenceTimeUpdated = true;
			}

			// The minimum time for a valid amplitude
			Core::Time minAmplTime = _referenceTime - _bufferLength;

			bool needFinderUpdate = false;

#if !defined(USE_FINDER) || defined(LOG_AMPS)
			std::cout << "+ " << id << "." << proc->waveformID().channelCode() << "   " << _referenceTime.iso() << "   " << minAmplTime.iso() << "   " << timestamp.iso() << "   " << value << std::endl;

#endif
			// Buffer envelope value
			if ( it->second->pgas.feed(Amplitude(value, timestamp, proc->waveformID().channelCode())) ) {
				// Buffer changed -> update maximum
				if ( (it->second->maxPGA.timestamp < minAmplTime)
				  || (timestamp < minAmplTime)
				  || (value >= it->second->maxPGA.value) ) {
					if ( it->second->updateMaximum(minAmplTime) ) {
#if !defined(USE_FINDER) || defined(LOG_AMPS)
						std::cout << "M " << id << "   " << it->second->maxPGA.timestamp.iso() << "   " << it->second->maxPGA.value << std::endl;
#endif
						needFinderUpdate = true;
					}
				}
			}

			// If reference time has updated then all locations must be updated
			// as well
			if ( referenceTimeUpdated ) {
				for ( it = _locationLookup.begin(); it != _locationLookup.end(); ++it ) {
					if ( it->second->maxPGA.timestamp >= minAmplTime ) continue;
					if ( it->second->updateMaximum(minAmplTime) ) {
#if !defined(USE_FINDER) || defined(LOG_AMPS)
						std::cout << "M " << it->first << "   " << it->second->maxPGA.timestamp.iso() << "   " << it->second->maxPGA.value << std::endl;
#endif
						needFinderUpdate = true;
					}
				}
			}

			// No changes, return
			if ( !needFinderUpdate )
				return;

			// Maximum updated, call Finder
			callFinder();
		}


		void callFinder() {
			PGA_Data_List pga_data_list;
			LocationLookup::iterator it;

#if !defined(USE_FINDER) || defined(LOG_AMPS)
			std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
#endif
			for ( it = _locationLookup.begin(); it != _locationLookup.end(); ++it ) {
				if ( !it->second->maxPGA.timestamp.valid() ) continue;

				pga_data_list.push_back(
					PGA_Data(
						it->second->meta->station()->code(),
						it->second->meta->station()->network()->code(),
						it->second->maxPGA.channel.c_str(),
						it->second->meta->code().empty()?"--":it->second->meta->code().c_str(),
						Coordinate(it->second->meta->latitude(), it->second->meta->longitude()),
						it->second->maxPGA.value*100, it->second->maxPGA.timestamp
					)
				);
#if !defined(USE_FINDER) || defined(LOG_AMPS)
				std::cout << it->first << "   " << it->second->maxPGA.timestamp.iso() << "   " << it->second->maxPGA.timestamp.seconds() << "   " << (it->second->maxPGA.value*100) << std::endl;
#endif
			}
#if !defined(USE_FINDER) || defined(LOG_AMPS)
			std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
#ifndef USE_FINDER
			// No finder for the time being
			return;
#endif
#endif
			Coordinate_List clist;
			Coordinate_List::iterator cit;

			_lastFinderCall = Core::Time::GMT();

			try {
				clist = Finder::Scan_Data(pga_data_list, _finderList);
			}
			catch ( FiniteFault::Error &e ) {
				SEISCOMP_ERROR("Exception from FinDer: %s", e.what());
				return;
			}

			for ( cit = clist.begin(); cit != clist.end(); ++cit ) {
				// get the current time for this new finder object
				long event_id = (long)time(NULL);

				if ( Finder::Nfinder > 0 ) {
					if ( event_id <= _finderList[Finder::Nfinder-1]->get_event_id() )
						event_id = _finderList[Finder::Nfinder-1]->get_event_id() + 1;
				}

				// make constructor take a Coordinate designated by *cit...it will increment Nfinder
				_finderList.push_back(new Finder(*cit, pga_data_list, event_id, max(_bufferLength.seconds(),1L)*2));
			}

			Finder_List::iterator fit;
			for ( fit = _finderList.begin(); fit != _finderList.end(); /* incrementing below */) {
				// some method for getting the timestamp associated with the data
				// event_continue == false when we want to stop processing
				try {
					(*fit)->process(_referenceTime, pga_data_list);
				}
				catch ( FiniteFault::Error &e ) {
					SEISCOMP_ERROR("Exception from FinDer::process: %s", e.what());
				}

				if ( (*fit)->get_finder_flags().get_message() &&
				    ((*fit)->get_finder_length_list().size() > 0) )
					sendFinder(*fit);

				if ( !(*fit)->get_finder_flags().get_hold_object() ) {
					(*fit)->stop_processing();
					delete *fit;  // this will decrement Nfinder
					fit = _finderList.erase(fit);
					// remove mask for finder object
				}
				else
					++fit;
			}
		}


		void sendFinder(const Finder *finder) {
			_creationInfo.setCreationTime(Core::Time::GMT());

			Coordinate epicenter = finder->get_epicenter();
			Coordinate epicenter_uncertainty = finder->get_epicenter_uncer();

			OriginPtr org = Origin::Create();
			org->setCreationInfo(_creationInfo);
			org->setLatitude(RealQuantity(epicenter.get_lat(), epicenter_uncertainty.get_lat(), Core::None, Core::None, Core::None));
			org->setLongitude(RealQuantity(epicenter.get_lon(), epicenter_uncertainty.get_lon(), Core::None, Core::None, Core::None));
			org->setDepth(RealQuantity(finder->get_depth()));
			org->setEvaluationMode(EvaluationMode(AUTOMATIC));
			org->setTime(TimeQuantity(Core::Time(finder->get_origin_time())));

			OriginQuality qual;
			qual.setUsedStationCount(finder->get_Nstat_used());

			org->setQuality(qual);

			MagnitudePtr mag = Magnitude::Create();
			mag->setCreationInfo(_creationInfo);
			mag->setMagnitude(RealQuantity(finder->get_mag(), finder->get_mag_uncer(), Core::None, Core::None, Core::None));
			mag->setType("Mfd");

			if ( _testMode ) {
				IO::XMLArchive ar;
				ar.create("-");
				ar.setFormattedOutput(true);
				ar << org;
				ar.close();
			}
			else if ( connection() ){
				NotifierMessagePtr msg;

				msg = new NotifierMessage;
				msg->attach(new Notifier("EventParameters", OP_ADD, org.get()));

				connection()->send(msg.get());

				msg = new NotifierMessage;
				msg->attach(new Notifier(org->publicID(), OP_ADD, mag.get()));

				connection()->send(_magnitudeGroup, msg.get());

				_sentMessagesTotal += 2;
			}
		}


	private:
		struct Amplitude {
			Amplitude() {}
			Amplitude(double v, const Core::Time &ts, const std::string &cha) : value(v), timestamp(ts), channel(cha) {}

			bool operator==(const Amplitude &other) const {
				return value == other.value && timestamp == other.timestamp;
			}

			bool operator!=(const Amplitude &other) const {
				return value != other.value || timestamp != other.timestamp;
			}

			double      value;
			Core::Time  timestamp;
			std::string channel;
		};

		typedef Ring<Amplitude> PGABuffer;

		DEFINE_SMARTPOINTER(Buddy);
		struct Buddy : Core::BaseObject {
			SensorLocation *meta;
			PGABuffer       pgas;
			Amplitude       maxPGA;

			bool updateMaximum(const Core::Time &minTime);
		};

		// Mapping of id=net.sta.loc to SensorLocation object
		typedef map<string, BuddyPtr> LocationLookup;

		bool                           _testMode;
		std::string                    _strTs;
		std::string                    _strTe;
		std::string                    _magnitudeGroup;
		std::string                    _finderConfig;

		Core::TimeSpan                 _bufferLength;

		Processing::EEWAmps::Processor _eewProc;
		CreationInfo                   _creationInfo;

		size_t                         _sentMessagesTotal;

		Core::Time                     _appStartTime;
		Core::Time                     _startTime;
		Core::Time                     _endTime;
		Core::Time                     _referenceTime;
		Core::Time                     _lastFinderCall;

		LocationLookup                 _locationLookup;
		Finder_List                    _finderList;
};


bool App::Buddy::updateMaximum(const Core::Time &minTime) {
	Amplitude lastMaximum = maxPGA;
	maxPGA = Amplitude();

	if ( !pgas.empty() && pgas.back().timestamp >= minTime ) {
		// Update maxmimum
		PGABuffer::iterator it;
		for ( it = pgas.begin(); it != pgas.end(); ++it ) {
			if ( it->timestamp < minTime ) continue;
			if ( !maxPGA.timestamp.valid() || it->value >= maxPGA.value ) {
				maxPGA.timestamp = it->timestamp;
				maxPGA.value = it->value;
				maxPGA.channel = it->channel;
			}
		}
	}

	return maxPGA != lastMaximum;
}


int main(int argc, char **argv) {
	App app(argc, argv);
	return app();
}
