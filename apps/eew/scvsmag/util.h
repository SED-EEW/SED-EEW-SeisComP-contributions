/***************************************************************************
 *   Copyright (C) by ETHZ/SED, GNS New Zealand, GeoScience Australia      *
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

#ifndef __SEISCOMP_APPLICATIONS_SCVSMAG_UTIL_H__
#define __SEISCOMP_APPLICATIONS_SCVSMAG_UTIL_H__

#include <seiscomp/datamodel/waveformstreamid.h>
#include <string>

namespace Seiscomp {
namespace Private {

std::string
toStreamID(const DataModel::WaveformStreamID &wfid);

}
}

#endif

