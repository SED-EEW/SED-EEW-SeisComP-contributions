#!/bin/bash

# bash script to generate station_mask_file "mask.nc" for FinDer
# call:
#./create_finder_mask.sh <station_lon_lat_file> <mask_station_distance> 
# e.g. 
#timeout -1 scfinder -I slink://hghgh:18000 --debug -u test --test 2>&1 |grep "debug] + .*  .*"|awk '{print $6, $5}' >stations
#./create_finder_mask.sh stations.xy 100

set -x

inputFile="$1"

if [[ $2 =~ ^(\.[0-9]+|[0-9]+(\.[0-9]*)?)$ ]]
then
    mask_station_distance="$2" 
    dkm=5.0 # resolution, needs to be the same as in templates

elif [ -f "$2" ]
then
    mask_station_distance=$( awk '$1~/MASK_STATION_DISTANCE/{print $2}' "$2" )

    tempcfg=$( awk '$1~/TEMPLATE_SETS_FILES/{print $3}' "$2" )
    dkm=$( awk '$1~/D_KM/{print $2}' "$( dirname $2 )/$tempcfg" )
else
    set +x
    echo "WRONG USAGE. TRY:"
    echo "timeout -1 scfinder -I slink://42.42.42.42:18000 --debug -u test --test 2>&1 |grep "debug] + .*  .*"|awk '{print $6, $5}' > <station_lon_lat_file>"
    echo "./create_finder_mask.sh <station_lon_lat_file> <mask_station_distance>" 
    exit 1
fi

M_PI=3.14159265359

#mask_border=$( bc -l <<< " $mask_station_distance / 75. " )  		# this is what is currently used in finder_initialize
mask_border=$( bc -l <<< "$mask_station_distance * 1.5 / 111.0 " ) 	# this adds a slightly larger border  

minlon=$( awk '{print $1}' $inputFile|sort -n|head -1 )
maxlon=$( awk '{print $1}' $inputFile|sort -n|tail -1 )
minlat=$( awk '{print $2}' $inputFile|sort -n|head -1 )
maxlat=$( awk '{print $2}' $inputFile|sort -n|tail -1 )

minlat=$( bc -l <<< " $minlat - $mask_border " )
maxlat=$( bc -l <<< " $maxlat + $mask_border " )
minlon=$( bc -l <<< " $minlon - $mask_border " )
maxlon=$( bc -l <<< " $maxlon + $mask_border " )

avlat=$( bc -l <<< " ($minlat + $maxlat) * 0.5 " )
arg=$( bc -l <<< " $avlat * $M_PI / 180. " )
dLatDegree=$( bc -l <<< " $dkm / 111.0 " )
dLonDegree=$( bc -l <<< " $dkm / (111.321 * c($arg)) " )

# cut-off at three significant figures of precision and make resolution slightly
# better than the image resolution.  I.e. divide by 1.25e6 rather than 1.0e6
dLatDegree=$( bc -l <<< " ($dLatDegree * 10^5) / (1.25 * 10^5) " )
dLonDegree=$( bc -l <<< " ($dLonDegree * 10^5) / (1.25 * 10^5) " )

gmt grdmath -I${dLonDegree}/${dLatDegree} -R${minlon}/${maxlon}/${minlat}/${maxlat} -fg ${inputFile} PDIST = tmp_out 

gmt grdclip tmp_out -Gmask.nc -Sa${mask_station_distance}/0 -Sb${mask_station_distance}/1 

gmt psbasemap -Bpxa5f5 -Bpya5f5 -JM5.5i -R${minlon}/${maxlon}/${minlat}/${maxlat} -X2 -Y6 -P -K -V > mask.ps
gmt grdimage mask.nc -JM5.5i -R${minlon}/${maxlon}/${minlat}/${maxlat}  -n+c -V -K -P -O -Q >> mask.ps
gmt pscoast -JM5.5i  -R${minlon}/${maxlon}/${minlat}/${maxlat} -W0.75p -Df -A10  -Na/0.75p -P -O -V >> mask.ps

ps2pdf mask.ps mask.pdf

rm tmp_out mask.ps

#gmt grdimage calculated_mask.nc -JM5.5i -R > reference_mask.ps
#ps2pdf reference_mask.ps reference_mask.pdf
#rm tmp_out reference_mask.ps
