/***************************************************************************
 *   Copyright (C) by gempa GmbH                                           *
 *                                                                         *
 *   You can redistribute and/or modify this program under the             *
 *   terms of the SeisComP Public License.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   SeisComP Public License for more details.                             *
 *                                                                         *
 *   --------------------------------------------------------------------  *
 *                                                                         *
 *   Simple test client that reads inventory and data, removes the gain    *
 *   according to the given inventory and corrects for the baseline. The   *
 *   results are written as MiniSEED records to stdout.                    *
 *                                                                         *
 *   Example: prog -I test.mseed -d db-host > test_out.mseed               *
 *                                                                         *
 *   --------------------------------------------------------------------  *
 *                                                                         *
 *   Author: Jan Becker, gempa GmbH <jabe@gempa.de>                        *
 *                                                                         *
 ***************************************************************************/


#define SEISCOMP_COMPONENT TEST

#include <seiscomp3/logging/log.h>
#include <seiscomp3/client/streamapplication.h>
#include <seiscomp3/client/inventory.h>
#include <seiscomp3/io/records/mseedrecord.h>
#include <seiscomp3/processing/eewamps/processor.h>
#include <string>


using namespace std;
using namespace Seiscomp;


class App : public Client::StreamApplication {
	public:
		App(int argc, char** argv) : Client::StreamApplication(argc, argv) {
			setMessagingEnabled(false);
			setDatabaseEnabled(true, true);
			setLoadStationsEnabled(true);
			setLoggingToStdErr(true);
			setRecordDatatype(Array::FLOAT);
		}


		void createCommandLineDescription() {
			Client::Application::createCommandLineDescription();

			commandline().addGroup("Streams");
			commandline().addOption("Streams", "streams-allow,a", "Stream IDs to allow separated by comma", &_allowString);
			commandline().addOption("Streams", "streams-deny,r", "Stream IDs to deny separated by comma", &_denyString);
			commandline().addOption("Streams", "dump", "Dump all processed streams as mseed to stdout");
		}


		bool validateParameters() {
			if ( !StreamApplication::validateParameters() )
				return false;

			if ( !_allowString.empty() ) {
				std::vector<std::string> tokens;
				Core::split(tokens, _allowString.c_str(), ",");
				for ( size_t i = 0; i < tokens.size(); ++i )
					_eewProc.addAllowRule(tokens[i]);
			}

			if ( !_denyString.empty() ) {
				std::vector<std::string> tokens;
				Core::split(tokens, _denyString.c_str(), ",");
				for ( size_t i = 0; i < tokens.size(); ++i )
					_eewProc.addDenyRule(tokens[i]);
			}

			return true;
		}


		bool init() {
			if ( !StreamApplication::init() )
				return false;

			Processing::EEWAmps::Config eewCfg;
			eewCfg.dumpRecords = commandline().hasOption("dump");

			eewCfg.vsfndr.enable = true;
			eewCfg.gba.enable = true;
			eewCfg.omp.enable = true;
			//_config.vsfndr.envelopeInterval.set(0).setUSecs(100000);

			// Convert to all signal units
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecondSquared] = true;
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecond] = true;
			eewCfg.wantSignal[Processing::WaveformProcessor::Meter] = true;

			_eewProc.setConfiguration(eewCfg);
			_eewProc.setEnvelopeCallback(boost::bind(&App::handleEnvelope, this, _1, _2, _3, _4));
			_eewProc.setInventory(Client::Inventory::Instance()->inventory());

			_eewProc.showConfig();
			_eewProc.showRules();

			if ( !_eewProc.init(configuration()) )
				return false;

			_eewProc.subscribeToChannels(recordStream(), Core::Time::GMT());

			return true;
		}


		void handleRecord(Record *rec) {
			RecordPtr tmp(rec);
			_eewProc.feed(rec);
		}


		void handleEnvelope(const Processing::EEWAmps::BaseProcessor *proc,
		                    double value, const Core::Time &timestamp,
		                    bool clipped) {
			/*
			cerr << "- env " << proc->waveformID().networkCode() << "." << proc->waveformID().stationCode();
			cerr << " " << timestamp.iso() << " " << value << " "
			     << (proc->usedComponent() == Processing::WaveformProcessor::Vertical?"V":"H") << " "
			     << proc->signalUnit().toString() << " "
			     << (clipped?"clipped":"good") << endl;
			*/

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
				tmp.setSamplingFrequency(1.0 / _eewProc.configuration().vsfndr.envelopeInterval);

				FloatArrayPtr data = new FloatArray(1);
				data->set(0, value);
				tmp.setData(data.get());

				IO::MSeedRecord mseed(tmp);
				mseed.write(std::cout);
			}
		}


	private:
		std::string                    _allowString, _denyString;
		Processing::EEWAmps::Processor _eewProc;
};


int main(int argc, char **argv) {
	return App(argc, argv)();
}
