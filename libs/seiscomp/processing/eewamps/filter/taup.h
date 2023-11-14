/******************************************************************************
 *     Copyright (C) by ETHZ/SED                                              *
 *                                                                            *
 *   This program is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE as published *
 *   by the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Affero General Public License for more details.                      *
 *                                                                            *
 *                                                                            *
 *   Jan Becker, gempa GmbH <jabe@gempa.de>                                   *
 ******************************************************************************/


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_FILTER_TAUP_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_FILTER_TAUP_H__


#include <seiscomp/processing/eewamps/api.h>
#include <seiscomp/math/filter.h>


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
