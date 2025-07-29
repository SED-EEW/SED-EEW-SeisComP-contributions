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
* running scfinder once with the mask creation switch activated
* setting the path to the generated mask in the finder configuration file

.. warning::

    The following steps will only work if you use a recent version of the finder docker image (downloaded after July 28 2025).
    Make sure to update you finder image as needed.   

..  
    This procedure is only valid for the default generic templates for which the grid resolution is set to 5 km.
    If using a custom template with a different grid resolution, the mask generation script (see below) should be modified accordingly. 

Setup 
-----

#. Log into the docker container to access the configuration files::

    docker exec -u sysop -w /home/sysop/.seiscomp -it finder bash

#. Select the stations to enable or disable in ``scfinder.cfg`` (all the inventory is enabled by default):
    Use vim to set or edit the `streams.whitelist <https://docs.gempa.de/sed-eew/current/apps/scfinder.html#confval-streams.whitelist>`_ and
    `streams.blacklist <https://docs.gempa.de/sed-eew/current/apps/scfinder.html#confval-streams.blacklist>`_ parameters. 

#. (Optional) Change the default ``MASK_STATION_DISTANCE`` parameter:
    This parameter is defined in the ``finder.config`` file (default to 75.0 km) and will be used to generate the mask. You can adapt it according to your network density.

#. Generate the Mask by runing scfinder with the ``--calculate-mask`` option and the path to the output file::

    scfinder --calculate-mask ./finder_mask.nc --debug --offline

#. Add the path to the new mask file in the ``finder.config`` configuration file:
    Set the ``REGIONAL_MASK`` parameter to: ``REGIONAL_MASK /home/sysop/.seiscomp/finder_mask.nc`` 


Visualization 
-------------

#. Extract the grid boundaries from the mask file into variables (requires GMT)::

    # You may need to adjust the commands depending on the GMT version
    read minlon maxlon < <(gmt grdinfo finder_mask.nc | awk '/x_min:/ {print $3, $5}')
    read minlat maxlat < <(gmt grdinfo finder_mask.nc | awk '/y_min:/ {print $3, $5}')

#. Create a postscript image::

    gmt psbasemap -Bpxa5f5 -Bpya5f5 -JM5.5i -R${minlon}/${maxlon}/${minlat}/${maxlat} -X2 -Y6 -P -K -V > mask.ps
    gmt grdimage finder_mask.nc -JM5.5i -R${minlon}/${maxlon}/${minlat}/${maxlat}  -n+c -V -K -P -O -Q >> mask.ps
    gmt pscoast -JM5.5i  -R${minlon}/${maxlon}/${minlat}/${maxlat} -W0.75p -Df -A10  -Na/0.75p -P -O -V >> mask.ps

#. Copy the postscript file to the host for visualization::

    # From a terminal on host
    docker cp finder:/home/sysop/.seiscomp/mask.ps ./
    
#. Convert to pdf if needed (requires ghostscript)::
    
    gs -sDEVICE=pdfwrite -dNOPAUSE -dBATCH -sOutputFile=mask.pdf mask.ps
