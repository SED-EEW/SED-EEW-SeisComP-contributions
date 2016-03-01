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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_PROCESSOR_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_PROCESSOR_H__


#include <seiscomp3/config/config.h>
#include <seiscomp3/io/recordstream.h>
#include <seiscomp3/utils/stringfirewall.h>

#include "config.h"



namespace Seiscomp {

// Forward declaration
namespace DataModel {

class Pick;
class Inventory;

}

namespace Processing {
namespace EEWAmps {


DEFINE_SMARTPOINTER(Processor);

/**
 * @brief The Processor class implements EEW processing. It combines all
 *        EEW classes in a single easy-to-use interface.
 */
class SC_LIBEEWAMPS_API Processor : public Core::BaseObject {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		Processor();

		//! D'tor
		~Processor();


	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		/**
		 * @brief Sets the inventory. The inventory is required to setup
		 *        the routing tables and sensitivity correction filters.
		 * @param inventory The inventory pointer which is not managed by
		 *                  this instance. It must be valid during the lifetime
		 *                  of the processor.
		 */
		void setInventory(const DataModel::Inventory *inventory);

		/**
		 * @brief setConfiguration
		 * @param config
		 */
		void setConfiguration(const Config &config);

		/**
		 * @brief configuration
		 * @return
		 */
		const Config &configuration() const;

		//! Prints the configuration to debug channel
		void showConfig() const;

		/**
		 * @brief Sets the callback for envelope (VS & Finder) results.
		 * @param callback The callback function
		 */
		void setEnvelopeCallback(Config::VsFndr::PublishFunc callback);

		/**
		 * @brief Sets the callback for Gutenberg algorithm results.
		 * @param callback The callback function
		 */
		void setGbACallback(Config::GbA::PublishFunc callback);

		//! Adds a rule to the stream id whitelist
		void addAllowRule(const std::string &rule);

		//! Adds a rule to the stream id blacklist
		void addDenyRule(const std::string &rule);

		//! Prints the stream id rules to debug channel
		void showRules() const;

		//! Clears all stream filter rules.
		void clearRules();

		/**
		 * @brief Initializes the processor. This method must be called before
		 *        any data packets can be fed.
		 * @param conf The config object to read configuration options from.
		 * @param configPrefix An optional variable name prefix for
		 *                     configuration parameters read from conf.
		 * @return The success flag
		 */
		bool init(const Seiscomp::Config::Config &conf,
		          const std::string &configPrefix = "");

		/**
		 * @brief Adds channel subscriptions to a record stream according
		 *        to the configured black- and whitelist.
		 *
		 * If no inventory instance is set this call will fail. Otherwise all
		 * channels in inventory are traversed, their epoch is matched with
		 * the #refTime parameter and if they pass the stream filter they are
		 * added to the record stream subscriptions.
		 * @param rs The target record stream
		 * @param refTime The reference time for which the channel must be
		 *                active.
		 * @return The success flag
		 */
		bool subscribeToChannels(IO::RecordStream *rs,
		                         const Core::Time &refTime);

		/**
		 * @brief Feeds a record to EEW processing.
		 * @param record The input record
		 * @return true if the record has been used, false otherwise
		 */
		bool feed(const Seiscomp::Record *record);

		/**
		 * @brief Feeds a pick.
		 * @param pick The pick pointer
		 * @return true if the pick has been handled, false otherwise
		 */
		bool feed(const Seiscomp::DataModel::Pick *pick);


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		struct Members;
		Members                      *_members;
		Util::WildcardStringFirewall  _streamFirewall;
		const DataModel::Inventory   *_inventory;
};


}
}
}


#endif
