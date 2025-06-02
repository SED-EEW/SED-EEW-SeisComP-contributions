.. _PLAYBACKS:

============
Playbacks
============

Principle
---------

We provide tools to reprocess data within a real-time simulation environment according to the original data timestamps. This allows to evaluate realistic EEW delays for any event recorded with a representative network. This also facilitates testing of configuration parameters for improvement. This work is based on https://github.com/yannikbehr/sc3-playback developed by @yannikbehr.

The playback in a docker comes with:
- a pre-configured scautoloc for rapid earthquake location
- basic configuration for appropriate VS magnitude amd origin association
- a basic FinDer configuration with generic symmetric crustal template (access upon request)

Containerization  
----------------

SeisComP's `msrtsimul <https://www.seiscomp.de/doc/apps/msrtsimul.html>_` module simulates a real-time data acquisition by injecting miniSEED data from a file into the seedlink buffer. Its usage requires configuring a dedicated seedlink server appropriately for msrtsimul and the related metadata.

Using our `scpd <https://github.com/FMassin/scpbd/pkgs/container/scpbd>` tool (SeisComP playback in a docker), the playbacks can be performed without altering your system's configuration. Note that scpd includes VS but not finder. If you are one of our external collaborators, you can also our finder docker container to perform playbacks that also include finder.

This requires `docker <https://docs.docker.com/engine/install/>`_ and ``ssh`` to be installed and enabled.

#. First make sure that you complete ``docker login ghcr.io/fmassin/scpbd`` (`authenticating to the container registry <https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry#authenticating-to-the-container-registry>`_). You need to generate a new Token (classic) from your github account with the ``read:packages`` option enabled. If you have been granted access to the finder container, use instead ``docker login ghcr.io/sed-eew/finder``

#. Download the scpbd/finder docker image (only once):: 

    docker pull ghcr.io/fmassin/scpbd:main 
    # or for finder:
    docker pull ghcr.io/sed-eew/finder:v5.5.3.multipolygon 

#. Start the docker (only once or when updating docker image. For old docker versions: replace ``host-gateway`` by ``$(ip addr show docker0 | grep -Po 'inet \K[\d.]+')``):: 

    docker stop scpdb && docker rm scpdb # That is in case you update an existing one 
    docker run -d \
        --add-host=host.docker.internal:host-gateway  \
        -p 18022:18000 -p 222:22 \
        --name scpbd \
        ghcr.io/fmassin/scpbd:main 
        # or for finder:
        ghcr.io/sed-eew/finder:v5.5.3.multipolygon


#. Allow the container to access data from the host:: 
    
    docker exec -u 0  -it scpbd ssh-keygen -t rsa -N '' 
    docker exec -u 0  -it scpbd ssh-copy-id $USER@host.docker.internal


#. Define a shortcut function:: 

    scpbd () { docker exec -u 0  -it scpbd main $@ ; }


Configuration 
-------------

#. Enable the required SeisComP automatic processing modules::     
    
    docker exec -u sysop  -it scpbd /opt/seiscomp/bin/seiscomp enable scautopick scamp scautoloc scevent sceewenv scvsmag scfinder

#. Import your own configuration, using the user directory::  
    
    docker cp <path to your config file> scpbd:/home/sysop/.seiscomp/

#. Revise the default configuration:
    
    ssh -X -p 222 sysop@localhost  /opt/seiscomp/bin/seiscomp exec scconfig
    # Nb: sysop password is ``sysop``

Usage
-----

#. Prepare the data::

    #The mseed data should include at least 1 min of noise before the origin time (OT) and end at least 2 min after the OT. The record length must be 512 Bytes and can be prepared if needed with:
    ms512 () { python -c "from obspy import read; read('$1').write('$2', format='MSEED', reclen=512) ; exit()" ; }
    ms512 data_not512.mseed data.mseed

    #The station metadata should be formatted in the fdsnxml or scml format, and units should be M, M/S, or M/S/S.

#. Start the playback::

    scpbd $USER@host.docker.internal:$(pwd)/data.mseed \
    $USER@host.docker.internal:$(pwd)/inv.xml,scml
    # for a scml inventory format, or:
    scpbd $USER@host.docker.internal:$(pwd)/data.mseed \
    $USER@host.docker.internal:$(pwd)/inv.xml,fdsnxml
    # for a fdsnxml inventory format.

#. (Optional) Check the data stream::
    
    slinktool -Q localhost:18022 
    scrttv -I slink://localhost:18022 

#. Get and inspect the results::
    
    docker cp scpbd:/home/sysop/event_db.sqlite ./  
    scolv -I file://data.mseed \
    -d sqlite3://event_db.sqlite  \
    --offline --debug

.. note::

    This is a placeholder

