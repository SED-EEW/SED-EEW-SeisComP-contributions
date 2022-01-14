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


#ifndef __SEISCOMP_PROCESSING_EEWAMPS_SAMPLEPOOL_H__
#define __SEISCOMP_PROCESSING_EEWAMPS_SAMPLEPOOL_H__


namespace Seiscomp {
namespace Processing {
namespace EEWAmps {


struct SamplePool {
	SamplePool() : samples(NULL), clipped(false), _ofs(0), _size(0) {}
	~SamplePool() { if ( samples != NULL ) delete[] samples; }

	void reset(size_t i) {
		if ( _size == i ) {
			clear();
			return;
		}

		delete[] samples;
		samples = new double[i];
		clipped = false;
		_size = i;
		_ofs = 0;
	}

	size_t capacity() const { return _size; }
	size_t size() const { return _ofs; }
	void clear() { _ofs = 0; clipped = false; }

	void push(double val) {
		if ( _ofs < _size ) samples[_ofs++] = val;
		else throw std::overflow_error("pool overflow");
	}

	double *samples;
	bool    clipped;
	size_t  _ofs;
	size_t  _size;
};


}
}
}


#endif
