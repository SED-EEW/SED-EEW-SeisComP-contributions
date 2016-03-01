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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_PREPROCESSOR_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_PREPROCESSOR_H__


#include <seiscomp3/datamodel/waveformstreamid.h>
#include <seiscomp3/processing/waveformprocessor.h>
#include <seiscomp3/processing/eewamps/api.h>
#include <seiscomp3/io/recordfilter.h>
#include <vector>

#include "baseprocessor.h"


// Forward declaration
namespace Seiscomp {
namespace DataModel {

class Pick;

}
}


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


class Config;


DEFINE_SMARTPOINTER(RoutingProcessor);

/**
 * @brief The RoutingProcessor class implements a dummy processor that forwards
 *        incoming records to concrete implementations such as VS and FinDer
 *        or GbA.
 *
 * During construction it will create all known implementations and attach
 * them according the the given config object.
 */
class SC_LIBEEWAMPS_API RoutingProcessor : public WaveformProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		explicit RoutingProcessor(const Config *config, SignalUnit unit);

		//! D'tor
		~RoutingProcessor();


	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		/**
		 * @brief Compiles a processor *after* the stream configs for the
		 *        used components have been set.
		 * @param id The waveform ID to compile this processor for.
		 * @return A status flag. A processor where compilation failed will not
		 *         produce any output so it can be safely removed from
		 *         processing.
		 */
		virtual bool compile(const DataModel::WaveformStreamID &id);

		virtual bool handle(const DataModel::Pick *pick);


	// ----------------------------------------------------------------------
	//  WaveformProcessor interface
	// ----------------------------------------------------------------------
	protected:
		virtual void process(const Record *rec, const DoubleArray &filteredData);


	// ----------------------------------------------------------------------
	//  Protected members
	// ----------------------------------------------------------------------
	protected:
		typedef std::vector<BaseProcessorPtr> Implementations;
		const Config     *_config;
		SignalUnit        _unit;
		Implementations   _impls;
};


class SC_LIBEEWAMPS_API HRoutingProcessor : public RoutingProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		explicit HRoutingProcessor(const Config *config, SignalUnit unit);


	// ----------------------------------------------------------------------
	//  WaveformProcessor interface
	// ----------------------------------------------------------------------
	public:
		virtual bool feed(const Record *record);
};


DEFINE_SMARTPOINTER(PreProcessor);

/**
 * @brief The PreProcessor class implements a WaveformProcessor and provides
 *        the default EEW stream pre processing. Preprocessing includes
 *        differentiation/integration to acceleration/velocity/displacement.
 *
 * The converted records are being fed into child processors that redirect
 * record to concret algorithm implementations. The general layout looks as
 * follows:
 *
 * - 'this' processes all incoming data according to #_unit
 * - 'child1' processes all virtual co-located channels
 * - 'child2' processes all virtual displacement channels
 */
class SC_LIBEEWAMPS_API PreProcessor : public RoutingProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		explicit PreProcessor(const Config *config);

		//! D'tor
		~PreProcessor();


	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		virtual bool compile(const DataModel::WaveformStreamID &id);
		virtual bool handle(const DataModel::Pick *pick);


	// ----------------------------------------------------------------------
	//  WaveformProcessor interface
	// ----------------------------------------------------------------------
	public:
		virtual void reset();
		virtual bool feed(const Record *rec);
		virtual bool handleGap(Filter *, const Core::TimeSpan &ts, double, double,
		                       size_t missingSamples);


	// ----------------------------------------------------------------------
	//  Protected members
	// ----------------------------------------------------------------------
	protected:
		IO::RecordFilterInterfacePtr _coLocatedFilter;
		IO::RecordFilterInterfacePtr _displacementFilter;
		RoutingProcessorPtr          _coLocatedProc;
		RoutingProcessorPtr          _displacementProc;
		std::string                  _coLocatedLocationCode;
};


DEFINE_SMARTPOINTER(VPreProcessor);

/**
 * @brief The VProcessor class is the processor implementation for the vertical
 *        component. The only difference to the generic processor is that
 *        the used component is configured in the constructor. It is set to
 *        Vertical.
 */
class SC_LIBEEWAMPS_API VPreProcessor : public PreProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		explicit VPreProcessor(const Config *config);

		//! D'tor
		~VPreProcessor();


	// ----------------------------------------------------------------------
	//  Processor interface
	// ----------------------------------------------------------------------
	public:
		virtual bool compile(const DataModel::WaveformStreamID &id);
};


DEFINE_SMARTPOINTER(HPreProcessor);

/**
 * @brief The HProcessor class is the processor implementation for the
 *        horizontal components. It combines both horizontal channels to a
 *        single L2 channel (L2=sqrt(N*N+E*E)) and sets the component code of
 *        the generated records to 'X'. The processing itself is done as in
 *        the generic processor.
 */
class SC_LIBEEWAMPS_API HPreProcessor : public PreProcessor {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		HPreProcessor(const Config *config);

		//! D'tor
		~HPreProcessor();


	// ----------------------------------------------------------------------
	//  WaveformProcessor interface
	// ----------------------------------------------------------------------
	public:
		virtual bool feed(const Record *rec);


	// ----------------------------------------------------------------------
	//  Processor interface
	// ----------------------------------------------------------------------
	public:
		virtual bool compile(const DataModel::WaveformStreamID &id);


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		WaveformOperatorPtr _l2norm;     //!< The L2 combiner: sqrt(N*N+E*E)

		// Private class that forwards handles component combination
		template <typename T> class L2Norm;
};


}
}
}


#endif
