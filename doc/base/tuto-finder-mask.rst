.. _FINDER_MASK:

============
FinDer Mask
============

Principle
---------

FinDer interpolates the recorded acceleration amplitudes in the area of interest before applying the template matching algorithm for source estimation.
The area of interest for FinDer is restricted via a custom mask: the mask enabled region is constructed as the union of the disks of radius
`MASK_STATION_DISTANCE` centered on every enabled stations. In other words, any interpolated point that is located more than `MASK_STATION_DISTANCE` away 
from every enabled station is not considered for FinDer source estimation.

The setup of the FinDer Mask requires:

* setting up a white list and/or black list of the stations to use or discard with finder
* the generation of a station file based on the inventory filtered through the lists above
* running the mask creation script with the the station file and the finder configuration file as input


Setup 
-----

#. Select the stations to enable in `scfinder.cfg`::     
    
    mkdir myconf
    # Copy you current configuration from the finder container
    docker cp finder:/home/sysop/.seiscomp/scfinder.cfg myconf/

    # Set or edit the `streams.whitelist <https://docs.gempa.de/sed-eew/current/apps/scfinder.html#confval-streams.whitelist>`_ and \
        `streams.blacklist <https://docs.gempa.de/sed-eew/current/apps/scfinder.html#confval-streams.blacklist>`_ parameters 

    # Copy your scfinder config back to the to container
    docker cp myconf/scfinder.cfg finder:/home/sysop/.seiscomp/


#. (Optional) Change the default `MASK_STATION_DISTANCE` parameter::

    # This parameter is defined in the `finder.config` file (default to 75.0 km) and will be used to generate the mask. You can adjust it to your needs.
    docker cp finder:/home/sysop/.seiscomp/finder.config myconf/

    # edit the file to change the `MASK_STATION_DISTANCE` parameter
    
    docker cp myconf/scfinder.cfg finder:/home/sysop/.seiscomp/

#. Generate the effective station file::
    
    .. warning::

        The following steps will produce a correct mask only if you use a recent version of the finder docker image (>?).
        Make sure to update you finder image (add link) as needed.   

    # Run the startup sequence of scfinder in debug mode to get a list of the enabled stations and verify that all the active stations are displayed as [] \
    # Note that two lists are actually displayed, and we are interested in the second list that includes the station coordinates.
    # Make sure those complete list are fully captured in the (increase the timeout as needed): 
    
    docker exec -it finder timeout -3 scfinder --debug --offline 2>&1 \
        | grep "debug] + .*" > stations_lists.txt
        
        #| grep "debug] + .*  .*" | awk '{print $6, $5}' > /home/sysop/.seiscomp/stations.xy
    #docker exec -it finder scfinder ... 
    
    # Repeat the same command and redirect the active station coordinates to a file:
    ... > /home/sysop/.seiscomp/stations.xy

#. Generate Mask itself::

    # Copy the mask generation script to 
    
    ssh -X -p 222 sysop@localhost /opt/seiscomp/bin/seiscomp exec scconfig 
    # nb: sysop password is "sysop"


Mask visu, gen image?
---------------

                                                                                                                                                                          4,127         Top