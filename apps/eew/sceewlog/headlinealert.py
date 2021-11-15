#!/usr/bin/env python
from math import asin, cos, radians, sin, sqrt, atan2, pi
import csv
import seiscomp.seismology
import xml.etree.ElementTree as ET

class HeadlineAlert:

    def __init__(self, dataFile):
        self.dataFile = dataFile
        self.scNamespace = '{http://geofon.gfz-potsdam.de/ns/seiscomp3-schema/0.11}'
        self.capNamespace = '{urn:oasis:names:tc:emergency:cap:1.2}'

    def getEpiLatLonFromDom(self, dom):
        
        epi ={'lat':999,'lon':999}
        
        ET.register_namespace("", "http://geofon.gfz-potsdam.de/ns/seiscomp3-schema/0.11")
        try:
            root = dom.getroot()
        except:
            #is dom already root? assumming yes!
            root = dom
        #event to obtain preferred originID 
        event = root[0].find(self.scNamespace+'event')
        preferredOriginID = event.find(self.scNamespace + 'preferredOriginID')[0].text
        preferredMagnitudeID = event.find(self.scNamespace + 'preferredMagnitudeID')[0].text
        
        origins = root[0].findall(self.scNamespace+'origin')
        
        preferredOrigin = None
        for origin in origins:
            if origin.attrib['publicID'] == preferredOriginID:
                preferredOrigin = origin
        
        if preferredOrigin is not None:
            epi['lat'] = float(preferredOrigin.find(self.scNamespace+'latitude')[0].text)
            epi['lon'] = float(preferredOrigin.find(self.scNamespace+'longitude')[0].text)
        
        else:
            origin = root[0].find(self.scNamespace + 'origin')
            epi['lat'] = float(origin.find(self.scNamespace+'latitude')[0].text)
            epi['lon'] = float(origin.find(self.scNamespace+'longitude')[0].text)
        
        return epi
        
    def replaceHeadline(self, hl, language, dom):
        ET.register_namespace("", "urn:oasis:names:tc:emergency:cap:1.2")
        
        root = None
        try:
            root = dom.getroot()
        except:
            #dom is root
            root = dom
            pass
        
        if root is None:
            return dom
            
        try:
            infoList = root.findall( self.capNamespace + 'info' )
            mainInfo = None
            for info in infoList:
                if info.find(self.capNamespace + 'language' ).text == language:
                    mainInfo = info
            if mainInfo is not None:
                mainInfo.find( self.capNamespace + 'headline' ).text = hl
            
        except:
            pass
            
        return dom
      
    def findNearestPlace( self, data, epi ):
        try:
            return min( data, key=lambda p: self.distance( [epi['lat'], float(p['lat']), epi['lon'], float(p['lon'])] ) )
        except:
            return None
    
    def findRegion( self, epiLat, epiLon):
        #Regions are in English
        return seiscomp.seismology.Regions.getRegionName(epiLat, epiLon)
    
    def distance( self, points ):
        #points is a list [refLat, epiLat, refLon, epiLon]
        
        refLat, epiLat, refLon, epiLon = map(radians, points)
        dist_lats = epiLat - refLat  
        dist_longs = epiLon - refLon 
        a = sin(dist_lats/2)**2 + cos(refLat) * cos(epiLat) * sin(dist_longs/2)**2
        c = 2 * atan2 ( sqrt(a), sqrt(1 - a) )
        radius_earth = 6371 # the Earth's radius 
        
        distance = c * radius_earth # distance in km
        return int(round(distance))
    
    def azimuth( self, points ):
        #points is a list [refLat, epiLat, refLon, epiLon]
        refLat, epiLat, refLon, epiLon = map(radians, points)
        dist_lats = epiLat - refLat  
        dist_longs = epiLon - refLon 
        azRad = atan2( sin(dist_longs)*cos(epiLat) , cos( refLat)*sin( epiLat ) - sin( refLat ) * cos( epiLat ) * cos ( dist_longs) )
        azDeg = azRad * 180 / pi
                
        if azDeg < 0:
            azimuth = 360 + azDeg
        else:
            azimuth = azDeg
        return int(round(azimuth))
    
    def direction(self, azVal, language ):
        
        if azVal >= 0 and azVal<10:
            return "N"
                
        elif azVal >= 10 and azVal<40:
            return "NNE"
            
        elif azVal >= 40 and azVal<50:
            return "NE"
            
        elif azVal >= 50 and azVal<80:
            return "ENE"
            
        elif azVal >= 80 and azVal < 100:
            return "E"
        
        elif azVal >= 100 and azVal < 130:
            return "ESE"
        
        elif azVal >= 130 and azVal < 140:
            return "SE"
        
        elif azVal >= 140 and azVal < 170:
            return "SSE"
        
        elif azVal >= 170 and azVal < 190:
            return "S"
        
        elif azVal >= 190 and azVal < 220:
            if language == 'es-US':
                return "SSO"
            else:
                return "SSW"
                
        elif azVal >= 220 and azVal < 230:
            if language == 'es-US':
                return "SO"
            else:
                return "SW"
            
        elif azVal >= 230 and azVal < 260:
            if language == 'es-US':
                return "OSO"
            else:
                return "WSW"
                
        elif azVal >= 260 and azVal < 280:
            if language == 'es-US':
                return "O"
            else:
                return "W"
            
        elif azVal >= 280 and azVal < 310:
            if language == 'es-US':
                return "ONO"
            else:
                return "WNW"
            
        elif azVal >= 310 and azVal < 320:
            if language == 'es-US':
                return "NO"
            else:
                return "NW"
            
        elif azVal >= 320 and azVal < 350:
            if language == 'es-US':
                return "NNO"
            else:
                return "NNW"
            
        elif azVal >= 350 and azVal < 360:
            return "N"
    
    def location(self, distKm, azText, city, country, language ):
        
        if language == 'es-US':
            return str(distKm)+' km al '+ azText + ' de '+ city + ', '+ country
        else:
            return str(distKm)+' km '+ azText + ' of '+ city + ', '+ country
    
    def region( self, lat, lon):
        #only English
        return self.findRegion(lat, lon)
    
    def csvFile2dic( self, filename ):
        with open(filename, encoding = 'utf8') as f:
            file_data=csv.reader(f)
            headers=next(file_data)
            return [dict(zip(headers,i)) for i in file_data]
