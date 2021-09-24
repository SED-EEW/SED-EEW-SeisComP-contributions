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
                 format='qml1.2-rt'):
        UDConnection.__init__(self, host, port, topic, username, password)
        ei = seiscomp.system.Environment.Instance()
        self.transform = None
        if format == 'qml1.2-rt':
            xslt = ET.parse(os.path.join(ei.shareDir(), 'sceewlog',
                                         'sc3ml_0.11__quakeml_1.2-RT_eewd.xsl'))
            self.transform = ET.XSLT(xslt)
        elif format == 'shakealert':
            xslt = ET.parse(os.path.join(ei.shareDir(), 'sceewlog',
                            'sc3ml_0.11__shakealert.xsl'))
            self.transform = ET.XSLT(xslt)
        elif format == 'cap1.2':
            xslt = ET.parse(os.path.join(ei.shareDir(), 'sceewlog',
                            'sc3ml_0.11__cap_1.2.xsl'))
            self.transform = ET.XSLT(xslt)
        elif format == 'sc3ml':
            pass
        else:
            seiscomp.logging.error('Currently supported AMQ message formats \
            are cap1.2, sc3ml, qml1.2-rt, and shakealert.')

    def message_encoder(self, ep, pretty_print=True):
        exp = Exporter.Create('trunk')
        io = BytesIO() 
        sink = Sink(io)
        exp.write(sink, ep)
        # apply XSLT
        dom = ET.fromstring(io.getvalue())
        if self.transform is not None:
            dom = self.transform(dom)
        return ET.tostring(dom, pretty_print=pretty_print)

if __name__ == '__main__':
    pass
