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
#include <seiscomp3/io/recordfilter/demux.h>

#include "processor.h"
#include "router.h"
#include "router.h"
#include "recordfilter/gainandbaselinecorrection.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
struct Processor::Members {
	Config                       config;  //!< Global configuration object
	Router                       router;  //!< Record router
	IO::RecordFilterInterfacePtr demuxer; //!< Record stream demultiplexer
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Processor::Processor() {
	_members = new Members;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Processor::~Processor() {
	delete _members;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::setInventory(const DataModel::Inventory *inventory) {
	_inventory = inventory;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::setConfiguration(const Config &config) {
	_members->config = config;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const Config &Processor::configuration() const {
	return _members->config;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::showConfig() const {
	SEISCOMP_DEBUG("------------------------------------------");
	SEISCOMP_DEBUG("EEW config");
	SEISCOMP_DEBUG("------------------------------------------");
	SEISCOMP_DEBUG("dump-records        : %s", _members->config.dumpRecords ? "yes":"no");
	SEISCOMP_DEBUG("saturation-threshold: %f%%", _members->config.saturationThreshold);
	SEISCOMP_DEBUG("baseline-corr-buffer: %fs", (double)_members->config.baseLineCorrectionBufferLength);
	SEISCOMP_DEBUG("taper length        : %fs", (double)_members->config.taperLength);
	SEISCOMP_DEBUG("hor-buffer-size     : %fs", (double)_members->config.horizontalBufferSize);
	SEISCOMP_DEBUG("hor-max-delay       : %fs", (double)_members->config.horizontalMaxDelay);
	SEISCOMP_DEBUG("max-delay           : %fs", (double)_members->config.maxDelay);
	SEISCOMP_DEBUG("enable-acc          : %s", _members->config.wantSignal[WaveformProcessor::MeterPerSecondSquared] ? "yes":"no");
	SEISCOMP_DEBUG("enable-vel          : %s", _members->config.wantSignal[WaveformProcessor::MeterPerSecond] ? "yes":"no");
	SEISCOMP_DEBUG("enable-disp         : %s", _members->config.wantSignal[WaveformProcessor::Meter] ? "yes":"no");
	SEISCOMP_DEBUG("enable-vsfndre      : %s", _members->config.vsfndr.enable ? "yes":"no");
	SEISCOMP_DEBUG("enable-gba          : %s", _members->config.gba.enable ? "yes":"no");
	SEISCOMP_DEBUG("enable-omp          : %s", _members->config.omp.enable ? "yes":"no");
	SEISCOMP_DEBUG("vs-envelope-interval: %fs", (double)_members->config.vsfndr.envelopeInterval);
	SEISCOMP_DEBUG("vs-filter-acc       : %s", _members->config.vsfndr.filterAcc ? "yes":"no");
	SEISCOMP_DEBUG("vs-filter-vel       : %s", _members->config.vsfndr.filterVel ? "yes":"no");
	SEISCOMP_DEBUG("vs-filter-disp      : %s", _members->config.vsfndr.filterDisp ? "yes":"no");
	SEISCOMP_DEBUG("gba-buffer-size     : %fs", (double)_members->config.gba.bufferSize);
	SEISCOMP_DEBUG("gba-cutoff-time     : %fs", (double)_members->config.gba.cutOffTime);
	SEISCOMP_DEBUG("gba-passbands       : %d", (int)_members->config.gba.passbands.size());
	for ( size_t i = 0; i < _members->config.gba.passbands.size(); ++i )
		SEISCOMP_DEBUG("  [%02d] %f - %fHz", (int)i,
		               _members->config.gba.passbands[i].first,
		               _members->config.gba.passbands[i].second);
	SEISCOMP_DEBUG("taup-dead-time      : %fs", (double)_members->config.omp.tauPDeadTime);
	SEISCOMP_DEBUG("taup-cutoff-time    : %fs", (double)_members->config.omp.cutOffTime);
	SEISCOMP_DEBUG("------------------------------------------");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::setEnvelopeCallback(Config::VsFndr::PublishFunc callback) {
	_members->config.vsfndr.publish = callback;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::setGbACallback(Config::GbA::PublishFunc callback) {
	_members->config.gba.publish = callback;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::addAllowRule(const std::string &rule) {
	_streamFirewall.allow.insert(rule);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::addDenyRule(const std::string &rule) {
	_streamFirewall.deny.insert(rule);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::showRules() const {
	SEISCOMP_DEBUG("Apply %d allow and %d deny rules",
	               (int)_streamFirewall.allow.size(),
	               (int)_streamFirewall.deny.size());

	Util::StringFirewall::StringSet::iterator it;
	for ( it = _streamFirewall.allow.begin();
	      it != _streamFirewall.allow.end(); ++it )
		SEISCOMP_DEBUG(" + %s", it->c_str());

	for ( it = _streamFirewall.deny.begin();
	      it != _streamFirewall.deny.end(); ++it )
		SEISCOMP_DEBUG(" - %s", it->c_str());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Processor::clearRules() {
	_streamFirewall = Util::WildcardStringFirewall();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Processor::isStreamIDAllowed(const std::string &id) const {
	return _streamFirewall.isAllowed(id);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Processor::init(const Seiscomp::Config::Config &conf,
                     const std::string &configPrefix) {
	if ( _inventory == NULL )
		return false;

	// ----------------------------------------------------------------------
	// Read black- and whitelists
	// ----------------------------------------------------------------------
	try {
		std::vector<std::string> tokens = conf.getStrings(configPrefix + "streams.whitelist");
		for ( size_t i = 0; i < tokens.size(); ++i )
			addAllowRule(tokens[i]);
	}
	catch ( ... ) {}

	try {
		std::vector<std::string> tokens = conf.getStrings(configPrefix + "streams.blacklist");
		for ( size_t i = 0; i < tokens.size(); ++i )
			addDenyRule(tokens[i]);
	}
	catch ( ... ) {}


	// ----------------------------------------------------------------------
	// General configuration
	// ----------------------------------------------------------------------
	try {
		_members->config.saturationThreshold = conf.getDouble(configPrefix + "saturationThreshold");
	}
	catch ( ... ) {}

	try {
		_members->config.baseLineCorrectionBufferLength = conf.getDouble(configPrefix + "baselineCorrectionBuffer");
	}
	catch ( ... ) {}

	try {
		_members->config.taperLength = conf.getDouble(configPrefix + "taperLength");
	}
	catch ( ... ) {}

	try {
		_members->config.horizontalBufferSize = conf.getDouble(configPrefix + "horizontalBuffer");
	}
	catch ( ... ) {}

	try {
		_members->config.horizontalMaxDelay = conf.getDouble(configPrefix + "debug.maxHorizontalGap");
	}
	catch ( ... ) {}

	try {
		_members->config.maxDelay = conf.getDouble(configPrefix + "debug.maxDelay");
	}
	catch ( ... ) {}


	// ----------------------------------------------------------------------
	// Gutenberg algorithm configuration
	// ----------------------------------------------------------------------
	try {
		_members->config.gba.bufferSize = conf.getDouble(configPrefix + "filterbank.bufferLength");
	}
	catch ( ... ) {}

	try {
		_members->config.gba.cutOffTime = conf.getDouble(configPrefix + "filterbank.cutoffTime");
	}
	catch ( ... ) {}
	// TODO: read passbands


	// ----------------------------------------------------------------------
	// VS/FinDer configuration
	// ----------------------------------------------------------------------
	try {
		_members->config.vsfndr.envelopeInterval = conf.getDouble(configPrefix + "vsfndr.envelopeInterval");
	}
	catch ( ... ) {}

	try {
		_members->config.vsfndr.filterAcc = conf.getBool(configPrefix + "vsfndr.filterAcc");
	}
	catch ( ... ) {}

	try {
		_members->config.vsfndr.filterVel = conf.getBool(configPrefix + "vsfndr.filterVel");
	}
	catch ( ... ) {}

	try {
		_members->config.vsfndr.filterDisp = conf.getBool(configPrefix + "vsfndr.filterDisp");
	}
	catch ( ... ) {}


	// ----------------------------------------------------------------------
	// Onsite magnitude processing configuration
	// ----------------------------------------------------------------------
	try {
		_members->config.omp.tauPDeadTime = conf.getDouble(configPrefix + "taup.deadTime");
	}
	catch ( ... ) {}


	// ----------------------------------------------------------------------
	// Setup processing chains
	// ----------------------------------------------------------------------
	IO::GainAndBaselineCorrectionRecordFilter<float> *tpl = new IO::GainAndBaselineCorrectionRecordFilter<float>(_inventory);
	tpl->setSaturationThreshold((1 << 23)*_members->config.saturationThreshold*0.01);
	tpl->setBaselineCorrectionBufferLength(_members->config.baseLineCorrectionBufferLength);
	tpl->setTaperLength(_members->config.taperLength);

	_members->demuxer = new IO::RecordDemuxFilter(tpl);
	_members->router.setConfig(&_members->config);
	_members->router.setInventory(_inventory);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Processor::subscribeToChannels(IO::RecordStream *rs,
                                    const Core::Time &refTime) {
	if ( _inventory == NULL )
		return false;

	for ( size_t n = 0; n < _inventory->networkCount(); ++n ) {
		DataModel::Network *net = _inventory->network(n);
		if ( net->start() > refTime ) continue;
		try { if ( net->end() <= refTime ) continue; }
		catch ( ... ) {}

		for ( size_t s = 0; s < net->stationCount(); ++s ) {
			DataModel::Station *sta = net->station(s);
			if ( sta->start() > refTime ) continue;
			try { if ( sta->end() <= refTime ) continue; }
			catch ( ... ) {}

			for ( size_t l = 0; l < sta->sensorLocationCount(); ++l ) {
				DataModel::SensorLocation *loc = sta->sensorLocation(l);
				if ( loc->start() > refTime ) continue;
				try { if ( loc->end() <= refTime ) continue; }
				catch ( ... ) {}

				for ( size_t c = 0; c < loc->streamCount(); ++c ) {
					DataModel::Stream *cha = loc->stream(c);
					if ( cha->start() > refTime ) continue;
					try { if ( cha->end() <= refTime ) continue; }
					catch ( ... ) {}

					std::string sid = net->code() + "." + sta->code() + "." + loc->code() + "." + cha->code();
					if ( _streamFirewall.isAllowed(sid) ) {
						SEISCOMP_DEBUG("+ %s.%s.%s.%s", net->code().c_str(),
						               sta->code().c_str(), loc->code().c_str(),
						               cha->code().c_str());
						rs->addStream(net->code(), sta->code(), loc->code(), cha->code());
					}
				}
			}
		}
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Processor::feed(const Seiscomp::Record *rec) {
	if ( _members->demuxer == NULL )
		return false;

	if ( _streamFirewall.isDenied(rec->streamID()) )
		return false;

	try {
		Core::TimeSpan delay = Core::Time::GMT() - rec->endTime();
		if ( delay > _members->config.maxDelay ) {
			SEISCOMP_WARNING("%s: max delay exceeded: %fs",
			                 rec->streamID().c_str(), (double)delay);
		}
	}
	catch ( ... ) {
		// Invalid end time
		return false;
	}

	RecordPtr out = _members->demuxer->feed(rec);
	if ( out )
		return _members->router.route(out.get()) != NULL;

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Processor::feed(const Seiscomp::DataModel::Pick *pick) {
	return _members->router.route(pick);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
