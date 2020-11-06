#!/usr/bin/env python
"""
Copyright (C) by ETHZ/SED

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

@author: behry
"""
from ud_interface import HeartBeat, CoreEventInfo

if __name__ == '__main__':
    if False:
        hb = HeartBeat()
        hb.send_hb()

    if False:
        evt = CoreEventInfo()
        evt.id = 2
        evt.mag = 6.9
        evt.mag_uncer = 0.5
        evt.lat = 49.298798
        evt.lat_uncer = 0.5
        evt.lon = 7.885094
        evt.lon_uncer = 0.5
        evt.dep = 5.0
        evt.dep_uncer = 10.0
        evt.o_time_uncer = 2.0
        evt.likelihood = 1.0
        evt.send(evt.DMMessageEncoder())

    if True:
        # print defaults
        evt = CoreEventInfo(host='localhost', port=61619, topic='/topic/eew.sys.dm.data',
                            username='decimod', password='weAreOK')
        print evt.DMMessageEncoder()
        hb = HeartBeat(host='localhost', port=61619, topic='/topic/eew.sys.dm.data',
                            username='decimod', password='weAreOK')
        print hb.send_hb()
