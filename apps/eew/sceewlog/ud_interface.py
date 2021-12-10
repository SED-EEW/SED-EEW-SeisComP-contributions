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

from stomp import Connection 
from io import BytesIO
from twisted.internet import reactor
import datetime
import time
import os
import lxml.etree as ET
import seiscomp
from seiscomp.io import Exporter, ExportSink


class UDException(Exception): pass


b_str = lambda s: s.encode('utf-8', 'replace')


class Sink(ExportSink):
    def __init__(self, buf):
        ExportSink.__init__(self)
        self.buf = buf
        self.written = 0

    def write(self, data, size):
        self.buf.write(b_str(data[:size]))
        self.written += size
        return size


class UDConnection:

    def __init__(self, host=None, port=None, topic=None, username=None,
                 password=None):
        try:
            self.topic = topic
            self.username = username
            self.password = password
            self.host = host
            self.port = port
            self.stomp = Connection([(self.host, self.port)]) 
            self.stomp.connect(self.username, self.password, wait=True) 
        except Exception as e:
            raise UDException('Cannot connect to message broker (%s@%s:%d): %s.'\
                               % (username, host, port, e))

    def send(self, msg):
        try:
            self.stomp.send(self.topic, msg) 
        except Exception as e:
            seiscomp.logging.error("ActiveMQ connection lost.")
            # Wait for a bit in case the ActiveMQ broker is restarting
            time.sleep(10)
            try:
                del self.stomp
                self.stomp = Connection([(self.host, self.port)]) 
                self.stomp.connect(self.username, self.password, wait=True) 
            except Exception as e:
                raise UDException('Cannot reconnect to server: %s' % e)
            seiscomp.logging.info('Connection re-established.')


class HeartBeat(UDConnection):

    def __init__(self, host, port, topic, username, password,
                 format='qml1.2-rt'):
        UDConnection.__init__(self, host, port, topic, username, password)
        self.format = format

    def send_hb(self):
        dt = datetime.datetime.utcnow()
        root = ET.Element('hb')
        root.set('originator', 'vssc3')
        root.set('sender', 'vssc3')
        if self.format in ['qml1.2-rt', 'cap1.2']:
            now = dt.strftime('%Y-%m-%dT%H:%M:%S.%fZ')
            root.set('xmlns', 'http://heartbeat.reakteu.org')
        else:
            now = dt.strftime('%a %B %d %H:%M:%S %Y')
        root.set('timestamp', now)
        tree = ET.ElementTree(root)
        f = BytesIO() 
        tree.write(f, encoding="UTF-8", xml_declaration=True, method='xml')
        msg = f.getvalue()
        self.send(msg)
        return msg


class CoreEventInfo(UDConnection):

    def __init__(self, host, port, topic, username, password,
                 change_headline, language, worldCitiesFile, format='qml1.2-rt' ):
        UDConnection.__init__(self, host, port, topic, username, password)
        ei = seiscomp.system.Environment.Instance()
        
        self.transform = None
        #this is the path to the world cities file
        self.changeHeadline = change_headline
        self.hlLangCities = language
        self.hlWorldCitiesFile = worldCitiesFile
        self.hlalert = None
        self.dic = None 
        
        if format == 'qml1.2-rt':
            xslt = ET.parse(os.path.join(ei.shareDir(), 'sceewlog',
                                         'sc3ml_0.11__quakeml_1.2-RT_eewd.xsl'))
            self.transform = ET.XSLT(xslt)
        elif format == 'shakealert':
            xslt = ET.parse(os.path.join(ei.shareDir(), 'sceewlog',
                            'sc3ml_0.11__shakealert.xsl'))
            self.transform = ET.XSLT(xslt)
        elif format == 'cap1.2':
            try:
                if self.changeHeadline and self.hlWorldCitiesFile:
                    from headlinealert import HeadlineAlert as hl
                    self.hlalert = hl(self.hlWorldCitiesFile)
                    seiscomp.logging.info("loading cities file: %s" % self.hlWorldCitiesFile)
                    self.dic = self.hlalert.csvFile2dic(self.hlalert.dataFile)
                    seiscomp.logging.info('The number of cities in this file is %s' % len(self.dic))
            except:
                seiscomp.logging.warning('Not possible to load the world cities file for language: %s' % self.hlLangCities)
                seiscomp.logging.warning('alert messages in cap1.2 format will present default headline strings.')
                pass
                
            xslt = ET.parse(os.path.join(ei.shareDir(), 'sceewlog',
                            'sc3ml_0.11__cap_1.2.xsl'))
            
            self.transform = ET.XSLT(xslt)
            
        elif format == 'sc3ml':
            pass
        else:
            seiscomp.logging.error('Currently supported AMQ message formats \
            are cap1.2, sc3ml, qml1.2-rt, and shakealert.')

    def modify_headline(self, ep, dom):
        #headline for alert msg
        hlSpanish = None
        hlEnglish = None
        try:
            evt = ep.event(0)
            agency = evt.creationInfo().agencyID()
            prefOrID = evt.preferredOriginID()
            prefMagID = evt.preferredMagnitudeID()
            origin = ep.findOrigin(prefOrID)
            
            mag = origin.findMagnitude(prefMagID).magnitude().value()
            lat = origin.latitude().value()
            lon = origin.longitude().value()
            depth = origin.depth().value()
            
            if self.hlalert is not None and self.dic is not None:
                
                np = dis = azVal = azTextSp = azTextEn = None
                seiscomp.logging.info('finding the nearest place to the epicenter:')
                seiscomp.logging.info('lat %s lon %s' % (lat, lon))
                epi = {'lat': lat,'lon': lon}
                #nearest city or place from self.dic (dictionary)
                np = self.hlalert.findNearestPlace(self.dic, epi)
                seiscomp.logging.info('the nearest place is %s' % np)
                #distance to the nearest place
                dis = self.hlalert.distance([ float(np['lat']), epi['lat'], float(np['lon']), epi['lon'] ] )
                seiscomp.logging.info('distance in km is: %s'%dis)
                #azimuth from the nearest place to the epicenter
                azVal = self.hlalert.azimuth([ float(np['lat']), epi['lat'], float(np['lon']), epi['lon'] ] )
                seiscomp.logging.info('azimuth %s'%azVal)
                #azimuth in abbreviated way of cardianal direction
                azTextSp = self.hlalert.direction(azVal, 'es-US')
                seiscomp.logging.info('Abreviated Cardinal Direction in Spanish: %s' % azTextSp)
                azTextEn = self.hlalert.direction(azVal, 'en-US')
                seiscomp.logging.info('Abreviated Cardinal Direction in English: %s' % azTextEn)
                #Location string text based on distance, direction, city name, country name, language
                if self.hlLangCities == 'es-US':
                    location = self.hlalert.location(dis, azTextSp, np['city'],np['country'], self.hlLangCities)
                else:
                    location = self.hlalert.location(dis, azTextEn, np['city'],np['country'], self.hlLangCities)
                seiscomp.logging.info('Location: %s' % location)
                region = self.hlalert.region( epi['lat'], epi['lon'] )
                seiscomp.logging.info('Region: %s' % region)
                #
                if self.hlLangCities == 'es-US':
                    hlSpanish = agency + '/Sismo - Magnitud '+str( round(mag, 1) )+ ', '+location
                    hlEnglish = agency + '/Earthquake - Magnitude '+str( round(mag, 1) )+ ', ' + region
                else:
                    hlSpanish = agency + '/Sismo - Magnitud '+str( round(mag, 1) )+ ', '+region
                    hlEnglish = agency + '/Earthquake - Magnitude '+str( round(mag, 1) )+ ', ' + location
                
                seiscomp.logging.info('new HL Spanish: %s' % hlSpanish)
+               seiscomp.logging.info('new HL English: %s' % hlEnglish)   
                
                if hlSpanish is not None:
                    seiscomp.logging.info('Replacing in Spanish')
                    dom = self.hlalert.replaceHeadline(hlSpanish, 'es-US',dom)
                
                if hlEnglish is not None:
                    seiscomp.logging.info('Replacing in English')
                    dom = self.hlalert.replaceHeadline(hlEnglish, 'en-US',dom)
        except Exception as e:
            seiscomp.logging.warning('There was an error while collecting information to change the headline')
            seiscomp.logging.warning(repr(e))
            #Something went wrong. Returning the same dom 
            return dom
        
        return dom
        
    def message_encoder(self, ep, pretty_print=True):
        exp = Exporter.Create('trunk')
        io = BytesIO() 
        sink = Sink(io)
        exp.write(sink, ep)
        dom = ET.fromstring(io.getvalue())
        # apply XSLT
        if self.transform is not None:
            dom = self.transform(dom)
            
            #replacing the headline in spanish and english
            if self.changeHeadline and self.hlalert and self.dic:
                seiscomp.logging.info('modifying the headline for CAP1.2 alert message')
                dom = self.modify_headline(ep, dom)
                
        return ET.tostring(dom, encoding='utf8', pretty_print=pretty_print)

if __name__ == '__main__':
    pass
