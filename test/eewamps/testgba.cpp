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
			if ( !isInventoryDatabaseEnabled() )
				setDatabaseEnabled(false, false);

			return true;
		}


		bool init() {
			first = true;
			if ( !StreamApplication::init() )
				return false;

			//Create test picks
			_pickabk = new DataModel::Pick("TESTPICK_ABK");
			Core::Time ptime(1996,8,10,18,12,21,840000);
			Core::Time now = Core::Time::GMT();
			DataModel::CreationInfo ci;
			ci.setCreationTime(now);
			ci.setAgencyID("GbA");
			_pickabk->setCreationInfo(ci);
			_pickabk->setTime(DataModel::TimeQuantity(ptime));
			_pickabk->setWaveformID(DataModel::WaveformStreamID("BO","ABK","","HGZ","SomeURI/ABK"));
			_pickabk->setPhaseHint(DataModel::Phase("P"));

			_pickabo = new DataModel::Pick("TESTPICK_ABO");
			ci.setCreationTime(now);
			ci.setAgencyID("GbA");
			_pickabo->setCreationInfo(ci);
			_pickabo->setTime(DataModel::TimeQuantity(ptime));
			_pickabo->setWaveformID(DataModel::WaveformStreamID("BO","ABO","","HGZ","SomeURI/ABO"));
			_pickabo->setPhaseHint(DataModel::Phase("P"));

			Processing::EEWAmps::Config eewCfg;
			eewCfg.dumpRecords = commandline().hasOption("dump");

			eewCfg.baseLineCorrectionBufferLength = 40.0;
			eewCfg.taperLength = 10.0;
			eewCfg.gba.enable = true;
			eewCfg.gba.bufferSize = Core::TimeSpan(20,0);
			eewCfg.gba.cutOffTime = Core::TimeSpan(20,0);

			// Convert to all signal units
			eewCfg.wantSignal[Processing::WaveformProcessor::MeterPerSecond] = true;

			_eewProc.setConfiguration(eewCfg);
			_eewProc.setGbACallback(boost::bind(&App::handleFilterBank, this, _1, _2, _3, _4, _5, _6));
			_eewProc.setInventory(Client::Inventory::Instance()->inventory());

			if ( !_eewProc.init(configuration()) )
				return false;

			_eewProc.showConfig();
			_eewProc.showRules();
			_eewProc.subscribeToChannels(recordStream(), Core::Time::GMT());
			return true;
		}


		void handleRecord(Record *rec) {
			RecordPtr tmp(rec);
			_eewProc.feed(rec);
			Core::Time start(rec->startTime());
			if ((double)start > (double)_pickabk->time().value() && first){
				_eewProc.feed(_pickabk);
				_eewProc.feed(_pickabo);
				SEISCOMP_DEBUG("Sending pick at %s",start.iso().c_str());
				first = false;
			}

		}

		void handleFilterBank(const Processing::EEWAmps::BaseProcessor *proc,
		                      std::string pickID, double *amps,
		                      const Core::Time &max, const Core::Time &ptime,
		                      const Core::Time &maxevaltime){
			cout << proc->usedComponent() << "; " << proc->streamID() <<"; ";
			cout << ptime.iso() << "; " << max.iso() << "; ";
			for(size_t i=0; i<_eewProc.configuration().gba.passbands.size(); i++){
				cout << amps[i] << " ";
			}
			cout << endl;
		}

	private:
		std::string                    _allowString, _denyString;
		Processing::EEWAmps::Processor _eewProc;
		DataModel::Pick *_pickabk, *_pickabo;
		bool first;
};


int main(int argc, char **argv) {
	return App(argc, argv)();
}
