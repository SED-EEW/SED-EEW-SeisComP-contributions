#!/bin/bash

########
# Copyright (C) by ETHZ/SED 
#  
# This program is free software: you can redistribute it and/or modify 
# it under the terms of the GNU Affero General Public License as published 
# by the Free Software Foundation, either version 3 of the License, or 
# (at your option) any later version. 
#  
# This program is distributed in the hope that it will be useful, 
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU Affero General Public License for more details. 
#  
# Author of the Software: Jen Andrews (GNS)
########

########
# This script is intended to be used with scalert, with the scripts.event option.
# For this to work with scfinder in SeisCompP >= 4 you must:
# - configure scxmldump
# - enable and configure scevent with:
#     eventAssociation.magTypes = Mfd, M
#     eventAssociation.magPriorityOverStationCount = true
#     eventAssociation.minimumMagnitudes = 0
#     eventAssociation.enableFallbackMagnitude = true
# - enable and configure scalert with:
#     # The script to be called when an event has been declared. The message string, a flag (1=new event, 
#     # 0=update event), the EventID, the arrival count and the magnitude (optional when set) are passed 
#     # as parameters $1, $2, $3, $4 and $5.
#     scripts.event = <path to makeFinDerPlot.sh>
#     # Type: string
########

base=$(basename -- "$0")
logfile=${base%.*}".log"
evscript=${base%.*}".py"

echo $# $* >> $logfile
comment=$1
flag=$2
evid=$3
arr=$4
mag=$5
if [ $flag == "1" ]; then
  python3 $evscript $evid >> $logfile &
else
  echo 'Not processing event update' >> $logfile
fi

