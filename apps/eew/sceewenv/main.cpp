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
 *   SeisComP EEW envelope modules that generates envelope amplitudes        *
 *   with libseiscomp_eewamps and sends them as data messages to the         *
 *   messaging bus.                                                           *
 *                                                                            *
 *   -----------------------------------------------------------------------  *
 *                                                                            *
 *   Author: Jan Becker, gempa GmbH <jabe@gempa.de>                           *
 *                                                                            *
 ******************************************************************************/


#define SEISCOMP_COMPONENT EEWENV

#include <seiscomp/logging/log.h>
#include <seiscomp/core/datamessage.h>
#include <seiscomp/client/streamapplication.h>
#include <seiscomp/client/inventory.h>
#include <seiscomp/io/records/mseedrecord.h>
#include <seiscomp/io/archive/xmlarchive.h>
#include <seiscomp/processing/eewamps/processor.h>

// This is required as datamodel/vs now resides in contrib-sed
#if SC_API_VERSION < SC_API_VERSION_CHECK(14,0,0)
	#include <seiscomp3/datamodel/vs/vs_package.h>
#else
	#include <seiscomp/datamodel/vs/vs_package.h>
#endif

#include <functional>
#include <string>


using namespace std;
using namespace Seiscomp;


/**
 * @brief The sceewenv application class. This class wraps envelope processing
 *        using the eewamps library.
 */
class App : public Client::StreamApplication {
	public:
		App(int argc, char** argv) : Client::StreamApplication(argc, argv) {
			setMessagingEnabled(true);
			setDatabaseEnabled(true, true);

			setLoadStationsEnabled(true);
			setLoadConfigModuleEnabled(false);

			setRecordDatatype(Array::FLOAT);
			setPrimaryMessagingGroup("AMPLITUDE");

			_sentMessages = _sentMessagesTotal = 0;
			_testMode = false;
		}


		void createCommandLineDescription() {
			StreamApplication::createCommandLineDescription();

			commandline().addGroup("Streams");
			commandline().addOption("Streams", "dump", "Dump all processed streams as mseed to stdout");

			commandline().addOption("Messaging", "test", "Test mode, no messages are sent");

			commandline().addGroup("Offline");
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

			return true;
		}


		bool init() {
			if ( !StreamApplication::init() )
				return false;

			_testMode = commandline().hasOption("test");

			_creationInfo.setAgencyID(agencyID());
			_creationInfo.setAuthor(author());

			Processing::EEWAmps::Config eewCfg;
			eewCfg.dumpRecords = commandline().hasOption("dump");

			eewCfg.vsfndr.enable = true;

			// Convert to all signal units
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecondSquared] = true;
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecond] = true;
			eewCfg.wantSignal[Processing::WaveformProcessor::Meter] = true;

			_eewProc.setConfiguration(eewCfg);
			_eewProc.setEnvelopeCallback(bind(&App::handleEnvelope, this,
			                                  placeholders::_1,
			                                  placeholders::_2,
			                                  placeholders::_3,
			                                  placeholders::_4));
			_eewProc.setInventory(Client::Inventory::Instance()->inventory());

			if ( !_eewProc.init(configuration(), "eewenv.") )
				return false;

			_eewProc.showConfig();
			_eewProc.showRules();

			if ( commandline().hasOption("dump-config") )
				return true;

			if ( _startTime.valid() ) recordStream()->setStartTime(_startTime);
			if ( _endTime.valid() ) recordStream()->setEndTime(_endTime);

			_eewProc.subscribeToChannels(recordStream(), Core::Time::GMT());

			// We do not need lookup objects by publicID
			DataModel::PublicObject::SetRegistrationEnabled(false);

			_sentMessages = 0;
			_sentMessagesTotal = 0;

			return true;
		}


		void handleRecord(Record *rec) {
			RecordPtr tmp(rec);

			// Clear the current envelope map
			_creationInfo.setCreationTime(Core::Time::GMT());

			_eewProc.feed(rec);

			// Since processing happens demultiplexed on individual channels
			// we have to multiplex again

			if ( !_testMode ) {
				Envelopes::iterator it;
				for ( it = _currentEnvelopes.begin(); it != _currentEnvelopes.end(); ++it ) {
					Core::DataMessagePtr msg = new Core::DataMessage;
					msg->attach(it->second.get());

					if ( connection() ) {
						connection()->send(msg.get());
						++_sentMessages;
						++_sentMessagesTotal;

						// Request a sync token every 100 messages
						if ( _sentMessages > 100 ) {
							_sentMessages = 0;

							// Tell the record acquisition to request synchronization and to
							// stop sending records until the sync is completed. Only required
							// for API versions lower than 14.0.0
							#if SC_API_VERSION < SC_API_VERSION_CHECK(14,0,0)
							    requestSync();
							#endif
						}
					}
				}
			}
			else
				++_sentMessagesTotal;

			_currentEnvelopes.clear();
		}


		void handleEnvelope(const Processing::EEWAmps::BaseProcessor *proc,
		                    double value, const Core::Time &timestamp,
		                    bool clipped) {
			// Objects with empty publicID are created since they are currently
			// not sent as notifiers and stored in the database
			DataModel::VS::EnvelopePtr envelope = _currentEnvelopes[EnvelopeKey(proc->streamID(), timestamp)];
			DataModel::VS::EnvelopeChannelPtr cha;

			if ( !envelope ) {
				envelope = new DataModel::VS::Envelope("");
				envelope->setCreationInfo(_creationInfo);
				envelope->setNetwork(proc->waveformID().networkCode());
				envelope->setStation(proc->waveformID().stationCode());
				envelope->setTimestamp(timestamp);

				cha = new DataModel::VS::EnvelopeChannel("");
				if ( proc->usedComponent() == Processing::WaveformProcessor::Vertical )
					cha->setName("V");
				else
					cha->setName("H");
				cha->setWaveformID(proc->waveformID());

				envelope->add(cha.get());

				_currentEnvelopes[EnvelopeKey(proc->streamID(), timestamp)] = envelope;
			}
			else {
				cha = envelope->envelopeChannel(0);
			}

			DataModel::VS::EnvelopeValuePtr val;

			// Add acceleration value
			val = new DataModel::VS::EnvelopeValue;
			val->setValue(value);
			switch ( proc->signalUnit() ) {
				case Processing::WaveformProcessor::Meter:
					val->setType("disp");
					break;
				case Processing::WaveformProcessor::MeterPerSecond:
					val->setType("vel");
					break;
				case Processing::WaveformProcessor::MeterPerSecondSquared:
					val->setType("acc");
					break;
				default:
					break;
			}

			if ( clipped ) val->setQuality(DataModel::VS::EnvelopeValueQuality(DataModel::VS::clipped));
			cha->add(val.get());

			if ( _eewProc.configuration().dumpRecords ) {
				GenericRecord tmp;
				tmp.setNetworkCode(proc->waveformID().networkCode());
				tmp.setStationCode(proc->waveformID().stationCode());
				switch ( proc->signalUnit() ) {
					case Processing::WaveformProcessor::Meter:
						tmp.setLocationCode("ED");
						break;
					case Processing::WaveformProcessor::MeterPerSecond:
						tmp.setLocationCode("EV");
						break;
					case Processing::WaveformProcessor::MeterPerSecondSquared:
						tmp.setLocationCode("EA");
						break;
					default:
						break;
				}

				if ( proc->usedComponent() != Processing::WaveformProcessor::Vertical )
					tmp.setChannelCode(proc->waveformID().channelCode()+'X');
				else
					tmp.setChannelCode(proc->waveformID().channelCode());

				tmp.setStartTime(timestamp);
				tmp.setSamplingFrequency(1.0 / _eewProc.configuration().vsfndr.envelopeInterval.length());

				FloatArrayPtr data = new FloatArray(1);
				data->set(0, value);
				tmp.setData(data.get());

				IO::MSeedRecord mseed(tmp);
				mseed.write(std::cout);
			}
		}


		bool run() {
			if ( commandline().hasOption("dump-config") )
				return true;

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


	private:
		typedef std::pair<std::string, Core::Time> EnvelopeKey;
		typedef std::map<EnvelopeKey, DataModel::VS::EnvelopePtr> Envelopes;

		std::string                    _allowString, _denyString;
		Processing::EEWAmps::Processor _eewProc;
		Envelopes                      _currentEnvelopes;
		DataModel::CreationInfo        _creationInfo;

		int                            _sentMessages;
		size_t                         _sentMessagesTotal;

		bool                           _testMode;

		std::string                    _strTs;
		std::string                    _strTe;

		Core::Time                     _appStartTime;
		Core::Time                     _startTime;
		Core::Time                     _endTime;
};


int main(int argc, char **argv) {
	return App(argc, argv)();
}
