#!/usr/bin/env python
"""
Copyright
---------
This file is part of the Virtual Seismologist (VS) software package.
VS is free software: you can redistribute it and/or modify it under
the terms of the "SED Public License for Seiscomp Contributions"

VS is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the SED Public License for Seiscomp
Contributions for more details.

You should have received a copy of the SED Public License for Seiscomp
Contributions with VS. If not, you can find it at
http://www.seismo.ethz.ch/static/seiscomp_contrib/license.txt

Author of the Software: Yannik Behr, Fred Massin, Luca Scarabello
Copyright (C) 2006-2018 by Swiss Seismological Service
"""

import sys, traceback, seiscomp3.Client
from seiscomp3.DataModel import PublicObjectTimeSpanBuffer, EventParameters
from seiscomp3.Core import TimeSpan, Time
from seiscomp3 import Config, System, Utils
import smtplib
from email.mime.text import MIMEText
import os
import datetime
from dateutil import tz
import random


class Listener(seiscomp3.Client.Application):

    def __init__(self, argc, argv):
        seiscomp3.Client.Application.__init__(self, argc, argv)
        self.setMessagingEnabled(True)
        self.setDatabaseEnabled(True, True)
        self.setPrimaryMessagingGroup(seiscomp3.Communication.Protocol.LISTENER_GROUP)
        self.addMessagingSubscription("LOCATION")
        self.addMessagingSubscription("MAGNITUDE")
        self.addMessagingSubscription("PICK")
        self.addMessagingSubscription("EVENT")

        self.cache = PublicObjectTimeSpanBuffer()
        self.expirationtime = 3600.
        self.event_dict = {}
        self.origin_lookup = {}
        self.event_lookup = {}
        self.received_comments = []
        self.latest_event = Time.Null
        # report settings
        self.storeReport=False
        self.ei = System.Environment.Instance()
        self.report_head =  "                                                                   |#St.   |                                                              \n"
        self.report_head += "Tdiff |Type|Mag.|Lat.  |Lon.   |Depth |origin time (UTC)      |Lik.|Or.|Ma.|Str.|Len. |Author   |Creation t.            |Tdiff(current o.)\n"
        self.report_head += "-"*int(len(self.report_head)/2-1) + "\n"
        self.report_directory = os.path.join(self.ei.logDir(), 'EEW_reports')
        # email settings
        self.sendemail = True
        self.smtp_server = None
        self.email_port = None
        self.hostname = None
        self.tls = False
        self.ssl = False
        self.credentials = None
        self.email_sender = None
        self.email_recipients = None
        self.email_subject = None
        self.username = None
        self.password = None
        self.auth = False
        self.magThresh = 0.0
        self.magTypes = ['MVS','Mfd']
        self.generateReportTimeout = 5 # seconds
        self.hb = None
        self.hb_seconds = None
        # UserDisplay interface
        self.udevt = None

    def validateParameters(self):
        try:
            if seiscomp3.Client.Application.validateParameters(self) == False:
                return False
            if self.commandline().hasOption('savedir'):
                self.report_directory = self.commandline().optionString('savedir')
            if self.commandline().hasOption('playback'):
                self.setDatabaseEnabled(False, False)
            return True
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info: sys.stderr.write(i)
            return False

    def createCommandLineDescription(self):
        """
        Setup command line options.
        """
        try:
            self.commandline().addGroup("Reports")
            self.commandline().addStringOption("Reports", "savedir", "Directory to save reports to.")
            self.commandline().addGroup("Mode")
            self.commandline().addOption("Mode", "playback", "Disable database connection.")
        except:
            seiscomp3.Logging.warning("caught unexpected error %s" % sys.exc_info())
            info = traceback.format_exception(*sys.exc_info())
            for i in info: sys.stderr.write(i)
        return True

    def initConfiguration(self):
        """
        Read configuration file.
        """
        if not seiscomp3.Client.Application.initConfiguration(self):
            return False

        try:
            self.magTypes = self.configGetStrings("magTypes")
            self.generateReportTimeout = self.configGetInt("generateReportTimeout")
        except Exception, e:
            pass

        try:
            self.sendemail = self.configGetBool("email.activate")
            self.smtp_server = self.configGetString("email.smtpserver")
            self.email_port = self.configGetInt("email.port")
            self.tls = self.configGetBool("email.usetls")
            self.ssl = self.configGetBool("email.usessl")
            if self.tls and self.ssl:
                seiscomp.Logging.warning('TLS and SSL cannot be both true!')
                self.tls = False
                self.ssl = False
            self.auth = self.configGetBool("email.authenticate")
            if (self.auth):
                cf = seiscomp3.Config.Config()
                cf.readConfig(self.configGetString("email.credentials"))
                self.username = cf.getString('username')
                self.password = cf.getString('password')
            self.email_sender = self.configGetString("email.senderaddress")
            self.email_recipients = self.configGetStrings("email.recipients")
            self.email_subject = self.configGetString("email.subject")
            self.hostname = self.configGetString("email.host")
            self.magThresh = self.configGetDouble("email.magThresh")
        except Exception, e:
            seiscomp3.Logging.info('Some configuration parameters could not be read: %s' % e)
            self.sendemail = False

        try:
            self.amqHost = self.configGetString('ActiveMQ.hostname')
            self.amqPort = self.configGetInt('ActiveMQ.port')
            self.amqTopic = self.configGetString('ActiveMQ.topic')
            self.amqHbTopic = self.configGetString('ActiveMQ.hbtopic')
            self.amqUser = self.configGetString('ActiveMQ.username')
            self.amqPwd = self.configGetString('ActiveMQ.password')
            self.amqMsgFormat = self.configGetString('ActiveMQ.messageFormat')
        except:
            pass

        try:
            self.storeReport = self.configGetBool("report.activate")
            self.expirationtime = self.configGetDouble("report.eventbuffer")
            self.report_directory = self.ei.absolutePath(self.configGetString("report.directory"))
        except:
            pass

        return True

    def init(self):
        """
        Initialization.
        """
        if not seiscomp3.Client.Application.init(self):
            return False

        seiscomp3.Logging.info("Listening to magnitude types %s" % str(self.magTypes))

        if not self.sendemail:
            seiscomp3.Logging.info('Sending email has been disabled.')
        else:
            self.sendMail({}, '', test=True)

        if self.storeReport:
            seiscomp3.Logging.info("Reports are stored in %s" % self.report_directory)
        else:
            seiscomp3.Logging.info("Saving reports to disk has been DISABLED!")

        self.cache.setTimeSpan(TimeSpan(self.expirationtime))
        if self.isDatabaseEnabled():
            self.cache.setDatabaseArchive(self.query());
            seiscomp3.Logging.info('Cache connected to database.')
        self.enableTimer(1)
        try:
            import ud_interface
            self.udevt = ud_interface.CoreEventInfo(self.amqHost, self.amqPort,
                                                    self.amqTopic, self.amqUser,
                                                    self.amqPwd,
                                                    self.amqMsgFormat)
            self.hb = ud_interface.HeartBeat(self.amqHost, self.amqPort,
                                             self.amqHbTopic, self.amqUser,
                                             self.amqPwd, self.amqMsgFormat)
            self.hb_seconds = 1
            seiscomp3.Logging.info('ActiveMQ interface is running.')
        except Exception, e:
            seiscomp3.Logging.warning('ActiveMQ interface cannot be loaded: %s' % e)
        return True

    def generateReport(self, evID):
        """
        Generate a report for an event, write it to disk and optionally send
        it as an email.
        """
        seiscomp3.Logging.info("Generating report for event %s " % evID)
 
	prefindex = sorted(self.event_dict[evID]['updates'].keys())[-1] 
        sout = self.report_head
        threshold_exceeded = False
        self.event_dict[evID]['diff'] = 9999
        for _i in sorted(self.event_dict[evID]['updates'].keys()):
            ed = self.event_dict[evID]['updates'][_i]
            mag = ed['magnitude']
            if mag > self.magThresh:
                threshold_exceeded = True

            difftime = ed['tsobject'] - \
            self.event_dict[evID]['updates'][prefindex]['tsobject'] 
            ed['difftopref'] = difftime.length() 
            ed['difftopref'] += self.event_dict[evID]['updates'][prefindex]['diff']
            sout += "%6.2f|" % ed['difftopref']
 
            sout += "%4s|"   % ed['type']
            sout += "%4.2f|" % mag
            sout += "%6.2f|" % ed['lat']
            sout += "%7.2f|" % ed['lon']
            sout += "%6.2f|" % ed['depth']
            sout += "%s|" % ed['ot']
            if 'likelihood' in ed:
                sout += "%4.2f|" % ed['likelihood'] 
            else:
                sout += "    |"
            sout += "%3d|" % ed['nstorg']
            sout += "%3s|" % ed['nstmag']
            if 'rupture-strike' in ed:
                sout += "%4d|" % ed['rupture-strike']
            else:
                sout += "    |"
            if 'rupture-length' in ed:
                sout += "%5.2f|" % ed['rupture-length']
            else:
                sout += "     |"
            sout += "%9s|" % ed['author'][:9]
            sout += "%s|" % ed['ts']
            sout += "%6.2f\n" % ed['diff']
 
            if ed['difftopref']< self.event_dict[evID]['diff']:
                self.event_dict[evID]['diff'] = ed['difftopref']

        if self.storeReport:
            self.event_dict[evID]['report'] = sout
            if not os.path.isdir(self.report_directory):
                os.makedirs(self.report_directory)
            f = open(os.path.join(self.report_directory,
                                  '%s_report.txt' % evID.replace('/', '_')), 'w')
            f.writelines(self.event_dict[evID]['report'])
            f.close()
        self.event_dict[evID]['type'] = ed['type']
        self.event_dict[evID]['magnitude'] = ed['magnitude']
        seiscomp3.Logging.info("\n" + sout)
        if self.sendemail and threshold_exceeded:
            self.sendMail(self.event_dict[evID], evID)
        self.event_dict[evID]['published'] = True

    def handleTimeout(self):
        self.checkExpiredTimers()
        # send heartbeat every 5 seconds
        if self.hb_seconds is not None:
            self.hb_seconds -= 1
            if self.hb_seconds <= 0:
                self.hb.send_hb()
                self.hb_seconds = 5

    def checkExpiredTimers(self):
        for evID, evDict in self.event_dict.iteritems():
            timer = evDict['report_timer']
            if not timer.isActive():
                continue
            seiscomp3.Logging.debug("There is an active timer for event %s (%s sec elapsed) "
                    % (evID, timer.elapsed().seconds()))
            if timer.elapsed().seconds() > self.generateReportTimeout:
                self.generateReport(evID)
                timer.reset()

    def setupGenerateReportTimer(self, magID):
        """
        Gather all the information required to generate a report and set up the 
        generateReport timer. If no other magnitudes are received before the
        timer expires, a report is generated
        """
        seiscomp3.Logging.debug("Start report generation timer for magnitude %s " % magID)
        orgID = self.origin_lookup[magID]
        if orgID not in self.event_lookup:
            seiscomp3.Logging.debug("Event not received yet for magnitude %s (setupGenerateReportTimer)" % magID)
            return
        evID = self.event_lookup[orgID]
        org = self.cache.get(seiscomp3.DataModel.Origin, orgID)
        if not org:
            # error messages
            not_in_cache = "Object %s not found in cache!\n" % orgID
            not_in_cache += "Is the cache size big enough?\n"
            not_in_cache += "Have you subscribed to all necessary message groups?"
            seiscomp3.Logging.warning(not_in_cache)
            return
        mag = self.cache.get(seiscomp3.DataModel.Magnitude, magID)
        if not mag:
            # error messages
            not_in_cache = "Object %s not found in cache!\n" % magID
            not_in_cache += "Is the cache size big enough?\n"
            not_in_cache += "Have you subscribed to all necessary message groups?"
            seiscomp3.Logging.warning(not_in_cache)
            return

        # use modification time if available, otherwise go for creation time
        try:
            updateno = mag.creationInfo().modificationTime().iso() # e.g. "1970-01-01T00:00:00.0000Z"
        except:
            updateno = mag.creationInfo().creationTime().iso() # e.g. "1970-01-01T00:00:00.0000Z"

        if updateno in self.event_dict[evID]['updates'].keys():
            # error messages
            err_msg = "Magnitude (%s) has the same creation/modifcaton time of an " % magID
            err_msg += "already received magnitude (%s)! Ignoring it.\n" %self.event_dict[evID]['updates'][updateno]['magID']
            seiscomp3.Logging.warning(err_msg)
            return

        # Check if the report has been already generated
        if self.event_dict[evID]['published']:
            # error messages
            err_msg = "A report has been already generated for magnitude %s." % magID
            err_msg += "This probably means the report timer expired before"
            err_msg += "this new magnitude was received by sceewlog."
            seiscomp3.Logging.error(err_msg)
            return

        # stop the generateReport timer if that was started already by a previous magnitude
        timer = self.event_dict[evID]['report_timer']
        if timer.isActive():
            timer.reset()

        self.event_dict[evID]['updates'][updateno] = {}
        self.event_dict[evID]['updates'][updateno]['magID'] = magID
        self.event_dict[evID]['updates'][updateno]['type'] = mag.type()
        self.event_dict[evID]['updates'][updateno]['author'] = mag.creationInfo().author()
        self.event_dict[evID]['updates'][updateno]['magnitude'] = mag.magnitude().value()
        self.event_dict[evID]['updates'][updateno]['lat'] = org.latitude().value()
        self.event_dict[evID]['updates'][updateno]['lon'] = org.longitude().value()
        self.event_dict[evID]['updates'][updateno]['depth'] = org.depth().value()
        self.event_dict[evID]['updates'][updateno]['nstorg'] = org.arrivalCount()
        try:
            self.event_dict[evID]['updates'][updateno]['nstmag'] = str(mag.stationCount())
        except:
            self.event_dict[evID]['updates'][updateno]['nstmag'] = ''
        try:
            self.event_dict[evID]['updates'][updateno]['ts'] = \
            mag.creationInfo().modificationTime().toString("%FT%T.%2fZ")
            difftime = mag.creationInfo().modificationTime() - org.time().value()
            reftime = mag.creationInfo().modificationTime()
        except:
            self.event_dict[evID]['updates'][updateno]['ts'] = \
            mag.creationInfo().creationTime().toString("%FT%T.%2fZ")
            difftime = mag.creationInfo().creationTime() - org.time().value()
            reftime = mag.creationInfo().creationTime()
        self.event_dict[evID]['updates'][updateno]['tsobject'] = reftime
        self.event_dict[evID]['updates'][updateno]['diff'] = difftime.length()
        self.event_dict[evID]['updates'][updateno]['ot'] = \
        org.time().value().toString("%FT%T.%2fZ")

        seiscomp3.Logging.info("Number of updates %d for event %s" % (len(self.event_dict[evID]['updates']), evID))
        seiscomp3.Logging.info("lat: %f; lon: %f; mag: %f; ot: %s" % \
                               (org.latitude().value(),
                                org.longitude().value(),
                                mag.magnitude().value(),
                                org.time().value().toString("%FT%T.%4fZ")))

        # Start generateReport timer
        timer.restart()

        # Make sure to attached additional information for this ev/mag
        self.processComments()

        # Send an alert
        self.sendAlert(magID)

    def sendAlert(self, magID):
        """
        Send an alert to a UserDisplay, if one is configured
        """
        if self.udevt is None:
            return

        seiscomp3.Logging.debug("Sending an alert for magnitude %s " % magID)
        orgID = self.origin_lookup[magID]
        evID = self.event_lookup[orgID]

        # if there are not updates yet, return
        if not self.event_dict[evID]['updates']:
            seiscomp3.Logging.debug("Cannot send alert, no updates for ev %s (mag %s) " % (evID,magID))
            return

        ep = EventParameters()
        evt = self.cache.get(seiscomp3.DataModel.Event, evID)
        if evt:
            ep.add(evt)
        else:
            seiscomp3.Logging.debug("Cannot find event %s in cache." % evID)
        org = self.cache.get(seiscomp3.DataModel.Origin, orgID)
        if org:
            ep.add(org)
            for _ia in xrange(org.arrivalCount()):
                pk = self.cache.get(seiscomp3.DataModel.Pick, org.arrival(_ia).pickID())
                if not pk:
                    seiscomp3.Logging.debug("Cannot find pick %s in cache." % org.arrival(_ia).pickID())
                else:
                    ep.add(pk)
        else:
            seiscomp3.Logging.debug("Cannot find origin %s in cache." % orgID)

        self.udevt.send(self.udevt.message_encoder(ep))

    def handleMagnitude(self, magnitude, parentID):
        """
        Generate an origin->magnitude lookup table.
        """
        try:
            if magnitude.type() in self.magTypes:
                seiscomp3.Logging.debug("Received %s magnitude for origin %s" % (magnitude.type(),parentID))
                self.origin_lookup[magnitude.publicID()] = parentID
                self.setupGenerateReportTimer(magnitude.publicID())
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info: sys.stderr.write(i)

    def handleOrigin(self, org, parentID):
        """
        Add origins to the cache.
        """
        try:
            seiscomp3.Logging.debug("Received origin %s" % org.publicID())
            self.cache.feed(org)
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info: sys.stderr.write(i)

    def handlePick(self, pk, parentID):
        """
        Add picks to the cache.
        """
        try:
            seiscomp3.Logging.debug("Received pick %s" % pk.publicID())
            self.cache.feed(pk)
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info: sys.stderr.write(i)

    def garbageCollector(self):
        """
        Remove outdated events from the event dictionary.
        """
        eventsRemoved = set()
        tcutoff = self.latest_event - TimeSpan(self.expirationtime)
        for evID in self.event_dict.keys():
            if self.event_dict[evID]['timestamp'] < tcutoff:
                self.event_dict.pop(evID)
                eventsRemoved.add(evID)
                seiscomp3.Logging.debug("Expired event %s" % evID)

        originsRemoved = set()
        if eventsRemoved:
            for _orgID, _evID in self.event_lookup.items():
                if _evID in eventsRemoved:
                    self.event_lookup.pop(_orgID)
                    originsRemoved.add(_orgID)
                    seiscomp3.Logging.debug("Expired origin %s (ev %s)" % (_orgID, _evID))

        if originsRemoved:
            for _magID, _orgID in self.origin_lookup.items():
                if _orgID in originsRemoved:
                    self.origin_lookup.pop(_magID)
                    seiscomp3.Logging.debug("Expired mag %s (org %s)" % (_magID, _orgID))

    def handleEvent(self, evt):
        """
        Add events to the event dictionary and generate an
        event->origin lookup table.
        """
        evID = evt.publicID()
        seiscomp3.Logging.debug("Received event %s" % evID)

        for i in range(evt.originReferenceCount()):
            originID = evt.originReference(i).originID()
            self.event_lookup[originID] = evID
            seiscomp3.Logging.debug("originId %s -> event %s" % (originID, evID))

            if evID not in self.event_dict.keys():
                self.event_dict[evID] = {}
                self.event_dict[evID]['published'] = False
                self.event_dict[evID]['updates'] = {}
                try:
                    self.event_dict[evID]['timestamp'] = \
                    evt.creationInfo().modificationTime()
                except:
                    self.event_dict[evID]['timestamp'] = \
                    evt.creationInfo().creationTime()
                if self.event_dict[evID]['timestamp'] > self.latest_event:
                    self.latest_event = self.event_dict[evID]['timestamp']
                self.event_dict[evID]['report_timer'] = Utils.StopWatch(False)

        # check if we have already received magnitudes for this event,
        # if so try to generate a report
        for _orgID, _evID in self.event_lookup.items():
            if evID == _evID:
                for _magID, _orgID2 in self.origin_lookup.items():
                    if _orgID == _orgID2:
                        self.setupGenerateReportTimer(_magID)
        # delete old events
        self.garbageCollector()

    def sendMail(self, evt, evID, test=False):
        """
        Email reports.
        """
        if test:
            msg = MIMEText('sceewlog was started.')
            msg['Subject'] = 'sceewlog startup message'
        else:
            msg = MIMEText(evt['report']) 
            subject = self.email_subject
            subject += ' / %s%.2f' % (evt['type'],evt['magnitude'])
            subject += ' / %.2fs' % evt['diff'] 
            subject += ' / %s'   % self.hostname
            subject += ' / %s'   % evID
            msg['Subject'] = subject
        msg['From'] = self.email_sender
        msg['To'] = self.email_recipients[0]
        utc_date = datetime.datetime.utcnow()
        utc_date.replace(tzinfo=tz.gettz('UTC'))
        msg['Date'] = utc_date.strftime("%a, %d %b %Y %T %z")
        try:
            if self.ssl:
                s = smtplib.SMTP_SSL(host=self.smtp_server,
                                     port=self.email_port, timeout=5)
            else:
                s = smtplib.SMTP(host=self.smtp_server, port=self.email_port,
                                 timeout=5)
        except Exception, e:
            seiscomp3.Logging.warning('Cannot connect to smtp server: %s' % e)
            return
        try:
            if self.tls:
                s.starttls()
            if self.auth:
                s.login(self.username, self.password)
            s.sendmail(self.email_sender, self.email_recipients, msg.as_string())
        except Exception, e:
            seiscomp3.Logging.warning('Email could not be sent: %s' % e)
        s.quit()

    def processComments(self):
        """
        Process received comments not yet associated to the corresponding event
        """
        comments_to_keep = []

        for comment, parentID in self.received_comments:

            seiscomp3.Logging.debug("Processing comment %s for magnitude %s " \
                    % (comment.id(), parentID))

            if comment.id() in ('likelihood','rupture-strike','rupture-length'):
                magID = parentID
                if magID not in self.origin_lookup:
                    # comment belonging to an expired magnitude/event
                    continue

                orgID = self.origin_lookup[magID]
                if orgID not in self.event_lookup:
                    comments_to_keep.append( (comment, parentID) )
                    seiscomp3.Logging.debug("Event not received yet for magnitude %s \
                            origin %s(handleComment)" % (magID,orgID))
                    continue

                evID = self.event_lookup[orgID]
                if comment.id() == 'likelihood':
                    evt = self.cache.get(seiscomp3.DataModel.Event, evID)
                    if evt:
                        evt.setPreferredMagnitudeID(magID)
                    else:
                        seiscomp3.Logging.debug("Cannot find event %s in cache." % evID)

                # Attach the likelihood to the right update
                updateno = None
                for _updateno, _update_dict in self.event_dict[evID]['updates'].iteritems():
                    if magID != _update_dict['magID']:
                        continue
                    if updateno is None:
                        updateno = _updateno
                        continue
                    msg = 'Found multiple updates with magID %s for the same' % magID
                    msg += '%s comment. Choosing the most recent one' % comment.id()
                    seiscomp3.Logging.warning(msg)
                    if updateno < _updateno:
                        updateno = _updateno

                if updateno is None:
                    comments_to_keep.append( (comment, parentID) )
                    msg = 'Could not find parent magnitude %s for %s comment' \
                            % (magID, comment.id())
                    seiscomp3.Logging.error(msg)
                    continue

                if comment.id() == 'likelihood':
                    self.event_dict[evID]['updates'][updateno]['likelihood'] = \
                            float(comment.text())
                elif comment.id() == 'rupture-strike':
                    self.event_dict[evID]['updates'][updateno]['rupture-strike'] = \
                            float(comment.text())
                elif comment.id() == 'rupture-length':
                    self.event_dict[evID]['updates'][updateno]['rupture-length'] = \
                            float(comment.text())

        self.received_comments = comments_to_keep

    def handleComment(self, comment, parentID):
        """
        Update events based on special magnitude comments.
        """
        if comment.id() in ('likelihood','rupture-strike','rupture-length'):
            seiscomp3.Logging.debug("%s comment received for magnitude %s " % (comment.id(), parentID))
            self.received_comments.append( (comment, parentID) )
            self.processComments()

    def updateObject(self, parentID, object):
        """
        Call-back function if an object is updated.
        """
        pk = seiscomp3.DataModel.Pick.Cast(object)
        if pk:
            self.handlePick(pk, parentID)

        mag = seiscomp3.DataModel.Magnitude.Cast(object)
        if mag:
            self.handleMagnitude(mag, parentID)

        event = seiscomp3.DataModel.Event.Cast(object)
        if event:
            evt = self.cache.get(seiscomp3.DataModel.Event, event.publicID())
            self.handleEvent(evt)

        comment = seiscomp3.DataModel.Comment.Cast(object)
        if comment:
            self.handleComment(comment, parentID)

    def addObject(self, parentID, object):
        """
        Call-back function if a new object is received.
        """
        pk = seiscomp3.DataModel.Pick.Cast(object)
        if pk:
            self.handlePick(pk, parentID)

        mag = seiscomp3.DataModel.Magnitude.Cast(object)
        if mag:
            self.cache.feed(mag)
            self.handleMagnitude(mag, parentID)

        origin = seiscomp3.DataModel.Origin.Cast(object)
        if origin:
            self.handleOrigin(origin, parentID)

        event = seiscomp3.DataModel.Event.Cast(object)
        if event:
            self.cache.feed(event)
            self.handleEvent(event)

        comment = seiscomp3.DataModel.Comment.Cast(object)
        if comment:
            self.handleComment(comment, parentID)

    def run(self):
        """
        Start the main loop.
        """
        seiscomp3.Logging.info("sceew-logging is running.")
        return seiscomp3.Client.Application.run(self)

if __name__ == '__main__':
    import sys
    app = Listener(len(sys.argv), sys.argv)
    sys.exit(app())
