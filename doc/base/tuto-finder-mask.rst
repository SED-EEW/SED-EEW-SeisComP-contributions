.. _FINDER_MASK:

============
FinDer Mask
============

Principle
---------

FinDer interpolates the recorded acceleration amplitudes in the area of interest before applying the template matching algorithm for estimating the source parameters.
The area of interest for FinDer is restricted via a custom mask: the mask enabled region is constructed as the union of the disks of radius
``MASK_STATION_DISTANCE`` centered on every enabled stations. In other words, any interpolated point that is located more than ``MASK_STATION_DISTANCE`` away 
from every enabled station is not considered for FinDer source estimation.

The setup of the FinDer Mask requires:

* defining a white list and/or black list of the stations to use or discard with finder
* the generation of a station file based on the inventory filtered through the lists above
* running the mask creation script with the the station file and the ``MASK_STATION_DISTANCE`` parameter as input

.. warning::
    
    This procedure is only valid for the default generic templates for which the grid resoltution is set to 5 km.
    If using a custom template with a different grid resolution, the mask generation script (see below) should be modified accordingly. 

.. warning::

    The following steps will produce a correct mask only if you use a recent version of the finder docker image (downloaded after July 7th 2025).
    Make sure to update you finder image as needed.   

Setup 
-----

..
    mkdir myconf
    # Copy your current configuration from the finder container
    docker cp finder:/home/sysop/.seiscomp/scfinder.cfg myconf/

    # Copy your scfinder config back to the to container
    docker cp myconf/scfinder.cfg finder:/home/sysop/.seiscomp/

    docker cp finder:/home/sysop/.seiscomp/finder.config myconf/
    docker cp myconf/finder.config finder:/home/sysop/.seiscomp/
    docker exec -u sysop -it finder timeout 3 /opt/seiscomp/bin/seiscomp exec scfinder --debug --offline 2>&1
    docker exec -u sysop -it finder timeout 3 /opt/seiscomp/bin/seiscomp exec scfinder --debug --offline 2>&1 \
        | grep "debug] + .* .*" | tr -d '\r' | awk '{print $6, $5}' > myconf/stations.xy
    Check the station file and copy to the finder container: 
    docker cp myconf/stations.xy finder:/home/sysop/.seiscomp/stations.xy
    docker exec -it finder apt-get install bc ghostscript
    docker exec -u sysop -w /home/sysop/.seiscomp -it finder bash /opt/seiscomp/share/FinDer/makeFinderMask.sh /home/sysop/.seiscomp/stations.xy $MASK_STATION_DISTANCE
    FINDER_CONFIG="/home/sysop/.seiscomp/finder.config"
    docker exec -u sysop -it finder sed -i.BAK \
    's|REGIONAL_MASK .*|REGIONAL_MASK /home/sysop/.seiscomp/mask.nc|' $FINDER_CONFIG

#. Log into the docker container to access the finder configuration files and install a text editor of your choice::

    docker exec -w /home/sysop/.seiscomp -it finder bash
    apt-get install vim

#. Select the stations to enable or disable in ``scfinder.cfg``:
    Set or edit the `streams.whitelist <https://docs.gempa.de/sed-eew/current/apps/scfinder.html#confval-streams.whitelist>`_ and
    `streams.blacklist <https://docs.gempa.de/sed-eew/current/apps/scfinder.html#confval-streams.blacklist>`_ parameters. 


#. (Optional) Change the default ``MASK_STATION_DISTANCE`` parameter:
    This parameter is defined in the ``finder.config`` file (default to 75.0 km) and will be used to generate the mask. You can adapt it according to your network density.
    Edit the file to change the ``MASK_STATION_DISTANCE`` parameter and setup a corresponding local variable for later use::
        
        export MASK_STATION_DISTANCE=75.0  # Use the value you set in finder.config


#. Generate the effective station file:
    Run the startup sequence of scfinder in debug mode to get a list of the enabled stations and verify that all the active stations are displayed.
    Note that two lists are actually displayed, and we are only interested in the second list that includes the station coordinates (increase the timeout as needed)::

        su sysop  # Run the seiscomp commands as the sysop user
        timeout 3 seiscomp exec scfinder --debug --offline 2>&1
    
    Capture a list of the stations coordinates::
    
        timeout 3 seiscomp exec scfinder --debug --offline 2>&1 \
            | grep "debug] + .* .*" | tr -d '\r' | awk '{print $6, $5}' > stations.xy
        exit  # To go back to the root user


#. Generate the Mask itself:
    Install the `bc` command line calculator and ghostscript (optional, to generate the mask image) in the finder container::
        
        apt-get install bc ghostscript

    Run the mask generation script::
        
        /opt/seiscomp/share/FinDer/makeFinderMask.sh stations.xy $MASK_STATION_DISTANCE

    (Optional) copy the mask image to host for visualization::
        
        # From another terminal on host:
        docker cp finder:/home/sysop/.seiscomp/mask.pdf ./

    Add the path to the new mask file in the ``finder.config`` configuration file:
        Set the ``REGIONAL_MASK`` parameter to: ``REGIONAL_MASK /home/sysop/.seiscomp/mask.nc`` 
    