#!/usr/bin/env python

import sys, traceback
import seiscomp.client, seiscomp.core
import seiscomp.config, seiscomp.datamodel, seiscomp.system, seiscomp.utils
from seiscomp.geo import GeoFeatureSet, GeoCoordinate

class Listener(seiscomp.client.Application):

    def __init__(self, argc, argv):
        seiscomp.client.Application.__init__(self, argc, argv)
        self.setMessagingEnabled(True)
        self.setDatabaseEnabled(False, False)
        self.setPrimaryMessagingGroup(
            seiscomp.client.Protocol.LISTENER_GROUP)
        self.addMessagingSubscription("LOCATION")

    def validateParameters(self):
        try:
            if seiscomp.client.Application.validateParameters(self) == False:
                return False
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

        # The BNA file including all polygons
        try:
            self.bnaFile = self.configGetString("bnaFile")
        except Exception as e:
            pass

        if self.bnaFile.lower() != 'none':
            self.fs = GeoFeatureSet()
            if not self.fs.readBNAFile(self.bnaFile, None):
                seiscomp.logging.error('Impossible to open the bna file %s'%self.bnaFile)
                pass

        profiles = []
        self.profilesDic = []
        try:
            profiles = self.configGetStrings('profiles')
        except:
            seiscomp.logging.error('Error while reading the list of profiles.' )
            pass

        if len(profiles) > 0:
            for name in profiles:
                tmpDic = {}
                tmpDic['name'] = name
                
                # Author of origin and magnitude 
                try:
                    tmpDic['author'] = self.configGetString( 'profile.' + name + '.author')
                except:
                    seiscomp.logging.error('Impossible to parse: '+'profile.' + name + '.author' )
                    sys.exit(-1) 

                # Closed BNA polygon
                try:
                    tmpDic['polygon'] = self.configGetString( 'profile.' + name + '.polygon')
                except:
                    seiscomp.logging.error('Please fix this: Impossible to parse profile.%s.polygon' % name )
                    sys.exit(-1) 
                
                # Check if the polygon is setup 
                if (tmpDic['polygon'] == '' 
                    or tmpDic['polygon'].lower() == 'none' 
                    or self.fs == None):
                    seiscomp.logging.warning("Please fix this: No polygon for profile: "+tmpDic['name'])
                    sys.exit(-1)
                
                # Check if the polygon exists and if it is closed
                try:                   
                    tmpList = list( filter ( lambda x : x.name() == tmpDic['polygon'] and x.closedPolygon() , self.fs.features() ) )        
                except Exception as e:
                    seiscomp.logging.error('There was an error while checking the BNA file')
                    seiscomp.logging.error( repr(e) )
                    sys.exit(-1)
                
                if len(tmpList) == 0:
                    seiscomp.logging.error('Please fix this: Polygon',tmpDic['polygon'],' does not not exist or is not closed in ',self.bnaFile )
                    sys.exit(-1)
                elif len(tmpList) > 1:
                    seiscomp.logging.warning('Please fix this: There are several polygons with the name "%s"' % tmpDic['polygon'])
                    sys.exit(-1)

                seiscomp.logging.info('Adding profile %s: author=%s & polygon=%s'%(tmpDic['name'],tmpDic['author'],tmpDic['polygon']))
                
                tmpDic['feature'] = tmpList[0]

                self.profilesDic.append(tmpDic)
                
        else:
            seiscomp.logging.warning('No origin filtering, all the origins will pass.')
            pass
        
        return True
        
    def init(self):
        """
        Initialization.
        """
        if not seiscomp.client.Application.init(self):
            return False

        seiscomp.logging.info("BNA file %s" % str(self.bnaFile))
        for d in self.profilesDic:
            seiscomp.logging.info('Profile: '+', '.join(['%s: %s'%(k,d[k]) for k in d if isinstance(d[k],str)]))
        return True

    def geographicCheckOrigin(self, origin):

        seiscomp.logging.debug('Author: %s'%origin.creationInfo().author()+"origin.publicID = {}".format(origin.publicID()))
        for profile in self.profilesDic:
            seiscomp.logging.debug( 'Evaluation for profile %s (author: %s, polygon: %s)...' % ( profile['name'], profile['author'], profile['polygon'] ) )

            if origin.creationInfo().author().split('@')[0] != profile['author'].split('@')[0]:
                seiscomp.logging.debug('Author: %s does not match with profile author %s'%(origin.creationInfo().author(),profile['author']))
                continue
            
            coordinates = GeoCoordinate( origin.latitude().value(), 
                                         origin.longitude().value() )
            tmp = ( profile['author'], 
                    origin.latitude().value(), 
                    origin.longitude().value(), 
                    profile['polygon'] )

            if not profile['feature'].contains( coordinates ):
                seiscomp.logging.debug('%s origin (lat: %s and lon: %s) is not within polygon %s.' % tmp )
                continue
            else:
                seiscomp.logging.debug('Will be sent: %s origin (lat: %s and lon: %s) within polygon %s.' % tmp )

            # Send the origin if it is in polygon
            return origin

        return False


    def handleOrigin(self, org, op=seiscomp.datamodel.OP_ADD):
        """
        Test origins and redirect those matching profiles
        """
        try:
            seiscomp.logging.debug("Received origin %s" % org.publicID())
            org = self.geographicCheckOrigin(org)
        except:
            info = traceback.format_exception(*sys.exc_info())
            for i in info:
                sys.stderr.write(i)
        if org:
            msg = seiscomp.datamodel.NotifierMessage()
            n = seiscomp.datamodel.Notifier("EventParameters", op, org)
            msg.attach(n)
            self.connection().send(msg)

    def updateObject(self, parentID, object):
        """
        Call-back function if an object is updated.
        """
        origin = seiscomp.datamodel.Origin.Cast(object)
        if origin:
            self.handleOrigin(origin, op=seiscomp.datamodel.OP_UPDATE)

    def addObject(self, parentID, object):
        """
        Call-back function if a new object is received.
        """
        origin = seiscomp.datamodel.Origin.Cast(object)
        if origin:
            self.handleOrigin(origin)
 
    def run(self):
        """
        Start the main loop.
        """
        seiscomp.logging.info("scfinder-filtering is running.")
        return seiscomp.client.Application.run(self)


if __name__ == '__main__':
    import sys
    app = Listener(len(sys.argv), sys.argv)
    sys.exit(app())

