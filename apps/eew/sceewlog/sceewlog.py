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

Author of the Software: Yannik Behr
"""

import sys, traceback
import smtplib
from email.mime.text import MIMEText
import os
import datetime
from dateutil import tz
import random
import seiscomp.client, seiscomp.core
import seiscomp.config, seiscomp.datamodel, seiscomp.system, seiscomp.utils
import seiscomp.geo
import socket


class Listener(seiscomp.client.Application):

    def __init__(self, argc, argv):
        seiscomp.client.Application.__init__(self, argc, argv)
        self.setMessagingEnabled(True)
        self.setDatabaseEnabled(True, True)
        self.setPrimaryMessagingGroup(
            seiscomp.client.Protocol.LISTENER_GROUP)
        self.addMessagingSubscription("LOCATION")
        self.addMessagingSubscription("MAGNITUDE")
        self.addMessagingSubscription("PICK")
        self.addMessagingSubscription("EVENT")

        self.cache = seiscomp.datamodel.PublicObjectTimeSpanBuffer()
        self.expirationtime = 3600.
        self.event_dict = {}
        self.origin_lookup = {}
        self.event_lookup = {}
        self.received_comments = []
        self.latest_event = seiscomp.core.Time.Null
        # report settings
        self.storeReport = False
        self.ei = seiscomp.system.Environment.Instance()
        self.report_head = "                                                                   |#St.   |                                                              \n"
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
        self.magTypes = ['MVS', 'Mfd']
        self.generateReportTimeout = 5  # seconds
        self.hb = None
        self.hb_seconds = None
        # UserDisplay interface
        self.activeMQ = False
        self.udevt = None
        self.amqMsgFormat = 'qml1.2-rt'
        self.udevtLhThresh = 0.0
        self.udevtMagThresh = 0.0
        self.profilesDic = []
        self.bnaFile = ''
        self.fs = None #GeoFeature object
        self.changeHeadline = False
        self.hlCitiesFile = None
        self.hlLanguage = None
        
        self.assocMagActivate = False
        self.assocMagPriority = ["magThresh","likelihood","stationMagNumber"]
        self.assocMagTypeThresh = { "Mfd":6.0, "MVS": 3.5 }
        self.assocMagStationNumber = { "Mfd":6, "MVS": 4 }
        self.assocMagAuthors = ["scvsmag"]
        self.fcm = False  #enabling FCM 
        self.moFcm = None # eews2fcm object
        self.fcmDataFile = None #File that contains keys and topic information for FCM
        
        self.eewComment = False #
                               
        
        
    def validateParameters(self):
        try:
            if seiscomp.client.Application.validateParameters(self) == False:
                return False
            if self.commandline().hasOption('savedir'):
                self.report_directory = self.commandline().optionString('savedir')
            if self.commandline().hasOption('playback'):
                self.setDatabaseEnabled(False, False)
            return True
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info:
                sys.stderr.write(i)
            return False

    def createCommandLineDescription(self):
        """
        Setup command line options.
        """
        try:
            self.commandline().addGroup("Reports")
            self.commandline().addStringOption(
                "Reports", "savedir", "Directory to save reports to.")
            self.commandline().addGroup("Mode")
            self.commandline().addOption("Mode", "playback", "Disable database connection.")
        except:
            seiscomp.logging.warning(
                "caught unexpected error %s" % sys.exc_info())
            info = traceback.format_exception(*sys.exc_info())
            for i in info:
                sys.stderr.write(i)
        return True

    def initConfiguration(self):
        """
        Read configuration file.
        """
        if not seiscomp.client.Application.initConfiguration(self):
            return False

        try:
            self.magTypes = self.configGetStrings("magTypes")
            self.generateReportTimeout = self.configGetInt(
                "generateReportTimeout")
        except Exception as e:
            pass

        try:
            self.sendemail = self.configGetBool("email.activate")
            self.smtp_server = self.configGetString("email.smtpserver")
            self.email_port = self.configGetInt("email.port")
            self.tls = self.configGetBool("email.usetls")
            self.ssl = self.configGetBool("email.usessl")
            if self.tls and self.ssl:
                seiscomp.logging.warning('TLS and SSL cannot be both true!')
                self.tls = False
                self.ssl = False
            self.auth = self.configGetBool("email.authenticate")
            if (self.auth):
                cf = seiscomp.config.Config()
                cf.readConfig(self.configGetString("email.credentials"))
                self.username = cf.getString('username')
                self.password = cf.getString('password')
            self.email_sender = self.configGetString("email.senderaddress")
            self.email_recipients = self.configGetStrings("email.recipients")
            self.email_subject = self.configGetString("email.subject")
            self.hostname = self.configGetString("email.host")
            self.magThresh = self.configGetDouble("email.magThresh")
        except Exception as e:
            seiscomp.logging.info(
                'Some configuration parameters could not be read: %s' % e)
            self.sendemail = False

        try:
            self.activeMQ = self.configGetBool("ActiveMQ.activate")
        except:
            seiscomp.logging.warning('not possible to check whether ActiveMQ is enable or not. Leaving this false')
            self.activeMQ = False
        
        try:
            self.assocMagActivate = self.configGetBool("magAssociation.activate")
        except:
            seiscomp.logging.warning('Not possible to read magAssociation.activate')
            seiscomp.logging.warning('setting default value: likelihood')
            self.assocMagActivate = False    
        
        
        if self.activeMQ:
            
            self.profilesInitConfig()
            self.hlChangeInitConfig()
            if self.assocMagActivate:
                self.prioritiesInitConfig()
            self.activeMQinitConfig()
        
        
        try:
            self.fcm = self.configGetBool("FCM.activate")
        except:
            self.fcm = False
            seiscomp.logging.warning('FCM.activate value is missing or not a set a proper boolean value')
            seiscomp.logging.warning('setting it to False')
            pass
        
        if self.fcm:
            self.profilesInitConfig()
            self.hlChangeInitConfig()
            if self.assocMagActivate:
                self.prioritiesInitConfig()
            self.fcmInitConfig()
            
        try:
            self.eewComment = self.configGetBool("EEW.comment")
        except Exception as e:
            seiscomp.logging.warning('Not possible to read EEW.comment. Setting this to False')
            self.eewComment = False

        try:
            self.storeReport = self.configGetBool("report.activate")
            self.expirationtime = self.configGetDouble("report.eventbuffer")
            self.report_directory = self.ei.absolutePath(
                self.configGetString("report.directory"))
        except:
            pass

        return True
        
    
    
    def activeMQinitConfig(self):
        """
        Init variables related to ActiveMQ. Only called when self.ActiveMQ = True
        """
        try:
            self.amqHost = self.configGetString('ActiveMQ.hostname')
            self.amqPort = self.configGetInt('ActiveMQ.port')
            self.amqTopic = self.configGetString('ActiveMQ.topic')
            self.amqHbTopic = self.configGetString('ActiveMQ.hbtopic')
            self.amqUser = self.configGetString('ActiveMQ.username')
            self.amqPwd = self.configGetString('ActiveMQ.password')
            self.amqMsgFormat = self.configGetString('ActiveMQ.messageFormat')
        except:
            seiscomp.logging.error('There was an error while reading the configuration for ActiveMQ. section. Please check it in detail' )
            sys.exit(-1)
    
    #Priorities
    def prioritiesInitConfig(self):
        try:
            self.assocMagPriority = self.configGetStrings("magAssociation.priority")
        except:
            seiscomp.logging.warning('Not possible to read magAssociation.priority')
            seiscomp.logging.warning('setting default value: likelihood')
            self.assocMagPriority = ["likelihood"]
            
        
        try:
            tmpList = self.configGetStrings("magAssociation.typeThresh")
            self.assocMagTypeThresh = {}
            for tmp in tmpList:
                magtype, magval = tmp.split(":")
                self.assocMagTypeThresh[magtype] = float(magval)
        except:
            seiscomp.logging.warning('Not possible to read magAssociation.typeThresh')
            seiscomp.logging.warning('setting default values: Mfd:6, MVS:3.5')
            self.assocMagTypeThresh = { "Mfd":6.0, "MVS": 3.5 }
        
        
        try:
            tmpList = self.configGetStrings("magAssociation.stationMagNumber")
            self.assocMagStationNumber = {}
            for tmp in tmpList:
                magtype, numStations = tmp.split(":")
                self.assocMagStationNumber[magtype] = int(numStations)
        except:
            seiscomp.logging.warning('Not possible to read magAssociation.stationMagNumber')
            seiscomp.logging.warning('setting default values: Mfd:6, MVS:4')
            self.assocMagStationNumber = { "Mfd":6, "MVS": 4 }
        
        try:
            self.assocMagAuthors = self.configGetStrings("magAssociation.authors")
            self.assocMagAuthors = b = list(map(lambda x: x.replace("@@hostname@",socket.gethostname()), self.assocMagAuthors))
            seiscomp.logging.info('Listed Authors are: %s' % str(self.assocMagAuthors) )
        except:
            seiscomp.logging.warning('Not possible to read magAssociation.authors')
            seiscomp.logging.warning('setting default values: unknown')
            self.assocMagAuthors = ["unknown"]
    
    #Firebase Cloud Messaging
    def fcmInitConfig(self):
        try:
            self.fcmDataFile = self.configGetString("FCM.dataFile")
            seiscomp.logging.info('Data file that contains Authorization Key and topic is:\n %s\n' % self.fcmDataFile )
        except:
            seiscomp.logging.warning('Not possible to read FCM.dataFile')
            seiscomp.logging.warning('Disabling FCM')
            self.fcm = False
            pass
        
        
        if self.fcm:   
            from eews2fcm import eews2fcm
            try:
                self.moFcm = eews2fcm(self.fcmDataFile, self.hlCitiesFile)
                self.moFcm.readFcmDataFile()
            except Exception as e:
                seiscomp.logging.error("Not possible to instancing eews2fcm object")
                seiscomp.logging.error("error: %s" % e)
                sys.exit(-1)
                
    #cap headline change    
    def hlChangeInitConfig(self):
        
        try:
            self.changeHeadline = self.configGetBool("ActiveMQ.changeHeadline")
        except:
            self.changeHeadline = False
            seiscomp.logging.warning('ActiveMQ.changeHeadline config value missing or not a set a proper boolean value')
            seiscomp.logging.warning('setting it to False')
            pass
        
        try:
            self.hlLanguage = self.configGetString('ActiveMQ.hlLanguage')
        except:
            self.hlLanguage = 'en-US'
            seiscomp.logging.warning('ActiveMQ.hlLanguage config value missing or not a set a proper string value')
            seiscomp.logging.warning('setting it to en-US')
        
        if self.hlLanguage not in ['es-US','en-US'] and self.changeHeadline:
            seiscomp.logging.error('ActiveMQ.hlLanguage must be "en-US" or "es-US"')
            seiscomp.logging.error('Please, fix this')
            sys.exit(-1)
            
        try:
            self.hlCitiesFile = self.configGetString('ActiveMQ.hlCitiesFile')
        except:
            if self.changeHeadline:
                seiscomp.logging.error('ActiveMQ.hlCities must be a path to the file that contains cities and their location')
                seiscomp.logging.error('please, check this and set the string value properly')
                sys.exit(-1)
    
    #Region profiles 
    def profilesInitConfig(self):
        
        profiles = []
        try:
            profiles = self.configGetStrings('ActiveMQ.profiles')
        except:
            seiscomp.logging.error('Error while reading the ActiveMQ.profiles. Check it in detail' )
            sys.exit(-1)
        
        #
        try:
            self.bnaFile = self.configGetString('ActiveMQ.bnaFile')
        except:
            seiscomp.logging.error('Error while reading the ActiveMQ.bnaFile' )
            sys.exit(-1)
        
        if self.bnaFile.lower() != 'none':
            self.fs = seiscomp.geo.GeoFeatureSet()
            fo = self.fs.readBNAFile(self.bnaFile, None)
            if not fo:
                seiscomp.logging.error('Not possible to open the bnaFile')
                sys.exit(-1)
        
        #Assoc Mag
        if len(profiles) > 0:
            #reading each profile
            for prof in profiles:
                
                tmpDic = {}
                tmpDic['name'] = prof
                
                #Mag Threshold
                try:
                    tmpDic['magThresh'] = self.configGetDouble( 'ActiveMQ.profile.' + prof + '.magThresh')
                except:
                    seiscomp.logging.warning('not possible to parse: '+'ActiveMQ.profile.' + prof + '.magThresh' )
                    seiscomp.logging.warning('setting '+'ActiveMQ.profile.' + prof + '.magThresh = 0.0')
                    tmpDic['magThresh'] = 0.0
                    pass
                
                #Likelihood Threshold
                try:
                    tmpVal = self.configGetDouble( 'ActiveMQ.profile.' + prof + '.likelihoodThresh')
                except:
                    seiscomp.logging.warning('Not possible to parse: '+'ActiveMQ.profile.' + prof + '.likelihoodThresh' )
                    seiscomp.logging.info('setting '+'ActiveMQ.profile.' + prof + '.likelihoodThresh = 0.0')
                    tmpDic['likelihoodThresh'] = 0.0
                    pass
                
                if tmpVal >= 0.0 and tmpVal <= 1:
                    tmpDic['likelihoodThresh'] = tmpVal
                else:
                    seiscomp.logging.error('Incorrect ActiveMQ.profile.' + prof + '.likelihoodThresh. It must be between 0.0 to 1.0' )
                    sys.exit(-1)
                
                #Min depth
                try:
                    
                    tmpDic['minDepth'] = self.configGetDouble( 'ActiveMQ.profile.' + prof + '.minDepth')
                except:
                    seiscomp.logging.warning('not possible to parse: '+'ActiveMQ.profile.' + prof + '.minDepth' )
                    seiscomp.logging.warning('setting '+'ActiveMQ.profile.' + prof + '.minDepth = 0.0')
                    tmpDic['minDepth'] = 0.0
                    pass
                
                #Max depth
                try:
                    
                    tmpDic['maxDepth'] = self.configGetDouble( 'ActiveMQ.profile.' + prof + '.maxDepth')
                except:
                    seiscomp.logging.warning('not possible to parse: '+'ActiveMQ.profile.' + prof + '.maxDepth' )
                    seiscomp.logging.warning('setting '+'ActiveMQ.profile.' + prof + '.maxDepth = 700.0')
                    tmpDic['maxDepth'] = 700.0
                    pass
                
                if tmpDic['minDepth'] > tmpDic['maxDepth']:
                    seiscomp.logging.error('Min Depth cannot be greater than maxDepth. Please, correct this error' )
                    sys.exit(-1)
                    
                #max time - seconds
                try:
                    tmpDic['maxTime'] = self.configGetInt( 'ActiveMQ.profile.' + prof + '.maxTime')
                except:
                    seiscomp.logging.warning('not possible to parse: '+'ActiveMQ.profile.' + prof + '.maxTime' )
                    seiscomp.logging.warning('setting '+'ActiveMQ.profile.' + prof + '.maxTime = -1')
                    tmpDic['maxTime'] = -1
                    pass
                
                #BNA closed polygon
                try:
                    tmpDic['bnaPolygon'] = self.configGetString( 'ActiveMQ.profile.' + prof + '.bnaPolygonName')
                except:
                    seiscomp.logging.error('not possible to parse: '+'ActiveMQ.profile.' + prof + '.bnaPolygonName' )
                    seiscomp.logging.error('please check in detail')                    
                    sys.exit(-1) #TODO: another clean way to exit?
                
                #if there is bna file or the polygon name is set to none or empty
                #then the bnaFeature object is None - no check if the origin is within a polygon
                if tmpDic['bnaPolygon'] == '' \
                or tmpDic['bnaPolygon'].lower() == 'none' \
                or self.fs == None:
                    seiscomp.logging.warning("No Polygon check for profile: "+tmpDic['name'])
                    tmpDic['bnaFeature'] = False
                else:
                    try:
                        #check if the closed polygon exists and if they are closed ones                   
                        tmpList = list( filter ( lambda x : x.name() == tmpDic['bnaPolygon'] and x.closedPolygon() , self.fs.features() ) )
                        
                        if len(tmpList) == 0:
                            seiscomp.logging.error('Closed polygon: '+tmpDic['bnaPolygon']+' does not not exist in '+ self.bnaFile )
                            seiscomp.logging.error('or the polygon IS NOT closed')
                            seiscomp.logging.error('please fix this')
                            sys.exit(-1)
                        else:
                            #if len(tmpList) > 1 then it will only use the first closed polygon
                            if len(tmpList) > 1:
                                seiscomp.logging.warning('There are more than one closed polygon with the name "%s"' % tmpDic['bnaPolygon'])
                                seiscomp.logging.warning('It will used only the first one')
                            tmpDic['bnaFeature'] = True
                            
                    except Exception as e:
                        seiscomp.logging.error('There was an error while checking the BNA file')
                        seiscomp.logging.error( repr(e) )
                        sys.exit(-1)

                
                #all okay with this poylgon and threshold values - Appending the tmpDic
                self.profilesDic.append(tmpDic)
                
        else:
            seiscomp.logging.warning('No ActiveMQ message filtering. All the configured magnitudes will be sent')
            pass

    
    def init(self):
        """
        Initialization.
        """
        if not seiscomp.client.Application.init(self):
            return False

        seiscomp.logging.info(
            "Listening to magnitude types %s" % str(self.magTypes))

        if not self.sendemail:
            seiscomp.logging.info('Sending email has been disabled.')
        else:
            self.sendMail({}, '', test=True)

        if self.storeReport:
            seiscomp.logging.info(
                "Reports are stored in %s" % self.report_directory)
        else:
            seiscomp.logging.info("Saving reports to disk has been DISABLED!")

        self.cache.setTimeSpan(seiscomp.core.TimeSpan(self.expirationtime))
        if self.isDatabaseEnabled():
            self.cache.setDatabaseArchive(self.query())
            seiscomp.logging.info('Cache connected to database.')
        self.enableTimer(1)
        try:
            if not self.activeMQ:
                seiscomp.logging.info('ActiveMQ is deactivated')
                return True
            import ud_interface
            self.udevt = ud_interface.CoreEventInfo(self.amqHost, self.amqPort,
                                                    self.amqTopic, self.amqUser,
                                                    self.amqPwd, self.changeHeadline,
                                                    self.hlLanguage, self.hlCitiesFile,
                                                    self.amqMsgFormat)
            self.hb = ud_interface.HeartBeat(self.amqHost, self.amqPort,
                                             self.amqHbTopic, self.amqUser,
                                             self.amqPwd, self.amqMsgFormat)
            self.hb_seconds = 1
            seiscomp.logging.info('ActiveMQ interface is running.')
        except Exception as e:
            seiscomp.logging.warning(
                'ActiveMQ interface cannot be loaded: %s' % e)
        return True

    def generateReport(self, evID):
        """
        Generate a report for an event, write it to disk and optionally send
        it as an email.
        """
        seiscomp.logging.info("Generating report for event %s " % evID)

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

            sout += "%4s|" % ed['type']
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

            if ed['difftopref'] < self.event_dict[evID]['diff']:
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
        seiscomp.logging.info("\n" + sout)
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
        for evID, evDict in self.event_dict.items():
            timer = evDict['report_timer']
            if not timer.isActive():
                continue
            seiscomp.logging.debug("There is an active timer for event %s (%s sec elapsed) "
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
        seiscomp.logging.debug(
            "Start report generation timer for magnitude %s " % magID)
        orgID = self.origin_lookup[magID]
        if orgID not in self.event_lookup:
            seiscomp.logging.debug(
                "Event not received yet for magnitude %s (setupGenerateReportTimer)" % magID)
            return
        evID = self.event_lookup[orgID]
        org = self.cache.get(seiscomp.datamodel.Origin, orgID)
        if not org:
            # error messages
            not_in_cache = "Object %s not found in cache!\n" % orgID
            not_in_cache += "Is the cache size big enough?\n"
            not_in_cache += "Have you subscribed to all necessary message groups?"
            seiscomp.logging.warning(not_in_cache)
            return
        mag = self.cache.get(seiscomp.datamodel.Magnitude, magID)
        if not mag:
            # error messages
            not_in_cache = "Object %s not found in cache!\n" % magID
            not_in_cache += "Is the cache size big enough?\n"
            not_in_cache += "Have you subscribed to all necessary message groups?"
            seiscomp.logging.warning(not_in_cache)
            return

        # use modification time if available, otherwise go for creation time
        try:
            # e.g. "1970-01-01T00:00:00.0000Z"
            updateno = mag.creationInfo().modificationTime().iso()
        except:
            # e.g. "1970-01-01T00:00:00.0000Z"
            updateno = mag.creationInfo().creationTime().iso()

        if updateno in self.event_dict[evID]['updates'].keys():
            # error messages
            err_msg = "Magnitude (%s) has the same creation/modifcaton time of an " % magID
            err_msg += "already received magnitude (%s)! Ignoring it.\n" % self.event_dict[
                evID]['updates'][updateno]['magID']
            seiscomp.logging.warning(err_msg)
            return

        # Check if the report has been already generated
        if self.event_dict[evID]['published']:
            # error messages
            err_msg = "A report has been already generated for magnitude %s." % magID
            err_msg += "This probably means the report timer expired before"
            err_msg += "this new magnitude was received by sceewlog."
            seiscomp.logging.error(err_msg)
            return

        # stop the generateReport timer if that was started already by a previous magnitude
        timer = self.event_dict[evID]['report_timer']
        if timer.isActive():
            timer.reset()

        self.event_dict[evID]['updates'][updateno] = {}
        self.event_dict[evID]['updates'][updateno]['magID'] = magID
        self.event_dict[evID]['updates'][updateno]['type'] = mag.type()
        self.event_dict[evID]['updates'][updateno]['author'] = mag.creationInfo(
        ).author()
        self.event_dict[evID]['updates'][updateno]['magnitude'] = mag.magnitude(
        ).value()
        self.event_dict[evID]['updates'][updateno]['lat'] = org.latitude().value()
        self.event_dict[evID]['updates'][updateno]['lon'] = org.longitude().value()
        self.event_dict[evID]['updates'][updateno]['depth'] = org.depth().value()
        self.event_dict[evID]['updates'][updateno]['nstorg'] = org.arrivalCount()
        try:
            self.event_dict[evID]['updates'][updateno]['nstmag'] = str(
                mag.stationCount())
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
        
        self.event_dict[evID]['updates'][updateno]['eew'] =  False
        
        seiscomp.logging.info("Number of updates %d for event %s" % (
            len(self.event_dict[evID]['updates']), evID))
        seiscomp.logging.info("lat: %f; lon: %f; mag: %f; ot: %s" %
                               (org.latitude().value(),
                                org.longitude().value(),
                                mag.magnitude().value(),
                                org.time().value().toString("%FT%T.%4fZ")))

        # Start generateReport timer
        timer.restart()

        # Make sure to attached additional information for this ev/mag
        self.processComments()
        
        magType = self.event_dict[evID]['updates'][updateno]['type']
        
        
        # Evaluation to send alert in case of a magType different
        # than MVS and Mfd
        # Generally the other mag types do not have likelihood values
        
        if magType in self.magTypes and magType != 'MVS' and magType != 'Mfd' and ( self.activeMQ or self.fcm ):
            #collecting the event udpate
            evtDic =  self.event_dict[evID]['updates'][updateno]
            
            #evaluate to send or not the alert based on profiles
            self.alertEvaluation( evID, magID, updateno ) 

    def sendAlert(self, magID):
        """
        Send an alert to a UserDisplay, if one is configured
        """
        if self.udevt is None and self.fcm == False:
            return
        

        seiscomp.logging.debug("Sending an alert for magnitude %s " % magID)
        orgID = self.origin_lookup[magID]
        evID = self.event_lookup[orgID]

        # if there are not updates yet, return
        if not self.event_dict[evID]['updates']:
            seiscomp.logging.debug(
                "Cannot send alert, no updates for ev %s (mag %s) " % (evID, magID))
            return

        ep = seiscomp.datamodel.EventParameters()
        evt = self.cache.get(seiscomp.datamodel.Event, evID)
        if evt:
            ep.add(evt)
        else:
            seiscomp.logging.debug("Cannot find event %s in cache." % evID)
        org = self.cache.get(seiscomp.datamodel.Origin, orgID)
        if org:
            ep.add(org)
            for _ia in range(org.arrivalCount()):
                pk = self.cache.get(seiscomp.datamodel.Pick,
                                    org.arrival(_ia).pickID())
                if not pk:
                    seiscomp.logging.debug(
                        "Cannot find pick %s in cache." % org.arrival(_ia).pickID())
                else:
                    ep.add(pk)
        else:
            seiscomp.logging.debug("Cannot find origin %s in cache." % orgID)
        
        if self.udevt is not None:
            self.udevt.send( self.udevt.message_encoder( ep ) )
        
        if self.fcm:
            self.moFcm.send( ep )
        
        # Adding a comment on magnitude object with
        # the number of times of EEW alert was sent (if enabled)
        if self.eewComment:
            try:
                #counting how many EEW alerts were sent out
                listUpdates = list( self.event_dict[evID]['updates'].values() )
                numEEW = sum( d['eew'] for d in listUpdates )
                #Notifying a comment about EEW on the magnitude object
                mag = self.cache.get(seiscomp.datamodel.Magnitude, magID)
                comment = seiscomp.datamodel.Comment();
                comment.setId("EEW")
                comment.setText(str(numEEW))
                
                #creation info
                ci = seiscomp.datamodel.CreationInfo()
                ci.setCreationTime(seiscomp.core.Time().GMT())
                comment.setCreationInfo(ci)
                
                #adding the comment object to mag object
                mag.add(comment)
                
                #Notifies
                n = seiscomp.datamodel.Notifier(mag.publicID(), seiscomp.datamodel.OP_ADD, comment)
                seiscomp.logging.debug("Notifier is enabled?: %s" % seiscomp.datamodel.Notifier.IsEnabled() )
                msg = seiscomp.datamodel.NotifierMessage()
                msg.attach(n)
                a=self.connection().send( 'MAGNITUDE', msg )
                seiscomp.logging.debug("Comment sent? %s" % "Yes" if a else "No, failed" )
                seiscomp.logging.debug("EEW alert sent so far: %s" % str( numEEW ) )
            except Exception as e:
                seiscomp.logging.error("There was an error while notifying a comment with the Num of times that EEW alert were sent")
                seiscomp.logging.error("Error message: %s" % repr(e))

    def handleMagnitude(self, magnitude, parentID):
        """
        Generate an origin->magnitude lookup table.
        """
        try:
            if magnitude.type() in self.magTypes:
                seiscomp.logging.debug(
                    "Received %s magnitude for origin %s" % (magnitude.type(), parentID))
                self.origin_lookup[magnitude.publicID()] = parentID
                self.setupGenerateReportTimer(magnitude.publicID())
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info:
                sys.stderr.write(i)

    def handleOrigin(self, org, parentID):
        """
        Add origins to the cache.
        """
        try:
            seiscomp.logging.debug("Received origin %s" % org.publicID())
            self.cache.feed(org)
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info:
                sys.stderr.write(i)

    def handlePick(self, pk, parentID):
        """
        Add picks to the cache.
        """
        try:
            seiscomp.logging.debug("Received pick %s" % pk.publicID())
            self.cache.feed(pk)
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info:
                sys.stderr.write(i)

    def garbageCollector(self):
        """
        Remove outdated events from the event dictionary.
        """
        eventsRemoved = set()
        tcutoff = self.latest_event - seiscomp.core.TimeSpan(self.expirationtime)
        for evID in self.event_dict.keys():
            if self.event_dict[evID]['timestamp'] < tcutoff:
                eventsRemoved.add(evID)
        for evID in eventsRemoved:
            self.event_dict.pop(evID)
            seiscomp.logging.debug("Expired event %s" % evID)

        originsRemoved = set()
        if eventsRemoved:
            for _orgID, _evID in self.event_lookup.items():
                if _evID in eventsRemoved:
                    originsRemoved.add((_orgID,_evID))
            for _orgID,_evID in originsRemoved:
                self.event_lookup.pop(_orgID)
                debuglog="Expired origin %s (ev %s)"
                seiscomp.logging.debug(debuglog % (_orgID, _evID))
        
        magnitudesRemoved = set()
        if originsRemoved:
            for _magID, _orgID in self.origin_lookup.items():
                if _orgID in originsRemoved:
                    magnitudesRemoved.add((_magID, _orgID))
            for _magID, _orgID in magnitudesRemoved: 
                self.origin_lookup.pop(_magID)
                debuglog="Expired mag %s (org %s)"
                seiscomp.logging.debug(debuglog % (_magID, _orgID))

    def handleEvent(self, evt):
        """
        Add events to the event dictionary and generate an
        event->origin lookup table.
        """
        evID = evt.publicID()
        seiscomp.logging.debug("Received event %s" % evID)

        for i in range(evt.originReferenceCount()):
            originID = evt.originReference(i).originID()
            self.event_lookup[originID] = evID
            seiscomp.logging.debug(
                "originId %s -> event %s" % (originID, evID))

            if evID not in self.event_dict.keys():
                self.event_dict[evID] = {}
                self.event_dict[evID]['published'] = False
                self.event_dict[evID]['lastupdatesent'] = None
                self.event_dict[evID]['updates'] = {}
                try:
                    self.event_dict[evID]['timestamp'] = \
                        evt.creationInfo().modificationTime()
                except:
                    self.event_dict[evID]['timestamp'] = \
                        evt.creationInfo().creationTime()
                if self.event_dict[evID]['timestamp'] > self.latest_event:
                    self.latest_event = self.event_dict[evID]['timestamp']
                self.event_dict[evID]['report_timer'] = seiscomp.utils.StopWatch(False)

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
            subject += ' / %s%.2f' % (evt['type'], evt['magnitude'])
            subject += ' / %.2fs' % evt['diff']
            subject += ' / %s' % self.hostname
            subject += ' / %s' % evID
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
        except Exception as e:
            seiscomp.logging.warning('Cannot connect to smtp server: %s' % e)
            return
        try:
            if self.tls:
                s.starttls()
            if self.auth:
                s.login(self.username, self.password)
            s.sendmail(self.email_sender,
                       self.email_recipients, msg.as_string())
        except Exception as e:
            seiscomp.logging.warning('Email could not be sent: %s' % e)
        try:
            s.quit()  
        except Exception as e:
            seiscomp.logging.warning("Error quiting smtp conexion: %s" % e)
        
    def processComments(self):
        """
        Process received comments not yet associated to the corresponding event
        """
        comments_to_keep = []

        for comment, parentID in self.received_comments:

            seiscomp.logging.debug("Processing comment %s for magnitude %s "
                                    % (comment.id(), parentID))

            if comment.id() in ('likelihood','rupture-strike','rupture-length'):
                magID = parentID
                if magID not in self.origin_lookup:
                    # comment belonging to an expired magnitude/event
                    continue

                orgID = self.origin_lookup[magID]
                if orgID not in self.event_lookup:
                    comments_to_keep.append( (comment, parentID) )
                    seiscomp.logging.debug("Event not received yet for magnitude %s \
                            origin %s(handleComment)" % (magID, orgID))
                    continue

                evID = self.event_lookup[orgID]
                if comment.id() == 'likelihood':
                    evt = self.cache.get(seiscomp.datamodel.Event, evID)
                    if evt:
                        # Check if orgID is missing from the Event
                        orgFound = False
                        for i in range(evt.originReferenceCount()):
                            if evt.originReference(i).originID() == orgID:
                                orgFound = True
                                break

                        # if orgID is missing add it to the event
                        if not orgFound:
                            seiscomp.logging.debug(
                                "Event %s doesn't have origin %s, add it then" % (evID, orgID))
                            orgRef = seiscomp.datamodel.OriginReference(orgID)
                            evt.add(orgRef)

                        # Check if magID is missing from orgID
                        magFound = False
                        tmpOrg = self.cache.get(seiscomp.datamodel.Origin, orgID)
                        for i in range(tmpOrg.magnitudeCount()):
                            if tmpOrg.magnitude(i).publicID() == magID:
                                magFound = True
                                break
 
                        if not magFound:
                            # here we should add the missing magnitude but
                            # for now just log the error
                            seiscomp.logging.error(
                                "Origin %s doesn't have magnitude %s (event %s)" % (orgID, magID, evID))
                            continue

                        evt.setPreferredOriginID(orgID)
                        evt.setPreferredMagnitudeID(magID)
                    else:
                        seiscomp.logging.debug("Cannot find event %s in cache." % evID)

                if evID not in self.event_dict:
                    seiscomp.logging.error("Internal logic error: cannot find \
                        event %s in self.event_dict" % evID)
                    continue

                # Attach the likelihood to the right update
                updateno = None
                for _updateno, _update_dict in self.event_dict[evID]['updates'].items():
                    if magID != _update_dict['magID']:
                        continue
                    if updateno is None:
                        updateno = _updateno
                        continue
                    msg = 'Found multiple updates with magID %s for the same' % magID
                    msg += '%s comment. Choosing the most recent one' % comment.id()
                    seiscomp.logging.warning(msg)
                    if updateno < _updateno:
                        updateno = _updateno

                if updateno is None:
                    comments_to_keep.append( (comment, parentID) )
                    msg = 'Could not find parent magnitude %s for %s comment' \
                            % (magID, comment.id())
                    seiscomp.logging.error(msg)
                    continue
                
                lhVal = -1.0
                if comment.id() == 'likelihood':
                    self.event_dict[evID]['updates'][updateno]['likelihood'] = lhVal = \
                            float(comment.text())
                    seiscomp.logging.info("likelihood value: %s" % lhVal)
                            
                elif comment.id() == 'rupture-strike':
                    self.event_dict[evID]['updates'][updateno]['rupture-strike'] = \
                            float(comment.text())
                
                elif comment.id() == 'rupture-length':
                    self.event_dict[evID]['updates'][updateno]['rupture-length'] = \
                            float(comment.text())
                
                #Evaluation to send or not an alert
                
                magType = self.event_dict[evID]['updates'][updateno]['type']
                
                #only evaluate an alert when there is a likelihood value for magnitude type MVS and Mfd
                #if lhVal >= 0 and self.activeMQ and magType == 'MVS' and magType in self.magTypes:
                if lhVal >= 0 and ( self.activeMQ or self.fcm ) and ( magType == 'MVS' or magType == 'Mfd' ) and magType in self.magTypes:

                    evtDic = self.event_dict[evID]['updates'][updateno]
                    #evaluate to send or not the alert based on profiles
                    self.alertEvaluation( evID, magID, updateno )
                    

        self.received_comments = comments_to_keep
    
    def alertEvaluation( self, evID, magID, updateno ):
        """
        Evaluation based on the profiles to send or not an alert.
        Additionally, there is a scoring option before evaluating 
        profiles.
        """
        #collecting the variables
        evt = self.event_dict[evID]['updates'][updateno]
        magVal = self.event_dict[evID]['updates'][updateno]['magnitude']
        depthVal = self.event_dict[evID]['updates'][updateno]['depth']
        latVal = self.event_dict[evID]['updates'][updateno]['lat']
        lonVal = self.event_dict[evID]['updates'][updateno]['lon']
        difftime = self.event_dict[evID]['updates'][updateno]['diff']
        magType = self.event_dict[evID]['updates'][updateno]['type']
        author = self.event_dict[evID]['updates'][updateno]['author']
        numarrivals = self.event_dict[evID]['updates'][updateno]['nstorg']
        nstmag = int(self.event_dict[evID]['updates'][updateno]['nstmag'])
        
        #Last reported sent - event dic
        lastupdatenoSent = self.event_dict[evID]['lastupdatesent']
        seiscomp.logging.debug("Last Update Sent: %s" % lastupdatenoSent if lastupdatenoSent != None else "NO UPDATE SENT YET" )
        if lastupdatenoSent != None:
            lastEvtSent = self.event_dict[evID]['updates'][lastupdatenoSent]
        else:
            lastEvtSent = {} #empty dic
                    
        if 'likelihood' in evt:
            lhVal = evt['likelihood'] 
        else:
            lhVal = None
        
        #going through priorities
        if self.assocMagActivate:
            for priority in self.assocMagPriority:
            
                if priority == "magThresh":
                    #tmpMagThresh = 0
                    for _magType, _magVal in self.assocMagTypeThresh.items():
                        if magType == _magType:
                            if magVal >= _magVal:
                                seiscomp.logging.debug("Priority passed for magnitude value: %s and type: %s" % ( magVal, magType ) )
                                break
                            else:
                                seiscomp.logging.debug("Priority NOT passed for magnitude value: %s and type: %s. No further evaluations" % (  magVal, magType ) )
                                return
                               
                if priority == "likelihood":#the first update WINS. Check the last preferred EEW sent
                    if len(self.event_dict[evID]['updates']) == 1:
                        if lhVal != None:
                            seiscomp.logging.debug("First update. No likelihood evaluation in priorities")
                            priorityPassed = True
                        
                        else:
                            seiscomp.logging.debug("No likehood value available for EEW magnitude value.")
                        
                    else:
                        #comparing likelihood values between last and current event
                        if 'likelihood' in lastEvtSent:
                            if lhVal >= lastEvtSent['likelihood']:
                                seiscomp.logging.debug("Likelihood value current value: %s higher or equal than the previous reported one: %s" % ( str( lhVal ), str( lastEvtSent['likelihood'] ) ) )
                            else:
                                seiscomp.logging.debug("Likelihood current value: %s lower than the previous reported one: %s. No further evaluations" % ( str( lhVal ), str( lastEvtSent['likelihood'] ) ) )
                                return
                        else:
                            seiscomp.logging.debug("Last Reported Event has no likelihood value")

                    if magType != "MVS" and magType != "Mfd": 
                        seiscomp.logging.debug("Magnitude value different than any EEW mag. Priority passed")
                        
                if priority == "authors":
                    if author in self.assocMagAuthors:
                        #author weight
                        currentIndex = self.assocMagAuthors.index( author )
                        weightCurrent = len(self.assocMagAuthors) - currentIndex
                        lastIndex = self.assocMagAuthors.index( lastEvtSent['author'] ) if len(lastEvtSent)>0 else -1
                        weightLast = len(self.assocMagAuthors) - ( lastIndex if lastIndex != -1 else len(self.assocMagAuthors) )
                        if weightCurrent >= weightLast:
                            seiscomp.logging.debug("Author with equal or higher priority %s than previous update: %s " % ( str(weightCurrent), str(weightLast) ) )
                        else:
                            seiscomp.logging.debug("This mag author has lower prioririty than the previous one" )
                            return
                    else:
                        #the author is not listed. No further evaluations.
                        seiscomp.logging.debug("Author: %s not listed on assocMagAuthors" % author )
                        return
                        
                
                if priority == "stationMagNumber":
                    for _magType, _stationCount in self.assocMagStationNumber.items():
                        if magType == _magType:
                            if nstmag >= _stationCount and len( lastEvtSent ) == 0 :
                                seiscomp.logging.debug("Number of Stations for mag type %s is %s" % ( magType, str(nstmag) ) )
                                break
                            elif nstmag >= _stationCount and len( lastEvtSent ) > 0 :
                                if nstmag > int(lastEvtSent['nstmag']):
                                    seiscomp.logging.debug("Current update has more stations: %s than the previous reported update: %s" % ( str(nstmag), str( lastEvtSent['nstmag'] ) ) )
                                    break
                                else:
                                    seiscomp.logging.debug( "Current update has less or equal number of stations: %s than the previous reported update: %s. No further evaluation." % ( str(nstmag), str( lastEvtSent['nstmag'] ) ) )
                                    return
            
            #finally, checking if the mag, lat, lon and depth significantly change
            if len( lastEvtSent ) > 0 :
                latDiffAbs = abs(lastEvtSent['lat'] - evt['lat'])
                lonDiffAbs = abs(lastEvtSent['lon'] - evt['lon'])
                depthDiffAbs = abs(lastEvtSent['depth'] - evt['depth'])
                magDiffAbs = abs(lastEvtSent['magnitude'] - evt['magnitude'])
                
                #evaluating the delta values
                if latDiffAbs >= 0.5 or lonDiffAbs >= 0.5 or depthDiffAbs >= 20 or magDiffAbs >=0.2:
                    seiscomp.logging.debug("Significant change -> deltaLat: %s, deltaLon: %s, deltaMag: %s, deltaDepth: %s" % \
                    (latDiffAbs, lonDiffAbs, magDiffAbs, depthDiffAbs) )
                    
                else:
                    seiscomp.logging.debug("Not significant change -> deltaLat: %s, deltaLon: %s, deltaMag: %s, deltaDepth: %s" % \
                    (latDiffAbs, lonDiffAbs, magDiffAbs, depthDiffAbs) )
                    return
                        
        #first Checking if origin location is within a closed polygon
        #there might be more than one polygon but once
        #a condition is accomplished then it will only alert once 
        
        for ft in self.profilesDic:
            noSend = False
            seiscomp.logging.debug( 'Evaluation for Profile: %s...' % ft['name'] )
            if magVal < ft['magThresh']:
                seiscomp.logging.debug('magVal was less than %s' % ft['magThresh'] )
                noSend = True
                
            if depthVal < ft['minDepth']:
                seiscomp.logging.debug('depth min value was less than %s' % ft['minDepth'] )
                noSend = True
            
            if depthVal > ft['maxDepth']:
                seiscomp.logging.debug('depth max value was greater than %s' % ft['maxDepth'] )
                noSend = True
            
            if lhVal != None:
                if lhVal < ft['likelihoodThresh']:
                    seiscomp.logging.debug('likelihood threshold was less than %s' % ft['likelihoodThresh'] )
                    noSend = True
                    
            if ft['maxTime'] != -1:
                if ft['maxTime'] < difftime:
                    seiscomp.logging.debug( 'Difftime of %s is greater than the maxTime for this profile' % difftime )
                    noSend = True
            
            if ft['bnaFeature']:
                tmpList = list( filter ( lambda x : x.name() == ft['bnaPolygon'], self.fs.features() ) )
                if len(tmpList) > 0:
                    if not tmpList[0].contains( seiscomp.geo.GeoCoordinate(float('%.3f' % latVal),float('%.3f' %lonVal)) ):
                        seiscomp.logging.debug('lat: %s and lon: %s are not within polygon: %s' \
                        % ( latVal, lonVal, ft['bnaPolygon'] ) )
                        noSend = True
                else:
                    seiscomp.logging.warning("There is no polygon whose name is %s " % ft['bnaPolygon'])
                            
                            
            if noSend:
                seiscomp.logging.debug('No alert for this origin and magnitude')
                continue
            else:
                seiscomp.logging.debug('Sending and alert....')
                self.event_dict[evID]['updates'][updateno]['eew'] = True
                #saving the last update sent or reported
                self.event_dict[evID]['lastupdatesent'] = updateno 
                
                self.sendAlert( magID )
                break
                            
        if len(self.profilesDic) == 0:
            #no profiles. Any origin and mag is reported
            seiscomp.logging.info('No profiles but activeMQ enabled. sending an alert...')
            self.sendAlert( magID )
                    
    def handleComment(self, comment, parentID):
        """
        Update events based on special magnitude comments.
        """
        if comment.id() in ('likelihood', 'rupture-strike', 'rupture-length'):
            seiscomp.logging.debug(
                "%s comment received for magnitude %s " % (comment.id(), parentID))
            self.received_comments.append((comment, parentID))
            self.processComments()

    def updateObject(self, parentID, object):
        """
        Call-back function if an object is updated.
        """
        pk = seiscomp.datamodel.Pick.Cast(object)
        if pk:
            self.handlePick(pk, parentID)

        mag = seiscomp.datamodel.Magnitude.Cast(object)
        if mag:
            self.handleMagnitude(mag, parentID)

        event = seiscomp.datamodel.Event.Cast(object)
        if event:
            evt = self.cache.get(seiscomp.datamodel.Event, event.publicID())
            self.handleEvent(evt)

        comment = seiscomp.datamodel.Comment.Cast(object)
        if comment:
            self.handleComment(comment, parentID)

    def addObject(self, parentID, object):
        """
        Call-back function if a new object is received.
        """
        pk = seiscomp.datamodel.Pick.Cast(object)
        if pk:
            self.handlePick(pk, parentID)

        mag = seiscomp.datamodel.Magnitude.Cast(object)
        if mag:
            self.cache.feed(mag)
            self.handleMagnitude(mag, parentID)

        origin = seiscomp.datamodel.Origin.Cast(object)
        if origin:
            self.handleOrigin(origin, parentID)

        event = seiscomp.datamodel.Event.Cast(object)
        if event:
            self.cache.feed(event)
            self.handleEvent(event)

        comment = seiscomp.datamodel.Comment.Cast(object)
        if comment:
            self.handleComment(comment, parentID)

    def run(self):
        """
        Start the main loop.
        """
        seiscomp.logging.info("sceew-logging is running.")
        return seiscomp.client.Application.run(self)


if __name__ == '__main__':
    import sys
    app = Listener(len(sys.argv), sys.argv)
    sys.exit(app())

