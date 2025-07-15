.. _FINDER_MASK:

============
FinDer Mask
============

Principle
---------

FinDer interpolates the recorded acceleration amplitudes in the area of interest before applying the template matching algorithm for estimating the source parameters.
The area of interest for FinDer is restricted via a custom mask: the mask enabled region is constructed as the union of the disks of radius
`MASK_STATION_DISTANCE` centered on every enabled stations. In other words, any interpolated point that is located more than `MASK_STATION_DISTANCE` away 
from every enabled station is not considered for FinDer source estimation.

The setup of the FinDer Mask requires:

* defining a white list and/or black list of the stations to use or discard with finder
* the generation of a station file based on the inventory filtered through the lists above
* running the mask creation script with the the station file and the `MASK_STATION_DISTANCE` parameter as input

.. note::
    
    This procedure is only valid for the default generic templates for which the grid resoltution is set to 5 km.
    If using a custom template with a different grid resolution, the mask generation script (see below) should be modified accordingly. 


Setup 
-----

#. Select the stations to enable in `scfinder.cfg`::
    
    mkdir myconf
    # Copy your current configuration from the finder container
    docker cp finder:/home/sysop/.seiscomp/scfinder.cfg myconf/

    # Set or edit the `streams.whitelist <https://docs.gempa.de/sed-eew/current/apps/scfinder.html#confval-streams.whitelist>`_ and \
        `streams.blacklist <https://docs.gempa.de/sed-eew/current/apps/scfinder.html#confval-streams.blacklist>`_ parameters 

    # Copy your scfinder config back to the to container
    docker cp myconf/scfinder.cfg finder:/home/sysop/.seiscomp/


#. (Optional) Change the default `MASK_STATION_DISTANCE` parameter::

    # This parameter is defined in the `finder.config` file (default to 75.0 km) and will be used to generate the mask. You can adapt it according to your network density.
    docker cp finder:/home/sysop/.seiscomp/finder.config myconf/

    # Edit the file to change the `MASK_STATION_DISTANCE` parameter and setup a corresponding local variable for later use:
    export MASK_STATION_DISTANCE=75.0  # Use the value you set in finder.config

    docker cp myconf/finder.config finder:/home/sysop/.seiscomp/

#. Generate the effective station file::
    
    .. warning::

        The following steps will produce a correct mask only if you use a recent version of the finder docker image (downloaded after July 7th 2025).
        Make sure to update you finder image as needed.   

    # Run the startup sequence of scfinder in debug mode to get a list of the enabled stations and verify that all the active stations are displayed.
    # Note that two lists are actually displayed, and we are only interested in the second list that includes the station coordinates (increase the timeout as needed):
    docker exec -u sysop -it finder timeout 3 /opt/seiscomp/bin/seiscomp exec scfinder --debug --offline 2>&1
    
    # Capture a list of the stations coordinates:
    docker exec -u sysop -it finder timeout 3 /opt/seiscomp/bin/seiscomp exec scfinder --debug --offline 2>&1 \
        | grep "debug] + .* .*" | tr -d '\r' | awk '{print $6, $5}' > myconf/stations.xy

    # Check the station file and copy to the finder container: 
    docker cp myconf/stations.xy finder:/home/sysop/.seiscomp/stations.xy


#. Generate the Mask itself::
    
    # Install the `bc` command line calculator and ghostscript (optional, to generate the mask image) in the finder container:
    docker exec -it finder apt-get install bc ghostscript

    # Run the mask generation script inside the container:
    docker exec -u sysop -w /home/sysop/.seiscomp -it finder bash /opt/seiscomp/share/FinDer/makeFinderMask.sh /home/sysop/.seiscomp/stations.xy $MASK_STATION_DISTANCE

    # Copy and check the mask image:
    docker cp finder:/home/sysop/.seiscomp/mask.pdf myconf/

    # Add the path to the mask file in the finder configuration:
    FINDER_CONFIG="/home/sysop/.seiscomp/finder.config"
    docker exec -u sysop -it finder sed -i.BAK \
    's|REGIONAL_MASK .*|REGIONAL_MASK /home/sysop/.seiscomp/mask.nc|' $FINDER_CONFIG
    