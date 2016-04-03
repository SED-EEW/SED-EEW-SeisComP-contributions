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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_RECORDFILTER_GAINANDBASELINECORRECTION_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_RECORDFILTER_GAINANDBASELINECORRECTION_H__


#include <seiscomp3/processing/eewamps/api.h>
#include <seiscomp3/datamodel/inventory.h>
#include <seiscomp3/io/recordfilter.h>
#include <seiscomp3/math/filter/average.h>
#include <seiscomp3/math/filter/taper.h>
#include <seiscomp3/math/filter/butterworth.h>


#define BASELINE_CORRECTION_WITH_TAPER
//#define BASELINE_CORRECTION_WITH_HIGHPASS


namespace Seiscomp {
namespace IO {


/**
 * @brief The GainAndBaselineCorrectionRecordFilter class removes the gain and
 *        the baseline (average of past data) from the input record and returns
 *        a corrected output record.
 *
 * For each input a valid epoch is looked up in the inventory for the gain. If
 * no epoch was found the input record is discarded and NULL is returned. To
 * speed up the epoch lookup the inventory is not queried for each incoming
 * record but only if the record time window falls outside the last fetched
 * epoch. So bascially the first record triggers an inventory query and all
 * subsequent records reuse that information unless they are outside the current
 * epoch.
 *
 * Since this class is a template class where only float or double values
 * make sense as output it is instantiated for both types in the implementation
 * file. Do not try to use it with any other type.
 *
 * Note that this filter is not aware of stream identifiers and just checks
 * the last epoch with respect to the input records time window. If multiplexed
 * record streams should be used then use a demultiplexer (RecordDemuxFilter).
 *
 * The default time window for the running mean used by the baseline
 * correction is 60s.
 */
template <typename T>
class SC_LIBEEWAMPS_API GainAndBaselineCorrectionRecordFilter : public RecordFilterInterface {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		/**
		 * @brief GainAndBaselineCorrectionRecordFilter constructor.
		 * @param inv The inventory used to query for meta data. The pointer
		 *            is not managed by this class and the caller must ensure
		 *            that the passed inventory pointer is valid for each call
		 *            of #feed.
		 */
		explicit GainAndBaselineCorrectionRecordFilter(const DataModel::Inventory *inv);

		//! Copy constructor
		GainAndBaselineCorrectionRecordFilter(const GainAndBaselineCorrectionRecordFilter<T> &other);

		//! D'tor
		~GainAndBaselineCorrectionRecordFilter();


	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		/**
		 * @brief Sets the lengths of the baseline correction buffer
		 * @param lengthInSeconds The buffer length in seconds.
		 */
		void setBaselineCorrectionBufferLength(double lengthInSeconds);

		/**
		 * @brief Sets the lengths of the taper at the beginning of the trace
		 * @param lengthInSeconds The taper length in seconds.
		 */
		void setTaperLength(double lengthInSeconds);

		/**
		 * @brief Sets the saturation threshold to check for clipped data.
		 * @param threshold The threshold in absolute amplitudes. A negative
		 *                  value disables the check.
		 */
		void setSaturationThreshold(double threshold);


	// ------------------------------------------------------------------
	//  RecordFilter interface
	// ------------------------------------------------------------------
	public:
		virtual Record *feed(const Record *rec);
		virtual Record *flush();
		virtual void reset();
		virtual RecordFilterInterface *clone() const;


	// ------------------------------------------------------------------
	//  Private methods
	// ------------------------------------------------------------------
	private:
		bool checkEpoch(const Record *rec) const;
		bool queryEpoch(const Record *rec);


	// ------------------------------------------------------------------
	//  Private members
	// ------------------------------------------------------------------
	private:
#ifdef BASELINE_CORRECTION_WITH_HIGHPASS
		typedef Math::Filtering::IIR::ButterworthHighpass<T> BaselineRemoval;
#else
		typedef Math::Filtering::Average<T> BaselineRemoval;
#endif

		const DataModel::Inventory      *_inventory;
		Core::TimeWindow                 _currentEpoch;
		double                           _gainCorrectionFactor;

		Core::Time                       _lastEndTime;
		double                           _samplingFrequency;
		double                           _saturationThreshold;
		double                           _baselineCorrectionLength;
		double                           _taperLength;

		Math::Filtering::InitialTaper<T> _taper;
		BaselineRemoval                  _baselineCorrection;
};


}
}


#endif
