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
 *   -----------------------------------------------------------------------  *
 *                                                                            *
 *   SeisComP wrapper for the FinDer algorithm using libFinder written by    *
 *   Deborah E. Smith (deborahsmith@usgs.gov) and                             *
 *   Maren Böse, ETH, (maren.boese@erdw.ethz.ch).                             *
 *                                                                            *
 *   -----------------------------------------------------------------------  *
 *                                                                            *
 *   Author: Jan Becker, gempa GmbH <jabe@gempa.de>                           *
 *                                                                            *
 ******************************************************************************/

/**
 * To do : 
 * - log PGA
 * - add station contribution regression ? 
 */

#define SEISCOMP_COMPONENT SCFINDER

#include <seiscomp/logging/log.h>
#include <seiscomp/logging/output.h>
#include <seiscomp/core/version.h>
#include <seiscomp/client/streamapplication.h>
#include <seiscomp/client/inventory.h>
#include <seiscomp/client/queue.h>
#include <seiscomp/datamodel/eventparameters.h>
#include <seiscomp/datamodel/origin.h>
#include <seiscomp/datamodel/magnitude.h>

#if SC_API_VERSION < SC_API_VERSION_CHECK(14,0,0)
    #include <seiscomp3/datamodel/strongmotion/strongmotionparameters_package.h>
#else
    #include <seiscomp/datamodel/strongmotion/strongmotionparameters_package.h>
#endif


#include <seiscomp/io/archive/xmlarchive.h>
#include <seiscomp/processing/eewamps/processor.h>
#include <seiscomp/math/geo.h>
#include <functional>

#include "finder.h"
#include "finite_fault.h"
#include "finder_util.h"

#define USE_FINDER
//#define LOG_AMPS
//#define LOG_FINDER_PGA

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
			_strongMotionGroup = "LOCATION";

			_sentMessagesTotal = 0;
			_testMode = false;
			_playbackMode = false;

			_bufferLength = Core::TimeSpan(120,0);
			_bufDefaultLen = Core::TimeSpan(60,0);
			_bufVarLen = Core::TimeSpan(60,0);

			// Default Finder call interval is 1s
			_finderProcessCallInterval.set(1);
			// Default scan call interval is 0.1s
			_finderScanCallInterval.set(1);
			// Default envelope buffer delay is 15s
			_finderMaxEnvelopeBufferDelay.set(15);
			// Default duration of data skipping following clipping is 30s
			_finderClipTimeout.set(30);

			_finderAmplitudesDirty = false;
			_finderScanDataDirty = false;
		}


		void createCommandLineDescription() {
			StreamApplication::createCommandLineDescription();

			commandline().addOption("Messaging", "test", "Test mode, no messages are sent");

			commandline().addGroup("Offline");
			commandline().addOption("Offline", "offline", "Do not connect to messaging, this implies --test");
			commandline().addOption("Offline", "dump-config", "Show configuration in debug log and exits");
			commandline().addOption("Offline", "ts", "Start time of data acquisition time window, requires also --te", &_strTs, false);
			commandline().addOption("Offline", "te", "End time of data acquisition time window, requires also --ts", &_strTe, false);

			commandline().addGroup("Mode");
			commandline().addOption("Mode", "playback", "Run in playback mode which means that the reference time is set to the timestamp to the latest record instead of systemtime");
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
			_playbackMode = commandline().hasOption("playback");

			if ( commandline().hasOption("offline") ) {
				setMessagingEnabled(false);
				_testMode = true;
			}

			return true;
		}


		/**
		 * The function initializes various configuration settings and objects for a program, including
		 * setting up a waveform processor and subscribing to channels for processing.
		 * 
		 * @return A boolean value is being returned.
		 */
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
				_strongMotionGroup = configGetString("finder.strongMotionGroup");
			}
			catch ( ... ) {}

			try {
				_bufferLength = configGetDouble("finder.envelopeBufferSize");
			}
			catch ( ... ) {}

			try {
				_bufDefaultLen = configGetDouble("finder.defaultFinDerEnvelopeLength");
				_bufVarLen = configGetDouble("finder.defaultFinDerEnvelopeLength");
			}
			catch ( ... ) {}

			try {
				_finderProcessCallInterval = configGetDouble("finder.processInterval");
			}
			catch ( ... ) {}

			try {
				_finderScanCallInterval = configGetDouble("finder.scanInterval");
			}
			catch ( ... ) {}

			try {
				_finderMaxEnvelopeBufferDelay = configGetDouble("finder.maxEnvelopeBufferDelay");
			}
			catch ( ... ) {}

			try {
				_finderClipTimeout = configGetDouble("finder.clipTimeout");
			}
			catch ( ... ) {}

			_creationInfo.setAgencyID(agencyID());
			_creationInfo.setAuthor(author());

			Processing::EEWAmps::Config eewCfg;
			eewCfg.dumpRecords = commandline().hasOption("dump");

			eewCfg.vsfndr.enable = true;

			eewCfg.vsfndr.filterCornerFreq = 1.0/3.0;
			try {
				eewCfg.vsfndr.filterCornerFreq = configGetDouble("debug.filterCornerFreq");
			}
			catch ( ... ) {}

			eewCfg.maxDelay = 3.0;
			try {
				eewCfg.maxDelay = configGetDouble("debug.maxDelay");
			}
			catch ( ... ) {}

			// Convert to all signal units
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecondSquared] = true;
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecond] = false;
			eewCfg.wantSignal[Processing::WaveformProcessor::Meter] = false;

			_eewProc.setConfiguration(eewCfg);
			_eewProc.setEnvelopeCallback(bind(&App::handleEnvelope, this,
														placeholders::_1,
														placeholders::_2,
														placeholders::_3,
														placeholders::_4));
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

			if ( !initFinder() )
				return false;

			if ( _finderProcessCallInterval != Core::TimeSpan(0,0) )
				enableTimer(1);

			if ( _finderProcessCallInterval != Core::TimeSpan(0,0) )
				SEISCOMP_INFO("Throttle Finder processing to an interval of %fs", (double)_finderProcessCallInterval);
			else
				SEISCOMP_INFO("Process with Finder on each amplitude update");

			return true;
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

			SEISCOMP_DEBUG("FinDer version is %s",
			               Finder::Get_alg_version().c_str());

			// This doesn't seems to work but doesn't hurt neither, fix on FinDer side?
			SEISCOMP_DEBUG("GMT version is: %s",
			               Finder::Get_GMT_runtime_version().c_str());

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

			if ( clipped ) {
				SEISCOMP_WARNING("[%s] Envelope clipped",proc->streamID().c_str());
			}

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
					SEISCOMP_WARNING("[%s.%s.%s] no sensor location found at time %s: ignore envelope value",
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
					SEISCOMP_WARNING("[%s.%s.%s] failed to add coordinate: %s",
					                 proc->waveformID().networkCode().c_str(),
					                 proc->waveformID().stationCode().c_str(),
					                 proc->waveformID().locationCode().c_str(),
					                 e.what());
					return;
				}

				string gainUnit = "none" ; 
				DataModel::Stream *stream = Client::Inventory::Instance()->getStream(proc->waveformID().networkCode(), 
						proc->waveformID().stationCode(),
						proc->waveformID().locationCode(),
						proc->waveformID().channelCode().substr(0,2)+"Z", 
						timestamp);
				Processing::WaveformProcessor::SignalUnit signalUnit;
				if ( stream ) {
					gainUnit = stream->gainUnit().c_str() ; 
					SEISCOMP_DEBUG("[%s.%s.%s.%s] signal unit found : %s", 
								proc->waveformID().networkCode().c_str(),
								proc->waveformID().stationCode().c_str(),
								proc->waveformID().locationCode().c_str(),
								proc->waveformID().channelCode().c_str(),
								stream->gainUnit().c_str()) ; //.%s] signal unit: %s, proc->waveformID().channelCode().c_str(), stream->gainUnit().c_str());

				} else {
					SEISCOMP_ERROR(
							"[%s.%s.%s.%s] unable to retrieve gain unit from inventory", 
					                 proc->waveformID().networkCode().c_str(),
					                 proc->waveformID().stationCode().c_str(),
					                 proc->waveformID().locationCode().c_str(), 
									 proc->waveformID().channelCode().c_str());
				}
				
				if ( it == _locationLookup.end() ) {
					BuddyPtr buddy = new Buddy;
					buddy->pgas.setCapacity(_bufferLength),
					buddy->meta = loc;
					buddy->gainUnit = gainUnit;
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

			// If in playback mode then use the latest envelope timestamp as
			// reference time.
			Core::Time tick = _playbackMode ? timestamp : Core::Time::GMT();

			if ( _referenceTime < tick ) {
				_referenceTime = tick;
				referenceTimeUpdated = true;
			}

			// The minimum time for a valid amplitude
			//Core::Time minAmplTime = _referenceTime - _bufferLength;
			Core::Time minAmplTime = _referenceTime - _bufVarLen;

			#if defined(LOG_AMPS)
			std::cout << "+ " << id << "." << proc->waveformID().channelCode() << "   " << _referenceTime.iso() << "   " << minAmplTime.iso() << "   " << timestamp.iso() << "   " << value << "   clipped:" << clipped << std::endl;

			#endif
			// Buffer envelope value
			if ( it->second->pgas.feed(Amplitude(value, timestamp, proc->waveformID().channelCode(), clipped)) ) {
				// Buffer changed -> update maximum
				if ( (it->second->maxPGA.timestamp < minAmplTime)
				  || (timestamp < minAmplTime)
				  || (value >= it->second->maxPGA.value) ) {
					if ( it->second->updateMaximum(minAmplTime) ) {
						#if defined(LOG_AMPS)
						std::cout << "M " << id << "   " << it->second->maxPGA.timestamp.iso() << "   " << it->second->maxPGA.value << "   clipped: " << it->second->maxPGA.clipped << std::endl;
						#endif
					}
				}
			}

			// If reference time has updated then all locations must be updated
			// as well
			if ( referenceTimeUpdated ) {
				for ( it = _locationLookup.begin(); it != _locationLookup.end(); ++it ) {
					if ( it->second->maxPGA.timestamp >= minAmplTime ) continue;
					if ( it->second->updateMaximum(minAmplTime) ) {
						#if defined(LOG_AMPS)
						std::cout << "M " << it->first << "   " << it->second->maxPGA.timestamp.iso() << "   " << it->second->maxPGA.value << "   clipped: " << it->second->maxPGA.clipped << std::endl;
						#endif
					}
				}
			}

			_finderAmplitudesDirty = true;

			// Maximum updated, call Finder
			scanFinderData();
		}


		void handleTimeout() {
			// Scan data
			scanFinderData();

			// Call Finder
			processFinder();
		}


		void scanFinderData() {
			// Changed by Maren, Jan 3 2017
			//if ( !_finderAmplitudesDirty )
			//	return;

			if ( _finderScanCallInterval != Core::TimeSpan(0,0) ) {
				// Throttle call frequency
				Core::Time now = Core::Time::GMT();
				if ( now - _lastFinderScanCall < _finderScanCallInterval )
					return;

				_lastFinderScanCall = now;
			}

			_finderAmplitudesDirty = false;
			_finderScanDataDirty = true;

			LocationLookup::iterator it;

			_latestMaxPGAs.clear();

			#if defined(LOG_AMPS)
			std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
			#endif
			#ifdef LOG_FINDER_PGA
			std::cout << _referenceTime.iso() << std::endl;
			#endif
			for ( it = _locationLookup.begin(); it != _locationLookup.end(); ++it ) {
				if ( !it->second->maxPGA.timestamp.valid() ) continue;
				
				string location_code = it->second->meta->code() ;
				if ( it->second->meta->code().empty() ) {
					location_code = "--" ;
				}
				string mseedid = it->second->meta->station()->network()->code() + "." +
					it->second->meta->station()->code() + "." +
			        location_code + "." + 
					it->second->maxPGA.channel;

				/* checks whether the timestamp of the last element in the pgas vector 
				falls within the previous 15 seconds, continuing the loop if true */
				if ( ( it->second->pgas.back().timestamp.seconds() ) < ( _referenceTime.seconds() - _finderMaxEnvelopeBufferDelay ) ) {
					std::cout << "Instrument skipped \t PGA buffer starts (iso,s)\t PGA buffer end (iso,s)\t Reference time (iso,s)" << std::endl;
					std::cout << it->first << "\t" << it->second->pgas.front().timestamp.iso() << "\t" << it->second->pgas.back().timestamp.iso() << "\t" << _referenceTime.iso() << std::endl;
					std::cout << it->first << "\t" << it->second->pgas.front().timestamp.seconds() << "\t" << it->second->pgas.back().timestamp.seconds() <<  "\t" << _referenceTime.seconds() << std::endl;
					continue;
				}

				/* Checking conditions related to clipping of an instrument's data */
				if ( it->second->pgas.back().clipped ) {
					SEISCOMP_DEBUG("[%s] Instrument clipped and skipped", mseedid.c_str());
					continue;
				}
				if ( ( it->second->maxPGA.lastclipped.seconds() ) >= ( _referenceTime.seconds() - _finderClipTimeout ) ) {
					SEISCOMP_DEBUG("[%s] Instrument clipped recently and skipped", mseedid.c_str());
					continue;
				}
				
				_latestMaxPGAs.push_back(
					PGA_Data(
						it->second->meta->station()->code(),
						it->second->meta->station()->network()->code(),
						it->second->maxPGA.channel.c_str(),
						location_code, //it->second->meta->code().empty()?"--":it->second->meta->code().c_str(),
						Coordinate(it->second->meta->latitude(), it->second->meta->longitude()),
						it->second->maxPGA.value*100, 
						it->second->maxPGA.timestamp
					)
				);

				if ( ( strcmp( it->second->gainUnit.c_str(), "M/S**2" ) != 0 ) 
					&& ( strcmp( it->second->gainUnit.c_str(), "m/s**2" ) != 0 ) 
					&& ( strcmp( it->second->gainUnit.c_str(), "M/S/S" ) != 0 ) 
					&& ( strcmp( it->second->gainUnit.c_str(), "m/s/s" ) != 0 )) {
						SEISCOMP_DEBUG("the new PGA from %s  [%s] IS NOT based on an accelerometer",
										mseedid.c_str(),
										it->second->gainUnit.c_str());
						continue;
				}
				// else: the new PGA is based on an accelerograph
				
				SEISCOMP_DEBUG("the new PGA from %s [%s] is based on an accelerometer",
								mseedid.c_str(),
								it->second->gainUnit.c_str());

				/* Checking conflicting PGA for the same network/station/location code */
				for ( size_t i = 0; i < _latestMaxPGAs.size(); ++i ) {
					PGA_Data &pga = _latestMaxPGAs[i];
					if ( strcmp( pga.get_network().c_str(), it->second->meta->station()->network()->code().c_str() ) != 0 ) {
						continue;
					}
					if ( strcmp( pga.get_name().c_str(), it->second->meta->station()->code().c_str() ) != 0 ) {
						continue;
					}
					if ( strcmp( pga.get_location_code().c_str(), location_code.c_str() ) != 0 ) {
						continue;
					}
					// else: same net.sta.loc code

					if ( strcmp( pga.get_channel().substr(0,2).c_str(), it->second->maxPGA.channel.substr(0,2).c_str() ) == 0 ) {				
						continue;
					}
					// else: another instrument code of the same net.sta.loc code

					SEISCOMP_DEBUG("WARNING: conflicting PGA in buffer for %s [%s] (%f cm/s/s) with %s.%s.%s.%s %f %f (%f cm/s/s at %s)",
									mseedid.c_str(),
									it->second->gainUnit.c_str(),
									it->second->maxPGA.value*100,
									pga.get_network().c_str(),
									pga.get_name().c_str(),
									pga.get_location_code().c_str(),
									pga.get_channel().c_str(),
									pga.get_location().get_lat(),
									pga.get_location().get_lon(),
									pga.get_value(),
									Core::Time(pga.get_timestamp()).iso().c_str());					

					_latestMaxPGAs.erase( _latestMaxPGAs.begin() + i );
					i--;
					
					SEISCOMP_DEBUG("WARNING: removed %s.%s.%s.%s %f %f (%f cm/s/s at %s)",
									pga.get_network().c_str(),
									pga.get_name().c_str(),
									pga.get_location_code().c_str(),
									pga.get_channel().c_str(),
									pga.get_location().get_lat(),
									pga.get_location().get_lon(),
									pga.get_value(),
									Core::Time(pga.get_timestamp()).iso().c_str());					
				}				

				#if defined(LOG_AMPS)
				std::cout << it->first << "   " << it->second->maxPGA.timestamp.iso() << "   " << it->second->maxPGA.timestamp.seconds() << "   " << (it->second->maxPGA.value*100) << std::endl;
				#endif
				#ifdef LOG_FINDER_PGA
				std::cout << "\t" << std::setw(12) << it->first << "\t" << it->second->maxPGA.timestamp.iso() << "\t" << (it->second->maxPGA.value*100) << std::endl;
				#endif
			}
			#if defined(LOG_AMPS)
			std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
			#endif

			#ifdef USE_FINDER
			Coordinate_List clist;
			Coordinate_List::iterator cit;

			try {
				clist = Finder::Scan_Data(_latestMaxPGAs, _finderList);
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
				_finderList.push_back(new Finder(*cit, _latestMaxPGAs, event_id, max(_bufferLength.seconds(),1L)*2));
			}
			#else
			std::cerr << "ScanData" << std::endl;
			#endif

			processFinder();
		}


		void processFinder() {
			if ( !_finderScanDataDirty )
				return;

			if ( _finderProcessCallInterval != Core::TimeSpan(0,0) ) {
				// Throttle call frequency
				Core::Time now = Core::Time::GMT();
				if ( now - _lastFinderProcessCall < _finderProcessCallInterval )
					return;

				_lastFinderProcessCall = now;
			}

			// Get the system time to report it to Finder
			Core::Time tick = _playbackMode ? _referenceTime : Core::Time::GMT();

			#ifdef USE_FINDER
			Finder_List::iterator fit;
			double maxRupLen = 0.;
			for ( fit = _finderList.begin(); fit != _finderList.end(); /* incrementing below */) {
				// some method for getting the timestamp associated with the data
				// event_continue == false when we want to stop processing
				try {
					(*fit)->process(tick, _latestMaxPGAs);
				}
				catch ( FiniteFault::Error &e ) {
					SEISCOMP_ERROR("Exception from FinDer::process: %s", e.what());
				}
				if ((*fit)->get_rupture_length() > maxRupLen) {
					maxRupLen = (*fit)->get_rupture_length();
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
			if (_finderList.size() == 0) {
				if (_bufVarLen != _bufDefaultLen) {
					SEISCOMP_DEBUG("Resetting data window to %ld", _bufDefaultLen.seconds());
				}
				_bufVarLen = _bufDefaultLen;
			} else {
				double rup2time = 1.5;
				if (maxRupLen > _bufVarLen * rup2time) {
					double tmp = maxRupLen / rup2time;
					_bufVarLen = min((long)tmp, _bufferLength.seconds());
					SEISCOMP_DEBUG("Increasing data window to %ld because of active FinDer event rupture length %.1f", 
					  _bufVarLen.seconds(), maxRupLen);
				}
			}
			#else
			std::cerr << "ProcessData" << std::endl;
			#endif
		}


		void sendFinder(const Finder *finder) {
			Notifier::SetEnabled(false);

			_creationInfo.setCreationTime(Core::Time::GMT());

			Coordinate epicenter = finder->get_epicenter();
			Coordinate epicenter_uncertainty = finder->get_epicenter_uncer();

			double deltalon,deltalat, azi1, azi2;
			Math::Geo::delazi_wgs84(epicenter.get_lat(),
			                  epicenter.get_lon(), 
			                  epicenter.get_lat()+epicenter_uncertainty.get_lat(),
			                  epicenter.get_lon(),
			                  &deltalat, &azi1, &azi2);
			Math::Geo::delazi_wgs84(epicenter.get_lat(),
			                  epicenter.get_lon(),
			                  epicenter.get_lat(),
			                  epicenter.get_lon()+epicenter_uncertainty.get_lon(),
			                  &deltalon, &azi1, &azi2);
			deltalon = Math::Geo::deg2km(deltalon);
			deltalat = Math::Geo::deg2km(deltalat);

			OriginPtr org = Origin::Create();
			org->setCreationInfo(_creationInfo);
			org->setMethodID("FinDer");

			org->setLatitude(RealQuantity(epicenter.get_lat(), deltalat, Core::None, Core::None, Core::None));
			org->setLongitude(RealQuantity(epicenter.get_lon(), deltalon, Core::None, Core::None, Core::None));
			org->setDepth(RealQuantity(finder->get_depth()));
			org->setEvaluationMode(EvaluationMode(AUTOMATIC));
			org->setTime(TimeQuantity(Core::Time(finder->get_origin_time())));
			SEISCOMP_DEBUG("FinDer epicenter (lat: %f +- %f km lon:%f +- %f km  dep: %f) wrapped in origin: %s", 
					epicenter.get_lat(), deltalat,
					epicenter.get_lon(), deltalon,
					finder->get_depth(),
					org->publicID().c_str()); 

			OriginQuality qual;
			qual.setUsedStationCount(finder->get_Nstat_used());
			qual.setUsedPhaseCount(finder->get_pga_above_min_thresh().size());

			org->setQuality(qual);

			MagnitudePtr mag = Magnitude::Create();
			mag->setCreationInfo(_creationInfo);
			mag->setMethodID("FinDer");
			mag->setMagnitude(RealQuantity(finder->get_mag(), finder->get_mag_uncer(), Core::None, Core::None, Core::None));
			mag->setType("Mfd");
			mag->setStationCount(finder->get_Nstat_used());
			SEISCOMP_DEBUG("FinDer magnitude (Mfd: %f, likelihood: %f) wrapped in magnitude %s", 
					finder->get_mag(),
					finder->get_likelihood_estimate(),
					mag->publicID().c_str());

			MagnitudePtr magr = Magnitude::Create();
			magr->setCreationInfo(_creationInfo);
			magr->setMethodID("FinDer (regression)");
			magr->setMagnitude(RealQuantity(finder->get_mag_reg(), Core::None, Core::None, Core::None, Core::None));
			magr->setType("Mfdr");
			SEISCOMP_DEBUG("FinDer regression magnitude (Mfdr: %f) wrapped in magnitude %s", 
					finder->get_mag_reg(),
					magr->publicID().c_str());

			MagnitudePtr magl = Magnitude::Create();
			magl->setCreationInfo(_creationInfo);
			magl->setMethodID("FinDer (length)");
			magl->setMagnitude(RealQuantity(finder->get_mag_FD(), Core::None, Core::None, Core::None, Core::None));
			magl->setType("Mfdl");
			SEISCOMP_DEBUG("FinDer fault length magnitude (Mfdl: %f) wrapped in magnitude %s", 
					finder->get_mag_FD(),
					magl->publicID().c_str());


			CommentPtr comment = new Comment();
			comment->setId("likelihood");
			comment->setText(Core::toString(finder->get_likelihood_estimate()));
			comment->setCreationInfo(_creationInfo);
			mag->add(comment.get());

			StrongMotion::StrongOriginDescriptionPtr smDesc = StrongMotion::StrongOriginDescription::Create();
			smDesc->setCreationInfo(_creationInfo);
			smDesc->setOriginID(org->publicID());

			StrongMotion::RupturePtr smRupture = StrongMotion::Rupture::Create();
			smDesc->add(smRupture.get());

#if SC_API_VERSION >= SC_API_VERSION_CHECK(11,0,0)
			OriginPtr centroid = Origin::Create();
			centroid->setCreationInfo(_creationInfo);
			org->setEvaluationMode(EvaluationMode(AUTOMATIC));
			centroid->setType(OriginType(CENTROID));
			centroid->setTime(TimeQuantity(Core::Time(finder->get_origin_time())));

			Math::Geo::delazi_wgs84(finder->get_finder_centroid().get_lat(),
			                  finder->get_finder_centroid().get_lon(),
			                  finder->get_finder_centroid().get_lat()+finder->get_finder_centroid_uncer().get_lat(), 
			                  finder->get_finder_centroid().get_lon(), 
			                  &deltalat, &azi1, &azi2);
			Math::Geo::delazi_wgs84(finder->get_finder_centroid().get_lat(),
			                  finder->get_finder_centroid().get_lon(), 
			                  finder->get_finder_centroid().get_lat(), 
			                  finder->get_finder_centroid().get_lon()+finder->get_finder_centroid_uncer().get_lon(), 
			                  &deltalat, &azi1, &azi2);

			centroid->setLatitude(RealQuantity(finder->get_finder_centroid().get_lat(),
			                                   Math::Geo::deg2km(deltalat)));
			centroid->setLongitude(RealQuantity(finder->get_finder_centroid().get_lon(),
			                                    Math::Geo::deg2km(deltalon)));

			{
				centroid->latitude().setPdf(RealPDF1D());
				LogLikelihood2D_List misfitLat = finder->get_centroid_lat_pdf();
				for ( size_t i = 0; i < misfitLat.size(); ++i ) {
					centroid->latitude().pdf().variable().content().push_back(misfitLat[i].get_location().get_lat());
					centroid->latitude().pdf().probability().content().push_back(misfitLat[i].get_llk());
				}
			}

			{
				centroid->longitude().setPdf(RealPDF1D());
				LogLikelihood2D_List misfitLon = finder->get_centroid_lon_pdf();
				for ( size_t i = 0; i < misfitLon.size(); ++i ) {
					centroid->longitude().pdf().variable().content().push_back(misfitLon[i].get_location().get_lon());
					centroid->longitude().pdf().probability().content().push_back(misfitLon[i].get_llk());
				}
			}

			smRupture->setCentroidReference(centroid->publicID());

			{
				RealQuantity ruptureLength;
				ruptureLength.setValue(finder->get_rupture_length());
				Finder_Length_LLK_List misfitLength = finder->get_finder_length_llk_list();
				ruptureLength.setPdf(RealPDF1D());
				for ( size_t i = 0; i < misfitLength.size(); ++i ) {
					ruptureLength.pdf().variable().content().push_back(misfitLength[i].get_value());
					ruptureLength.pdf().probability().content().push_back(misfitLength[i].get_llk());
				}

				smRupture->setLength(ruptureLength);

				CommentPtr comment = new Comment();
				comment->setId("rupture-length");
				comment->setText(Core::toString(finder->get_rupture_length()));
				comment->setCreationInfo(_creationInfo);
				mag->add(comment.get());
			}

      {
        RealQuantity ruptureWidth;
        ruptureWidth.setValue(finder->get_rupture_width());
        smRupture->setWidth(ruptureWidth);
      }

      {
        string faultgeom = "POLYGON Z ((";
        for (size_t i = 0; i < finder->get_finder_rupture_list().size(); i++) {
          faultgeom += Core::toString(finder->get_finder_rupture_list()[i].get_lon());
          faultgeom += " ";
          faultgeom += Core::toString(finder->get_finder_rupture_list()[i].get_lat());
          faultgeom += " ";
          faultgeom += Core::toString(finder->get_finder_rupture_list()[i].get_depth());
          if (i < finder->get_finder_rupture_list().size()-1) {
            faultgeom += ", ";
          }
        }
        faultgeom += "))";
        smRupture->setRuptureGeometryWKT(faultgeom);
      }

#endif
#if SC_API_VERSION >= SC_API_VERSION_CHECK(11,1,0)
			{
				RealQuantity ruptureStrike;
				ruptureStrike.setValue(finder->get_rupture_azimuth());
				ruptureStrike.setUncertainty(finder->get_azimuth_uncer());
				ruptureStrike.setPdf(RealPDF1D());
				Finder_Azimuth_LLK_List misfitAz = finder->get_finder_azimuth_llk_list();
				for ( size_t i = 0; i < misfitAz.size(); ++i ) {
					ruptureStrike.pdf().variable().content().push_back(misfitAz[i].get_value());
					ruptureStrike.pdf().probability().content().push_back(misfitAz[i].get_llk());
				}

				smRupture->setStrike(ruptureStrike);

				CommentPtr comment = new Comment();
				comment->setId("rupture-strike");
				comment->setText(Core::toString(finder->get_rupture_azimuth()));
				comment->setCreationInfo(_creationInfo);
				mag->add(comment.get());
			}
#endif
			if ( _testMode ) {
				org->add(mag.get());

				IO::XMLArchive ar;
				ar.create("-");
				ar.setFormattedOutput(true);
				ar << org;
#if SC_API_VERSION >= SC_API_VERSION_CHECK(11,0,0)
				ar << centroid;
#endif
				ar.close();
			}
			else if ( connection() ) {
				NotifierMessagePtr msg;

				{
					Notifier::SetEnabled(true);

					EventParameters ep;

					ep.add(org.get());
					msg = Notifier::GetMessage();
					connection()->send(msg.get());

					org->add(magr.get());
					org->add(magl.get());
					org->add(mag.get());
					msg = Notifier::GetMessage();

					connection()->send(_magnitudeGroup, msg.get());

#if SC_API_VERSION >= SC_API_VERSION_CHECK(11,0,0)
					ep.add(centroid.get());

#endif
					StrongMotion::StrongMotionParameters smp;
					smp.add(smDesc.get());

					PGA_Data_List pgas = finder->get_pga_above_min_thresh();
					for ( size_t i = 0; i < pgas.size(); ++i ) {
						PGA_Data &pga = pgas[i];
						StrongMotion::RecordPtr rec = StrongMotion::Record::Create();
						rec->setCreationInfo(_creationInfo);
						rec->setWaveformID(WaveformStreamID(pga.get_network(),
						                                    pga.get_name(),
						                                    pga.get_location_code(),
						                                    pga.get_channel(), ""));
						rec->setStartTime(TimeQuantity(Core::Time(pga.get_timestamp())));
						smp.add(rec.get());

						StrongMotion::EventRecordReferencePtr ref = new StrongMotion::EventRecordReference;
						ref->setRecordID(rec->publicID());
						smDesc->add(ref.get());

						StrongMotion::PeakMotionPtr peakMotion;

						peakMotion = new StrongMotion::PeakMotion;
						peakMotion->setType("pga");
						peakMotion->setMotion(RealQuantity(pga.get_value()));
						rec->add(peakMotion.get());
					}

					Notifier::SetEnabled(false);
				}

				msg = Notifier::GetMessage();

				connection()->send(_strongMotionGroup, msg.get());

				_sentMessagesTotal += 3;
			}
		}


	private:
		struct Amplitude {
			Amplitude() {}
			Amplitude(double v, const Core::Time &ts, const std::string &cha, bool cli) : value(v), timestamp(ts), channel(cha), clipped(cli) {}

			bool operator==(const Amplitude &other) const {
				return value == other.value && timestamp == other.timestamp;
			}

			bool operator!=(const Amplitude &other) const {
				return value != other.value || timestamp != other.timestamp;
			}

			double      value;
			Core::Time  timestamp;
			std::string channel;
			bool        clipped;
			Core::Time  lastclipped;
		};

		typedef Ring<Amplitude> PGABuffer;

		DEFINE_SMARTPOINTER(Buddy);
		struct Buddy : Core::BaseObject {
			SensorLocation *meta;
			PGABuffer       pgas;
			Amplitude       maxPGA;
			string          gainUnit;

			bool updateMaximum(const Core::Time &minTime);
		};

		// Mapping of id=net.sta.loc to SensorLocation object
		typedef map<string, BuddyPtr> LocationLookup;

		bool                           _testMode;
		bool                           _playbackMode;
		std::string                    _strTs;
		std::string                    _strTe;
		std::string                    _magnitudeGroup;
		std::string                    _strongMotionGroup;
		std::string                    _finderConfig;

		Core::TimeSpan                 _bufferLength;
		Core::TimeSpan                 _bufDefaultLen;
		Core::TimeSpan                 _bufVarLen;

		Processing::EEWAmps::Processor _eewProc;
		CreationInfo                   _creationInfo;

		size_t                         _sentMessagesTotal;

		Core::Time                     _appStartTime;
		Core::Time                     _startTime;
		Core::Time                     _endTime;
		Core::Time                     _referenceTime;
		Core::Time                     _lastFinderProcessCall;
		Core::Time                     _lastFinderScanCall;

		Core::TimeSpan                 _finderProcessCallInterval;
		Core::TimeSpan                 _finderScanCallInterval;
		Core::TimeSpan                 _finderMaxEnvelopeBufferDelay;
		Core::TimeSpan                 _finderClipTimeout;
		bool                           _finderAmplitudesDirty;
		bool                           _finderScanDataDirty;

		LocationLookup                 _locationLookup;
		Finder_List                    _finderList;
		PGA_Data_List                  _latestMaxPGAs;
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
			// Update latest clipped timestamp
			if ( !it->clipped ) continue ;
			if ( maxPGA.lastclipped > it->timestamp ) continue ;			
			maxPGA.lastclipped = it->timestamp;
		}
	}

	return maxPGA != lastMaximum;
}


int main(int argc, char **argv) {
	App app(argc, argv);
	return app();
}
