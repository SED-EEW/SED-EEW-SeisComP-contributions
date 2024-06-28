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

This script generates a polygon map based on the configuration files of sceewlog and its aliases.
The map displays the polygons defined in the configuration files and their corresponding parameters.

Author: Fred Massin

Usage:
    - Install required packages: python3-cartopy, python3-matplotlib, python3-scipy
    - Run the script: ./eewlogconfplot.py

License:
    This program is licensed under the GNU Affero General Public License version 3 or later.
    For more details, see the LICENSE file.

Dependencies:
    - configparser
    - os
    - fnmatch
    - matplotlib
    - seiscomp.geo
    - matplotlib.pyplot
    - cartopy.crs
    - shapely.geometry.Polygon
    - cartopy.feature
    - matplotlib.patches
    - matplotlib.patheffects
    - datetime.date

Environment Variables:
    - SEISCOMP_ROOT: The root directory of the SeisComP installation.

Configuration Files:
    - The script reads the configuration files of sceewlog and its aliases.
    - The configuration files are expected to be located in the following directories:
        - $SEISCOMP_ROOT/etc/defaults
        - $SEISCOMP_ROOT/etc
        - $HOME/.seiscomp
    - The configuration files should have the following naming convention: <basename>.cfg
    - The script looks for the following configuration files:
        - sceewlog.cfg (for sceewlog)
        - <alias>.cfg (for each alias of sceewlog)

Output:
    - The script generates a polygon map and saves it as 'polygon_map.png'.

Note:
    - The script uses the matplotlib backend 'Agg' to save the figures without displaying them.
    - The script requires the 'regfilters.bnafile' parameter to be defined in the configuration files and to point to a valid BNA polygon file.

"""


import configparser
import os
import fnmatch
import matplotlib
import seiscomp.geo
import matplotlib.pyplot as plt
import cartopy.crs as ccrs
from shapely.geometry import Polygon
import cartopy.feature as cfeature
import matplotlib.patches as mpatches
import matplotlib.patheffects as pe
from datetime import date

# Create a path effect with a white outline
path_effects = [pe.Stroke(linewidth=4, foreground='white'), pe.Normal()]

parwhitelist = ['magTypes',
                'email.activate',
                'ActiveMQ.activate',
                'magAssociation.activate',
                'FCM.activate'
                ]

# Config parser does not support case
parwhitelist = [k.lower() for k in parwhitelist]

# setup matplotlib backend so it doesn't display figure just save them
matplotlib.use('Agg')  
golden = (1 + 5 ** 0.5) / 2 * 10
fig = plt.figure(figsize=(golden*.9,10*.9))
ax = fig.add_subplot(1, 1, 1, projection=ccrs.PlateCarree())
lalo = [[],[]]
labels = []
polygons =[]

# Get env variable SEISCOMP_ROOT
HOME = os.getenv('HOME')
SEISCOMP_ROOT = os.getenv('SEISCOMP_ROOT')
if SEISCOMP_ROOT is None:
    print("SEISCOMP_ROOT environment variable is not set.")
else:
    print(f"SEISCOMP_ROOT is set to {SEISCOMP_ROOT}")

if SEISCOMP_ROOT is None:
    exit(1)

# Find all aliases of sceewlog in SEISCOMP_ROOT
aliases = []
for root, dirnames, filenames in os.walk('%s/bin/'%SEISCOMP_ROOT):
    for filename in fnmatch.filter(filenames, '*'):
        filepath = os.path.join(root, filename)
        if os.path.islink(filepath):
            target_path = os.readlink(filepath)
            if 'sceewlog' in os.path.basename(target_path) :
                aliases.append(filename)
print(f"All aliases of sceewlog in SEISCOMP_ROOT: {aliases}")

configs = {basename: configparser.ConfigParser(strict=False) for basename in ['sceewlog']+aliases }
# Load the config of sceewlog and all its aliases 
for basename in ['sceewlog']+aliases: 
    for p in ['%s/etc/defaults'%SEISCOMP_ROOT, '%s/etc'%SEISCOMP_ROOT, '%s/.seiscomp'%HOME]:

        filename = '%s/%s.cfg'%(p,basename) 
        config = configs[basename] 

        try:
            with open(filename, 'r') as f:
                config_string = '[dummy_section]\n' + f.read()
            config_string = config_string.replace(', \\\n',',')
            config.read_string(config_string)
            print(f"Loaded {filename}.")
        except FileNotFoundError:
            print(f"File {filename} not found.")
            continue
        except Exception as e:
            print(f"Could not load {filename}.")
            print(e)
            continue
parameterstring = []
for basename in ['sceewlog']+aliases: #
    config = configs[basename]

    # get list of parameter in dummy_section as dictionary 
    parameters = dict(config.items('dummy_section'))
    parameterstring += [ r'$\bf{%s/%s}$:$%s$'%(basename,k,v) for i,(k,v) in enumerate(parameters.items()) if k in parwhitelist]

    # read bna polygon file
    if 'regfilters.bnafile' in parameters:
        fs = seiscomp.geo.GeoFeatureSet()
        fo = fs.readBNAFile(parameters['regfilters.bnafile'], None)
        if not fo:
            print(f"Not possible to open {parameters['regfilters.bnafile']}.")

    # Loop over each polygon
    for profile in parameters['regfilters.profiles'].replace(' ','').lower().split(','):
        
        polygonname = parameters['regfilters.profile.%s.bnapolygonname'%profile]
        tmpList = [fs.features()[i] for i,x in enumerate(fs.features()) if fs.features()[i].name() == polygonname]

        
        if len(tmpList) > 0:
            polygon = tmpList[0]
            
            # Get the vertices of the polygon
            vertices = [(vertex.lon, vertex.lat) for vertex in polygon.vertices()]
            lalo[0] += [vertex.lon for vertex in polygon.vertices()]
            lalo[1] += [vertex.lat for vertex in polygon.vertices()]

            # Convert vertices to a Polygon
            polygon_geom = Polygon(vertices)

            # Make legend entry              
            propref = 'regfilters.profile.%s.'%profile   
            proparameterstring = '\n'.join([ r'%s: $%s$'%(k.replace(propref,''),v) for i,(k,v) in enumerate(parameters.items()) if propref in k ])
            label = r'$\bf{%s/%s}$' % (basename,profile)
            label += '\n%s' % proparameterstring
            labels += [label]

            # Add the polygon to the map
            ax.add_geometries([polygon_geom], 
                              ccrs.PlateCarree(), 
                              edgecolor='C%s'%(len(labels)-1),  
                              facecolor='none', 
                              path_effects=path_effects,
                              linewidth=2)

        else:
            print(f"There is no polygon whose name is {polygonname}")


# Add a legend 
polygons = [ mpatches.Rectangle((0,0),0,0,edgecolor='C%s'%(l),facecolor='none',linewidth=2) for l,label in enumerate(labels)]
l = ax.legend(polygons,labels, 
              bbox_to_anchor=(1., 0), 
              loc='lower left')
d = ax.legend([],[], 
              title=date.today().strftime("%B %d, %Y"), 
              bbox_to_anchor=(0, 0), 
              loc='lower left')
polygons = [ mpatches.Rectangle((0, 0), 0, 0, edgecolor='none' ,  facecolor='none' ) for l,label in enumerate(parameterstring)]
ax.legend(polygons,parameterstring,
          bbox_to_anchor=(0., 1., 1., .102), 
          loc='lower left',
          mode='expand',
          ncol=2)
ax.add_artist(l)
ax.add_artist(d)



# ax.set_extent  only to the area of all polygons
ax.set_extent([min(lalo[0]),max(lalo[0]),min(lalo[1]),max(lalo[1])])

# Add background map 
ax.add_feature(cfeature.LAND)
ax.add_feature(cfeature.OCEAN)
ax.add_feature(cfeature.COASTLINE)
ax.add_feature(cfeature.BORDERS, linestyle=':')
ax.add_feature(cfeature.LAKES, alpha=0.5)
ax.add_feature(cfeature.RIVERS)

# Add gridlines
gl = ax.gridlines(draw_labels=True)
gl.right_labels = False

# save and close the plot
fig.savefig('polygon_map.png', 
            bbox_inches='tight', 
            transparent=True)
    
