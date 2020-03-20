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
 *   Developed by gempa GmbH                                               *
 ***************************************************************************/


#ifndef __SEISCOMP_IO_RECORDSTREAM_VS_H__
#define __SEISCOMP_IO_RECORDSTREAM_VS_H__


#include <seiscomp/core/interruptible.h>
#include <seiscomp/core/datetime.h>
#include <seiscomp/core/genericrecord.h>
#include <seiscomp/io/recordstream.h>
#include <seiscomp/io/recordstream/streamidx.h>
#include <seiscomp/datamodel/vs/vs_package.h>
#include <seiscomp/messaging/connection.h>

#include <set>


class VSRecord : public Seiscomp::GenericRecord {
	public:
		VSRecord() : next(NULL) {}

		void read(std::istream &in) {}

	public:
		VSRecord *next;
};


struct Node {
	Node(const std::string &c) : code(c) {}

	std::string code;

	bool operator<(const Node &node ) const {
		return code < node.code;
	}

	bool matches(const std::string &c) const {
		return Seiscomp::Core::wildcmp(code, c);
	}

	typedef std::set<Node> List;
	static List::const_iterator findMatch(const List &list, const std::string &code) {
		List::const_iterator it;
		for ( it = list.begin(); it != list.end(); ++it ) {
			if ( it->matches(code) ) return it;
		}
		return it;
	}

	mutable List childs;
};



class VSConnection : public Seiscomp::IO::RecordStream {
	public:
		//! C'tor
		VSConnection();
		
		//! Destructor
		virtual ~VSConnection();

	public:
		//! Initialize the arclink connection.
		virtual bool setSource(const std::string &source);
		
		virtual bool addStream(const std::string &networkCode,
		                       const std::string &stationCode,
		                       const std::string &locationCode,
		                       const std::string &channelCode);

		virtual bool addStream(const std::string &networkCode,
		                       const std::string &stationCode,
		                       const std::string &locationCode,
		                       const std::string &channelCode,
		                       const Seiscomp::Core::Time &startTime,
		                       const Seiscomp::Core::Time &endTime);
  
		//! Adds the given start time to the server connection description
		virtual bool setStartTime(const Seiscomp::Core::Time &stime);
		
		//! Adds the given end time to the server connection description
		virtual bool setEndTime(const Seiscomp::Core::Time &etime);

		//! Terminates the arclink connection.
		virtual void close();

		//! Returns the data stream
		virtual Seiscomp::Record *next();

		//! Removes all stream list, time window, etc. -entries from the connection description object.
		bool clear();

		//! Reconnects a terminated arclink connection.
		bool reconnect();


	private:
		bool connect();
		bool handle(Seiscomp::DataModel::VS::Envelope *);
		bool isRequested(const std::string &net, const std::string &sta,
		                 const std::string &loc, const std::string &cha) const;


	private:
		std::string                      _host;
		std::string                      _group;
		bool                             _closeRequested;
		Seiscomp::Client::ConnectionPtr  _connection;
		Node::List                       _streams;
		VSRecord                        *_queue;
};


#endif

