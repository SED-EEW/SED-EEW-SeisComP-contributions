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

Author: Billy Burgoa Rosso (billyburgoa@gmail.com)
"""

from io import BytesIO
from twisted.internet import reactor
import time
import os
import lxml.etree as ET
import seiscomp
from seiscomp.io import Exporter, ExportSink
import configparser
import time
from headlinealert import HeadlineAlert as hl
import google
import requests
import json
from google.oauth2 import service_account
from firebase_admin import credentials
import concurrent.futures


class eews2fcm:
    #constructor needs FCM data file containing The projectid, service file, and topic.
    #language can be es-US or en-US. By default is es-US
    def __init__(self, worldCitiesFile, language = "es-US", fcmDataFile=None):

        #this is the long string template for sending notification to apps with version less than 2.0.0
        self.oldFormat = '''%EVTID%;%MAG%;%DEPTH%;%LAT%;%LON%;%LIKELIHOOD%;%ORTIME%;%TIMENOW%;NULL;%AGENCY%;%STATUS%;%TYPE%;%LOCATION%;%ORID%;%MAGID%;%STAMAGNUM%;%TIME_NOWMS%;%DISTANCE%'''

        self.fcmDataFile = fcmDataFile
        self.authKey = ''
        self.topic = ''
        self.worldCitiesFile = worldCitiesFile
        self.hlalert = hl(self.worldCitiesFile)
        self.dic = self.hlalert.csvFile2dic(self.hlalert.dataFile)
        self.language = language
        self.agency = ''
        self.distance = 0
        self.projectId = "" 
        self.serviceAccountFile = ""
        self.oldFormatSupport = False
        self.android = True
        self.ios = True

    def readFcmDataFile(self):
        if os.path.exists( self.fcmDataFile ):
            config = configparser.RawConfigParser()

        else:
            seiscomp.logging.error("the file %s does not exists" % self.fcmDataFile )
            return -1

        try:
            config.read(self.fcmDataFile)
        except Exception as e:
            seiscomp.logging.error("Not possible to open %s " % self.fcmDataFile)
            return -1

        try:
            self.topic = config.get('TOPICS', 'topic')
            self.serviceAccountFile = config.get('SERVICEFILE', 'servicefile')
            self.projectId = config.get('PROJECTID', 'projectid')
            self.oldFormatSupport = config.getboolean("SUPPORT_OLD_FORMAT","oldformat") 
            self.android = config.getboolean("ENABLED_OS","android")
            self.ios = config.getboolean("ENABLED_OS","ios")
        except:
            seiscomp.logging.error('Not possible to parse the configuration file.')
            return -1
    def get_access_token(self):
        """Retrieve a valid access token that can be used to authorize requests.

        :return: Access token.
        """
        SCOPES=["https://www.googleapis.com/auth/firebase.messaging"] # only for notifications
        credentials = service_account.Credentials.from_service_account_file(
          self.serviceAccountFile, scopes=SCOPES) 
        request = google.auth.transport.requests.Request()
        credentials.refresh(request)
        return credentials.token

    # Method to create the message dictionary for Firebase
    # It is necessary to provide as input the event parameter object (ep)
    # It will return the message dictonary,
    def message_creator(self, ep ):
         
        evtPayload = {
        "notificationType": "eqNotification",
        "eventId": "%EVENTID%",
        "magnitude": "%MAG%", #forced to use strings for numbers
        "depth": "%DEPTH%",
        "latitude": "%LAT%",
        "longitude": "%LON%",
        "likelihood": "%LIKELIHOOD%",
        "originTime": "ORTIME",
        "sentTime": "TIMENOW%",
        "agency": "%AGENCY%",
        "status": "%STATUS%",
        "type": "alert",
        "location": "%LOCATION%",
        "numArrivals": "%STAMAGNUM%",
        "nearPlaceDist": "%DISTANCE%",
        "message": "%OLDFORMAT%",
        "title": "someOldformatTitle",
        "body": "someOldformatBody"
        }
        
        #The time to live for both ios and android is one day. TODO: add a new configuration file to get this value
        apnsTimeToLiveInSeconds = int(time.time()) + 86400
        androidTimeToLiveInSeconds = "86400s"

        try:
            evt = ep.event(0)
            evtid = evt.publicID()
            agency = evt.creationInfo().agencyID()
            self.agency = agency
            prefOrID = evt.preferredOriginID()
            prefMagID = evt.preferredMagnitudeID()
            origin = ep.findOrigin(prefOrID)
            magnitude = origin.findMagnitude(prefMagID)

            mag = origin.findMagnitude(prefMagID).magnitude().value()
            magObj = origin.findMagnitude(prefMagID)
            lat = origin.latitude().value()
            lon = origin.longitude().value()
            depth = origin.depth().value()
            orTime = origin.time().value().seconds()
            now = time.time() #UTC
            nowms = int(time.time_ns()/1000000)
            now_seconds = int(time.time())
            likelihood = "0.0"
            numarrivals = 0
            numStaMag = 0

            try:
                 numStaMag = magObj.stationCount()
            except:
                numStaMag = 0

            try:
                numarrivals = origin.arrivalCount()
            except Exception as e:
                numarrivals = 0

            for x in range( magObj.commentCount() ):
                c = magObj.comment(x)
                if c.id() == 'likelihood':
                    likelihood = c.text()
                    seiscomp.logging.debug( "likelihood value found: %s" % c.text() )
                    break

        except Exception as e:
            seiscomp.logging.error("There was an error while using event parameter object")
            seiscomp.logging.error("Error message: %s" % e )
            return -1

        location = self.findNearLocation(lat, lon, mag )

        evtPayload["agency"] = agency

        evtPayload["eventId"] = evtid

        evtPayload["magnitude"] = str(round(mag,1))

        evtPayload["depth"] = str(round(depth,1))

        evtPayload["latitude"] = str(lat)

        evtPayload["longitude"] = str(lon)

        evtPayload["likelihood"] = likelihood

        evtPayload["originTime"] = str(orTime)

        evtPayload["sentTime"] = str(now)

        evtPayload["status"] = "automatic"

        evtPayload["type"] = "alert"

        evtPayload["location"] = location

        evtPayload["numArrivals"] = str(numStaMag)

        evtPayload["nearPlaceDist"] = str(self.distance)
        
        if self.oldFormatSupport:
            oldMsg = self.oldFormat
            oldMsg = oldMsg.replace("%EVTID%", evtid)
            oldMsg = oldMsg.replace("%AGENCY%",agency)
            oldMsg = oldMsg.replace("%MAG%",str(round(mag,1)))
            oldMsg = oldMsg.replace("%DEPTH%",str(round(depth,1)))
            oldMsg = oldMsg.replace("%LAT%",str(lat))
            oldMsg = oldMsg.replace("%LON%",str(lon))
            oldMsg = oldMsg.replace("%LIKELIHOOD%", likelihood )
            oldMsg = oldMsg.replace("%ORTIME%", str(orTime))
            oldMsg = oldMsg.replace("%TIMENOW%",str(now_seconds)) #exception: this is in seconds for old format
            oldMsg = oldMsg.replace("%STATUS%","automatic")
            oldMsg = oldMsg.replace("%TYPE%", "alert")
            oldMsg = oldMsg.replace("%LOCATION%", location)
            oldMsg = oldMsg.replace("%ORID%", prefOrID)
            oldMsg = oldMsg.replace("%MAGID%", prefMagID)
            oldMsg = oldMsg.replace("%STAMAGNUM%", str(numStaMag) )
            oldMsg = oldMsg.replace("%TIME_NOWMS%",str(nowms))
            oldMsg = oldMsg.replace("%DISTANCE%", str(self.distance) )
            
            evtPayload["message"]=oldMsg
            evtPayload["body"]= "eqNotification"
            evtPayload["title"]= "oldNotif"
        
        # Construct JSON request payload
        
        payload = {
            "message": {
                    "topic": self.topic,        
            }
        }
        
        if self.android : 
            payload["message"]["data"] = evtPayload
            payload["message"]["android"] = {
                "priority": "high",
                "ttl": androidTimeToLiveInSeconds
            }
        if self.ios:
            payload["message"]["apns"] = {
                        "headers": {
                           "apns-priority": "10",
                           "apns-push-type": "alert",
                           "apns-expiration": str(apnsTimeToLiveInSeconds)
                        },
                        "payload": {
                            "aps": {
                                "alert": {
                                    "title": "¡SISMO!",
                                    "subtitle": "Mag: "+evtPayload["magnitude"]+", "+evtPayload["location"]
                                },
                               "content-available" : 1,
                               "mutable-content" : 1
                        },
                        "data": evtPayload
                    }
            }
            
        return payload
        
    def send( self, ep ):
        # A thread pool executor to send the message asynchronously
        with concurrent.futures.ThreadPoolExecutor() as executor:
            future = executor.submit(self._send_message, ep)
    
    def _send_message(self, ep):
        # getting the message dictionary
        payload = self.message_creator(ep)

        # Send HTTP request with authorization
        accessToken = self.get_access_token()
        headers = {'Authorization': f'Bearer {accessToken}'}
        url = f'https://fcm.googleapis.com/v1/projects/{self.projectId}/messages:send'
        response = requests.post(url, headers=headers, json=payload)

        # Handle response
        if response.status_code == 200:
            seiscomp.logging.debug('Message sent successfully!')
        else:
            seiscomp.logging.error('Error sending message:' + response.text)

    def findNearLocation (self, epiLat, epiLon, mag):
        np = dis = azVal = azTextSp = azTextEn = None
        epi = {'lat': epiLat,'lon': epiLon}
        np = self.hlalert.findNearestPlace(self.dic, epi)
        dis = self.hlalert.distance([ float(np['lat']), epi['lat'], float(np['lon']), epi['lon'] ] )
        self.distance = dis
        azVal = self.hlalert.azimuth([ float(np['lat']), epi['lat'], float(np['lon']), epi['lon'] ] )
        azTextSp = self.hlalert.direction(azVal, 'es-US')
        azTextEn = self.hlalert.direction(azVal, 'en-US')

        if self.language == 'es-US':
            location = self.hlalert.location(dis, azTextSp, np['city'],np['country'], self.language)
        else:
            location = self.hlalert.location(dis, azTextEn, np['city'],np['country'], self.language)

        region = self.hlalert.region( epi['lat'], epi['lon'] )
        #
        if self.language == 'es-US':
            hlSpanish = location
            hlEnglish = region
        else:
            hlSpanish = region
            hlEnglish = location

        return hlSpanish if self.language == 'es-US' else hlEnglish


if __name__ == '__main__':
    pass
