/******************************************************************************
 *     Copyright (C) by ETHZ/SED                                              *
 *                                                                            *
 *   This program is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU Affero General Public License as published *
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


#include "diffcentral.h"


namespace Seiscomp {
namespace Math {
namespace Filtering {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename TYPE>
DiffCentral<TYPE>::DiffCentral() : _factor(0), _init(false) {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename TYPE>
void DiffCentral<TYPE>::setSamplingFrequency(double fsamp) {
	_factor = fsamp;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename TYPE>
int DiffCentral<TYPE>::setParameters(int, const double *) {
	return 0;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename TYPE>
void DiffCentral<TYPE>::apply(int n, TYPE *inout) {
	if ( !n ) return;

	TYPE v = inout[0];

	if ( !_init ) {
		_prevVal = v;
		inout[0] = 0;
		_init = true;
		if ( n < 2 )
			return;
	}
	else {
		if ( n > 1 ) {
			inout[0] = 0.5 * _factor*(inout[1]-_prevVal);
		}
		else {
			inout[0] = _factor*(v-_prevVal);
			_prevVal = v;
			return;
		}
	}

	if ( n > 2 ) {
		for ( int i = 1; i < n-1; ++i ) {
			TYPE c = inout[i];
			inout[i] = 0.5 * _factor * (inout[i+1] - v);
			v = c;
		}
	}

	_prevVal = inout[n-1];
	inout[n-1] = _factor * (inout[n-1]-v);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename TYPE>
InPlaceFilter<TYPE> *DiffCentral<TYPE>::clone() const {
	return new DiffCentral<TYPE>;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template <typename TYPE>
void DiffCentral<TYPE>::reset() {
	_init = false;
	_prevVal = 0;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
INSTANTIATE_INPLACE_FILTER(DiffCentral, SC_LIBEEWAMPS_API);
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
