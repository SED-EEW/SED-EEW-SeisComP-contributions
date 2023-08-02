#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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

Test:
    ./sceewlog2file.py magID=mag/42 type=Meew author=author42 magnitude=4.2 lat=10 lon=-88 depth=4.2 nstorg=4 nstmag=2 ts=2023-01-01T04:02:42.4200 tsobject=2023-01-01T04:02:42.4200 diff=4.2 ot=2023-01-01T04:02:42.4200 eew=1

Author of the Software: Fred Massin
"""
eewFile = '/tmp/.tweet.json'
local_tz = 'America/El_Salvador'
lalo2textbnaFile = '/opt/seiscomp/share/sceewlog/EEW_reference.bna' 
format='{"text": "#ElObservatorioInforma datos preliminares del sismo sentido:\\nMagnitud preliminar: %.1f\\nUbicación aproximada: %s [%s]\\nFecha y hora: %s\\nEsta información será actualizada en breve con los datos revisados del sismo."}'

from sys import argv, exit
args = {arg.split('=')[0]:arg.split('=')[-1] for arg in argv}
for k in args:
    try:
        args[k]=float(args[k])
    except Exception as e:
        pass # print("Cannot convert arg: %s" % repr(e))
"""
{'magID' = magID,
 'type' = mag.type()
 'author' = mag.creationInfo().author()
 'magnitude' = mag.magnitude().value()
 'lat' = org.latitude().value()
 'lon' = org.longitude().value()
 'depth' = org.depth().value()
 'nstorg' = org.arrivalCount()
 'nstmag' = str(mag.stationCount()) OR ''
 'ts' = mag.creationInfo().modificationTime().toString("%FT%T.%2fZ")
 'tsobject' = mag.creationInfo().modificationTime() OR mag.creationInfo().creationTime()
 'diff' =  mag.creationInfo().modificationTime() - org.time().value().length() OR mag.creationInfo().creationTime() - org.time().value()
 'ot' = org.time().value().toString("%FT%T.%2fZ")
 'eew' =  False OR 1..n
}
"""

import datetime
try:
    from dateutil import tz
except:
    print('apt-get install python3-dateutil')
    exit(-1)
    
def utc2local(dt, local_tz):
    dat = datetime.datetime.strptime(dt[:19],"%Y-%m-%dT%H:%M:%S")
    dat.replace(tzinfo = tz.gettz('UTC'))
    datloc = dat.astimezone(tz.gettz(local_tz))
    print(dat,'>',datloc)
    return datloc.strftime("%Y-%m-%d | %H:%M:%S")

import seiscomp.geo
fslalo2text = seiscomp.geo.GeoFeatureSet()
fo = fslalo2text.readBNAFile(lalo2textbnaFile, None)
if not fo:
    print('Not possible to open the lalo2textbnaFile %s'%lalo2textbnaFile)
    exit(-1)
else:
    print('Successfuly read the lalo2textbnaFile %s'%lalo2textbnaFile)

def lalo2text(la,lo):
    la = float(la)
    lo = float(lo)
    lalo = seiscomp.geo.GeoCoordinate(la,lo)
    for i,x in enumerate(fslalo2text.features()):
        if fslalo2text.features()[i].contains( lalo ):
            print('lat: %s and lon: %s are within polygon: %s' % ( la,
                                    lo, fslalo2text.features()[i].name() ) )
            return fslalo2text.features()[i].name()
    return '%.2fN|%.2fE'%(la,lo)

def write2file(format='', data=()):
    if eewFile is not None:
        with open(eewFile, "w", encoding='utf-8') as writer:
            try:
                writer.write(format % data)
                print("Succesfully wrote to file: %s" % eewFile)
            except Exception as e:
                print("Cannot write to file: %s" % repr(e))


if 'test' in args and args['test']:
    print(format)
    exit()
    
data=(args['magnitude'], 
      lalo2text(args['lat'],
                args['lon']), 
      '%.2fN,%.2fO'%(float(args['lat']),
                     float(args['lon'])), 
      utc2local(args['ot'],
                local_tz))

write2file(format=format,
           data=data)