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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_FILTER_TAUP_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_FILTER_TAUP_H__


#include <seiscomp3/processing/eewamps/api.h>
#include <seiscomp3/math/filter.h>


namespace Seiscomp {
namespace Math {
namespace Filtering {


template <typename TYPE>
class TauP : public InPlaceFilter<TYPE> {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		//! C'tor
		TauP();


	// ------------------------------------------------------------------
	//  InplaceFilter interface
	// ------------------------------------------------------------------
	public:
		virtual void setSamplingFrequency(double fsamp);
		virtual int setParameters(int n, const double *params);

		virtual void apply(int n, TYPE *inout);
		virtual InPlaceFilter<TYPE> *clone() const;

		// Resets the filter values
		void reset();


	// ------------------------------------------------------------------
	//  Private members
	// ------------------------------------------------------------------
	private:
		TYPE _V, _D;
		TYPE _last;
		double _alpha;
		double _fsamp;
		bool _init;
};


}
}
}


#endif
