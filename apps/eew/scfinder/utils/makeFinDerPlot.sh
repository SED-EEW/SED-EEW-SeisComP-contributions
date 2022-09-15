#!/bin/bash

# This script is intended to be used with scalert, for the scripts.event option
# For this to work with scfinder in SeisCompP >= 4 you must:
# - enable and configure scxmldump
# - configure scevent with:
#     eventAssociation.magTypes = Mfd, Mfdl, Mfdr, M
#     eventAssociation.magPriorityOverStationCount = true
#     eventAssociation.minimumMagnitudes = 0
#     eventAssociation.enableFallbackMagnitude = true
# - enable and configure scevent
#
# scripts.event
# Type: string
# The script to be called when an event has been declared. The message string, a flag (1=new event, 
# 0=update event), the EventID, the arrival count and the magnitude (optional when set) are passed 
# as parameters $1, $2, $3, $4 and $5.

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

