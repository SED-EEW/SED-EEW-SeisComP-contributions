#!/usr/bin/env seiscomp-python
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

Author of the Software: Fred Massin
"""

import sys
import os
import lxml.etree as ET

from seiscomp import client 
from seiscomp import core
from seiscomp import datamodel
from seiscomp import io

class eewDump(client.Application):

    def __init__(self, argc, argv):
        client.Application.__init__(self, argc, argv)

        self.setMessagingEnabled(False)
        self.setDatabaseEnabled(True, False)
        self.setDaemonEnabled(False)

        self.eventID = None
        self.inputFile = None
        self.xslFile = None
        self.inputFormat = "xml"

        self.magTypes = ['MVS','Mfd']
        self.authors = ['scvsmag', 'scfinder', 'scfd' , 'scfdalp', 'scfd20as', 'scfd8']


    def createCommandLineDescription(self):
        self.commandline().addGroup("Input")
        self.commandline().addStringOption(
            "Input", "authors,a",
            "Author preferrence order (last is preferred)")
        self.commandline().addStringOption(
            "Input", "xsl,x",
            "convert using xsl")
        self.commandline().addStringOption(
            "Input", "input,i",
            "read event from XML file instead of database. Use '-' to read "
            "from stdin.")
        self.commandline().addStringOption(
            "Input", "format,f",
            "input format to use (xml [default], zxml (zipped xml), binary). "
            "Only relevant with --input.")

        self.commandline().addGroup("Dump")
        self.commandline().addStringOption("Dump", "event,E", "event id")

        return True


    def validateParameters(self):

        try:
            self.inputFile = self.commandline().optionString("input")
            self.setDatabaseEnabled(False, False)
        except BaseException:
            pass

        try:
            self.xslFile = self.commandline().optionString("xsl")
        except BaseException:
            pass

        try:
            self.authors = self.commandline().optionString("authors").split(',')
        except BaseException:
            pass

        return True


    def init(self):

        if not client.Application.init(self):
            return False

        try:
            self.inputFormat = self.commandline().optionString("format")
        except BaseException:
            pass

        try:
            self.eventID = self.commandline().optionString("event")
        except BaseException:
            if not self.inputFile:
                raise ValueError("An eventID is mandatory if no input file is "
                                 "specified")

        return True


    def printUsage(self):

        print('''Usage:
sceewdump [options]

Dump EEW parameters from a given event ID or file''')

        client.Application.printUsage(self)

        print('''Example (setup databse parameter in sceewdump.cfg):
sceewdump -E smi:ch.ethz.sed/sc20d/Event/2022njvrzz  -i ~/playback_fm4test/2022njvrzz.xml --xsl /usr/local/share/sceewlog/sc3ml_0.12__quakeml_1.2-RT_eewd.xsl -a scvs,scfdfo
''')

    def run(self):

        resolveWildcards = self.commandline().hasOption("resolve-wildcards")

        magnitudes = []
        ev=None

        # read magnitudes from input file
        if self.inputFile:
            magnitudes,ev = self.readXML()
            if not magnitudes:
                raise ValueError("Could not find magnitudes in input file")

        # read magnitudes from database
        else:
            obj = self.query().getEventByPublicID(self.eventID)
            ev = datamodel.Event.Cast(obj)
            if ev:
                origins = [datamodel.Origin.Cast(o) for o in self.query().getOrigins(self.eventID)]
            else:
                raise ValueError("event id {} not found ".format(
                    self.eventID))
            
            # collect magnitudes
            for o in origins:
                if o is None:
                    continue
                if self.query().loadMagnitudes(o):
                    for i in range(o.magnitudeCount()):
                        magnitude = datamodel.Magnitude.Cast(o.magnitude(i))
                        # Filter magnitude types
                        if magnitude.type() not in self.magTypes :
                            #print("Skipping %s"%magnitude.type(),file=sys.stdout)
                            continue
                        magnitudes.append((magnitude, o))

            if not magnitudes:
                raise ValueError("Could not find magnitudes for event {} in "
                                 "database".format(self.eventID))

        lastauthorindex = -1
        for magnitude,origin in magnitudes:

            # Decision making on author pref order
            authorOK = False
            for a,author in enumerate(self.authors):
                if a < lastauthorindex:
                    continue
                if magnitude.creationInfo().author()[:len(author)] == author:
                    lastauthorindex = a
                    authorOK = True
            if not authorOK:
                continue


            # Clean new obj
            ep = datamodel.EventParameters()
            evt = datamodel.Event.Create()

            # Clean origin copy
            org_cloned = datamodel.Origin.Cast(origin.clone())
            mag_cloned = datamodel.Magnitude.Cast(magnitude.clone())
            org_cloned.add(mag_cloned)

            # Copy event
            evt.setPublicID(ev.publicID().split('/')[-1])
            evt.add(datamodel.OriginReference(org_cloned.publicID())) #org_ref_cloned)
            evt.setPreferredOriginID(origin.publicID())
            evt.setPreferredMagnitudeID(magnitude.publicID())

            # Copy event parameters
            ep.add(evt)
            ep.add(org_cloned)

            # time in millisecond since 1970 (java ref)
            otm = core.Time.FromYearDay(1970, 1)
            #print(otm,file=sys.stdout)
            now = magnitude.creationInfo().creationTime()
            dt = (now - otm).seconds()*1000

            # Debug info
            print("./%d.sc3xml"%dt,magnitude.creationInfo().creationTime(),magnitude.type(),magnitude.magnitude().value(),magnitude.creationInfo().author(),file=sys.stdout)

            # Prints
            ar = io.XMLArchive()
            ar.create("./%d.sc3xml"%dt)
            ar.setFormattedOutput(True)
            ar.writeObject(ep)
            ar.close()

            # Converts
            if self.xslFile is not None:
                dom = ET.parse("./%d.sc3xml"%dt)
                xslt = ET.parse(self.xslFile) 
                transform = ET.XSLT(xslt)
                newdom = transform(dom)
                with open("./%d.xml"%dt, "wb") as fd:
                    fd.write(ET.tostring(newdom, pretty_print=True))
                os.remove("./%d.sc3xml"%dt)

        return True


    def readXML(self):

        if self.inputFormat == "xml":
            ar = io.XMLArchive()
        elif self.inputFormat == "zxml":
            ar = io.XMLArchive()
            ar.setCompression(True)
        elif self.inputFormat == "binary":
            ar = io.VBinaryArchive()
        else:
            raise TypeError("unknown input format '{}'".format(
                self.inputFormat))

        if not ar.open(self.inputFile):
            raise IOError("unable to open input file")

        obj = ar.readObject()
        if obj is None:
            raise TypeError("invalid input file format")

        ep = datamodel.EventParameters.Cast(obj)
        if ep is None:
            raise ValueError("no event parameters found in input file")

        # we require at least one origin which references to magnitude
        if ep.originCount() == 0:
            raise ValueError("no origin found in input file")

        originIDs = []

        # search for a specific event id
        if self.eventID:
            ev = datamodel.Event.Find(self.eventID)
            if ev:
                originIDs = [ev.originReference(i).originID() \
                             for i in range(ev.originReferenceCount())]
            else:
                raise ValueError("event id {} not found in input file".format(
                    self.eventID))

        # use first event/origin if no id was specified
        else:
            # no event, use first available origin
            if ep.eventCount() == 0:
                if ep.originCount() > 1:
                    print("WARNING: Input file contains no event but more than "
                          "1 origin. Considering only first origin",
                          file=sys.stderr)
                originIDs.append(ep.origin(0).publicID())

            # use origin references of first available event
            else:
                if ep.eventCount() > 1:
                    print("WARNING: Input file contains more than 1 event. "
                          "Considering only first event", file=sys.stderr)
                ev = ep.event(0)
                originIDs = [ev.originReference(i).originID() \
                             for i in range(ev.originReferenceCount())]

        # collect magnitudes
        magnitudes = []
        for oID in originIDs:

            o = datamodel.Origin.Find(oID)
            if o is None:
                continue

            for i in range(o.magnitudeCount()):

                magnitude = datamodel.Magnitude.Find(o.magnitude(i).publicID())

                # Filter magnitude types
                if magnitude.type() not in self.magTypes :
                    #print("Skipping %s"%magnitude.type(),file=sys.stdout)
                    continue
                
                magnitudes.append((magnitude, o))

        return magnitudes,ev


if __name__ == '__main__':
    try:
        app = eewDump(len(sys.argv), sys.argv)
        sys.exit(app())
    except (ValueError, TypeError) as e:
        print("ERROR: {}".format(e), file=sys.stderr)
    sys.exit(1)
