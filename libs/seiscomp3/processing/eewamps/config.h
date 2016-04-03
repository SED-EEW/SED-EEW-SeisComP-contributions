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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_CONFIG_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_CONFIG_H__


#include <seiscomp3/processing/eewamps/api.h>
#include <seiscomp3/processing/waveformprocessor.h>
#include <boost/function.hpp>

#include "baseprocessor.h"


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


class FilterBankRecord : public GenericRecord {
	public:
		FilterBankRecord(size_t n, const Record& rec);
		~FilterBankRecord();

	public:
		DoubleArrayPtr *filteredData;
		size_t          n;
};


class TauCRecord : public GenericRecord {
	public:
		TauCRecord(const Record& rec) : GenericRecord(rec) {}

	public:
		DoubleArray displacement;
};


typedef FilterBankRecord GbARecord;
typedef std::pair<double,double> PassBand;


/**
 * @brief The Config class that is used in all processing classes: Router and
 *        Processor.
 */
struct SC_LIBEEWAMPS_API Config {
	Config();
	~Config();


	// ----------------------------------------------------------------------
	//  General configuration
	// ----------------------------------------------------------------------

	/**
	 * Enables dumping of incoming, transformed and combined records to stdout.
	 * This mode is for debugging. The default is false.
	 */
	bool dumpRecords;

	/**
	 * Relative saturation threshold in percent. If the absolute raw amplitude
	 * exceeds X% of 2**23 counts the station will be excluded from
	 * processing. This assumes a 24bit datalogger. The default is 80.
	 */
	double saturationThreshold;

	/**
	 * Length in seconds of the baseline correction buffer. A running average is
	 * computed and removed from the current sample. The default is 60s.
	 */
	double baseLineCorrectionBufferLength;

	/**
	 * Length in seconds of the taper that is applied to the beginning of the
	 * trace.
	 */
	double taperLength;

	/**
	 * An array that configures what signals are requested and required by
	 * the application. If e.g. only MeterPerSecondSquared is enable then
	 * each velocity data stream is converted to acceleration and only that
	 * converted data stream is being processed and also dumped (if enabled).
	 */
	bool wantSignal[WaveformProcessor::SignalUnit::Quantity];

	/**
	 * Sets the buffer size for each component to compensate for
	 * data acquisition latencies. Each component is buffered for
	 * at least as long as \p ts seconds before single records
	 * are being removed. A too short buffer size will lead to
	 * gaps in the combined data stream. bufferSize is as time span (in
	 * seconds with fractional part). The default is 60s.
	 */
	Core::TimeSpan horizontalBufferSize;

	/**
	 * Sets the maximum tolerated delay before issuing a warning. That
	 * parameter does not configure buffers as such but only a threshold it
	 * is being checked against to tell the user that something is not
	 * real-time anymore. maxDelay as time span (in seconds with fractional
	 * part). The default is 60s.
	 */
	Core::TimeSpan horizontalMaxDelay;

	/**
	 * Sets the maximum tolerated package delay before issuing a warning.
	 * The default is 3s.
	 */
	Core::TimeSpan maxDelay;


	// ----------------------------------------------------------------------
	//  VS and FinDer configuration
	// ----------------------------------------------------------------------

	struct VsFndr {
		/**
		 * Enables envelope processing support. Default is false.
		 */
		bool enable;

		/**
		 * Interval length for envelope computation. The default is 1s.
		 */
		Core::TimeSpan envelopeInterval;

		//! Enable acceleration 3s lo-pass filter, default is false.
		bool filterAcc;

		//! Enable velocity 3s lo-pass filter, default is false.
		bool filterVel;

		//! Enable displacement 3s lo-pass filter, default is true.
		bool filterDisp;

		typedef boost::function<void (const BaseProcessor*,
		                              double value,
		                              const Core::Time &timestamp,
		                              bool clipped)>
		        PublishFunc;

		PublishFunc publish;
	};


	// ----------------------------------------------------------------------
	//  Gutenberg configuration
	// ----------------------------------------------------------------------

	struct GbA {
		/**
		 * Enables envelope processing support. Default is false.
		 */
		bool enable;

		/**
		 * The buffer size in seconds of the GbA amplitudes. Default is 10s.
		 */
		Core::TimeSpan bufferSize;

		/**
		 * The cutoff time after picks are discarded. Default is 10s.
		 */
		Core::TimeSpan cutOffTime;

		/**
		 * The filter passbands. The default is 9 octaves with an upper
		 * frequency of 48Hz.
		 */
		std::vector<PassBand> passbands;


		typedef boost::function<void (const BaseProcessor *proc,
		                              const std::string &pickID,
		                              double peakPerPassband[9],
		                              const Core::Time &peakTime,
		                              const Core::Time &startTime,
		                              const Core::Time &endTime,
		                              bool clipped)>
		        PublishFunc;

		PublishFunc publish;
	};


	// ----------------------------------------------------------------------
	//  Onsite magnitude processing configuration
	// ----------------------------------------------------------------------

	struct OMP {
		/**
		 * Enables onsite magnitude processing support. Default is false.
		 */
		bool enable;

		Core::TimeSpan tauPDeadTime;

		/**
		 * The cutoff time after a trigger for tauP calculation. Default is 3s.
		 */
		Core::TimeSpan cutOffTime;

		typedef boost::function<void (const BaseProcessor *proc,
		                              const std::string &pickID,
		                              const Core::Time &peakTime,
		                              const Core::Time &startTime,
		                              const Core::Time &endTime,
		                              double tauP,
		                              bool clipped)>
		        TauPPublishFunc;

		typedef boost::function<void (const BaseProcessor *proc,
		                              const std::string &pickID,
		                              const Core::Time &startTime,
		                              const Core::Time &endTime,
		                              double tauC, double Pd,
		                              bool clipped)>
		        TauCPublishFunc;

		TauPPublishFunc publishTauP;
		TauCPublishFunc publishTauCPd;
	};


	VsFndr vsfndr; //!< VS configuration
	GbA    gba;    //!< Gutenberg configuration
	OMP    omp;    //!< Onsite magnitude processing configuration
};


}
}
}


#endif
