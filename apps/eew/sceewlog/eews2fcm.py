from io import BytesIO
from twisted.internet import reactor
import datetime
import time
import os
import lxml.etree as ET
import seiscomp
from seiscomp.io import Exporter, ExportSink
import configparser
import time
from headlinealert import HeadlineAlert as hl
import subprocess

class eews2fcm:
    #constructor needs FCM data file containing Auth key and FCM topic
    #language can be es-US or en-US. By default is es-US
    def __init__(self, fcmDataFile, worldCitiesFile, language = "es-US"):

        #this is the long string template for 
        self.dataTemplate = '''curl -s -H "Content-type: application/json" \
 -H "Authorization:key=AUTHORIZATION_KEY"  \
-X POST -d '{ "to": "/topics/TOPIC","data":{"title":"ATTAC Alerta de Terremotos","body":"Mag: MAG, LOCATION","message":"EVTID;MAG;DEPTH;LAT;LON;LIKELIHOOD;ORTIME;TIMENOW;NULL;AGENCY;STATUS;TYPE;LOCATION;ORID;MAGID;STAMAGNUM;TIME_NOWMS;DISTANCE"},"priority":"high","time_to_live":60}' \
https://fcm.googleapis.com/fcm/send'''

        self.notiTemplate = '''curl -s -H "Content-type: application/json" \
 -H "Authorization:key=AUTHORIZATION_KEY"  \
-X POST -d '{ "to": "/topics/TOPIC","notification":{"title":"Alerta de Terremoto","body":"Mag: MAG, LOCATION"},"android": {"priority":"high"}}' \
https://fcm.googleapis.com/fcm/send'''
        
        self.fcmDataFile = fcmDataFile
        self.authKey = ''
        self.topic = ''
        self.worldCitiesFile = worldCitiesFile
        self.hlalert = hl(self.worldCitiesFile)
        self.dic = self.hlalert.csvFile2dic(self.hlalert.dataFile)
        self.language = language
        self.agency = ''
        self.distance = 0
    
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
            self.authKey = config.get('AUTHKEY', 'key')
            self.topic = config.get('TOPICS', 'topic')
        except:
            seiscomp.logging.error('Not possible to parse the configuration file.')
            return -1
    
    def send( self, ep ):
        tmpNoti = self.notiTemplate
        tmpData = self.dataTemplate
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
            orTime = int(origin.time().value().seconds())
            now = int(time.time()) #UTC
            nowms = int(time.time_ns()/1000000)
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
        
        tmpNoti = tmpNoti.replace("AUTHORIZATION_KEY",self.authKey)
        
        tmpNoti = tmpNoti.replace("TOPIC", self.topic)
        
        tmpNoti = tmpNoti.replace("MAG",str(round(mag,1)) )
        
        location = self.findNearLocation(lat, lon, mag )
        
        tmpNoti = tmpNoti.replace("LOCATION",location)
        
        #EVTID;MAG;DEPTH;LAT;LON;LIKELIHOOD;ORTIME;TIMENOW;NULL;AGENCY;STATUS;TYPE;LOCATION
        tmpData = tmpData.replace("AUTHORIZATION_KEY",self.authKey)
        
        tmpData = tmpData.replace("TOPIC", self.topic)
        
        tmpData = tmpData.replace("AGENCY",agency)
        
        tmpData = tmpData.replace("EVTID", evtid)
        
        tmpData = tmpData.replace("MAG",str(round(mag,1)))
        
        tmpData = tmpData.replace("DEPTH",str(round(depth,1)))
        
        tmpData = tmpData.replace("LAT",str(lat))
        
        tmpData = tmpData.replace("LON",str(lon))
        
        tmpData = tmpData.replace("LIKELIHOOD", likelihood )
        
        tmpData = tmpData.replace("ORTIME", str(orTime))
        
        tmpData = tmpData.replace("TIMENOW",str(now))
        
        tmpData = tmpData.replace("AGENCY",agency)
        
        tmpData = tmpData.replace("STATUS","automatic")
        
        tmpData = tmpData.replace("TYPE", "alert")
        
        tmpData = tmpData.replace("LOCATION", location)
        
        tmpData = tmpData.replace("ORID", prefOrID)
        
        tmpData = tmpData.replace("MAGID", prefMagID)
        
        tmpData = tmpData.replace("STAMAGNUM", str(numStaMag) )
        
        tmpData = tmpData.replace("TIME_NOWMS",str(nowms))
        
        tmpData = tmpData.replace("DISTANCE", str(self.distance) )
        
        try:
            output = subprocess.Popen(tmpData,stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell = True )
            #os.system(tmpData)
        except Exception as e:
            seiscomp.logging.error("Not possible to send data to FCM")
        
        if output.stderr != None:
            seiscomp.logging.error("There was an error: %s" % str(output.stderr) )
        else:
            seiscomp.logging.debug("alert sent: %s" % str(output.stdout.read()) )
       # try:
       #     os.system(tmpNoti)
       # except:
       #     seiscomp.logging.error("Not possible to notification to FCM")
    
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

