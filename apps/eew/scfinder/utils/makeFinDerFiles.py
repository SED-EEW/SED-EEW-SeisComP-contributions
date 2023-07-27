########
# Copyright (C) by ETHZ/SED 
#  
# This program is free software: you can redistribute it and/or modify 
# it under the terms of the GNU Affero General Public License as published 
# by the Free Software Foundation, either version 3 of the License, or 
# (at your option) any later version. 
#  
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU Affero General Public License for more details. 
#  
# Author of the Software: Fred Massin (SED-ETHZ)
########

# This code is a Python script that processes seismic data to calculate the peak ground acceleration
# (PGA) at different time intervals. It uses the ObsPy library to read seismic data and metadata, and
# the NumPy library for array manipulation. It must be provided with arguments
# 1 - envelope data as produced by SeisComP module sceewenv (see --dump option)
# 2 - instrument metadata corresponding to 1, in a ObsPy-compatible format (station level metadata is enough)

from os import makedirs
from os.path import exists
from sys import argv
from obspy import read, read_inventory
from numpy import argmax,argsort

metadata = read_inventory(argv[2])
print(metadata)
data = read(argv[1]).select(location='EA')
print(data)

starttime = min([tr.stats.starttime for tr in data]) 
endtime = max([tr.stats.endtime for tr in data])
PGA = {tr.id:[] for tr in data}
timePGA = {tr.id:[] for tr in data}

longitudes={}
latitudes={}
for tr in data: 
    id=tr.id.split('.')
    st = metadata.select(network=id[0],
                            station=id[1],
                            time=starttime)
    longitudes[tr.id]=st[0][0].longitude
    latitudes[tr.id]=st[0][0].latitude

data_temp = '%d'%(int(starttime.timestamp))
if not exists(data_temp):
    makedirs(data_temp)
print(data_temp)

times = []
for v,stop in enumerate([starttime+s for s in range(1,int(endtime-starttime))]):

    times.append(stop)

    for tr in data:

        PGA[tr.id].append(0)
        timePGA[tr.id].append(0)

    for tr in data.slice(starttime, stop, nearest_sample=False):

        mx = argmax(tr.data)
        PGA[tr.id][-1] = tr.data[mx]
        timePGA[tr.id][-1] = tr.times("timestamp")[mx]


    """ 
    # 1679363108 1
    9.26317  -83.245  TC.DRK0.HH.--  1679363089  40.0414
    8.88848  -83.1477  OV.CRUC.HN.01  1679363093.5  14.9281
    12.7606  -87.4647  NU.PARN.HNZ.05  1679363058.5  0.494763 """    

    with open('%s/data_%d'%(data_temp,v), 'w') as file:

        print('%s/data_%d'%(data_temp,v))
        file.write('# %d  %d'%(starttime.timestamp,v))

        ids = [ ]
        PGAs = [ ]
        for id in latitudes:   
            ids.append(id)
            PGAs.append(PGA[id][-1])
    
        for i in argsort(PGAs)[::-1]:  
            
            id = ids[i]

            fdid = id.split('.')
            if fdid[-1][-1] == 'X' :
                fdid[-1] = fdid[-1][:2]
            if fdid[-2] == '' :
                fdid[-2] = '--'
            fdid = '.'.join(fdid[:2]+fdid[-1:]+fdid[-2:-1])

            fdtime = int(times[-1].timestamp)
            fdPGA = PGA[id][-1]*100
            fdtimePGA = int(timePGA[id][-1])
            
            file.write('\n%s %s %s %d %.5f'%(latitudes[id],
                                            longitudes[id],
                                            fdid,
                                            fdtimePGA, 
                                            fdPGA))
