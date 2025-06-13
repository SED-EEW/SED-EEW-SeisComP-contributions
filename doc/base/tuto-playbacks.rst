.. _PLAYBACKS:

============
Playbacks
============

Principle
---------

We provide containerized tools to reprocess data within a real-time simulation environment according to the original data timestamps.
This allows to evaluate realistic EEW delays for any event recorded with a representative network.
This also facilitates the testing of configuration parameters for improvement. 
This work was initiated by `Yannik Behr <https://github.com/yannikbehr/sc3-playback>`_ and extended by `Fred Massin <https://github.com/FMassin/scpbd/pkgs/container/scpbd>`_.

The playback in a docker comes with:

* a pre-configured scautoloc for rapid earthquake location
* a basic configuration for appropriate VS magnitude and origin association
* a basic FinDer configuration with generic symmetric crustal template (access upon request)

Containerization  
----------------

SeisComP's `msrtsimul <https://www.seiscomp.de/doc/apps/msrtsimul.html>`_ module simulates 
a real-time data acquisition by injecting miniSEED data from a file into the seedlink buffer.
Its usage requires configuring a dedicated seedlink server appropriately for `msrtsimul` and the related metadata.

Using our `scpbd <https://github.com/FMassin/scpbd/pkgs/container/scpbd>`_ tool (SeisComP playback in a docker), 
the playbacks can be performed without altering your system's configuration. Note that `scpbd` includes VS but not finder. 
If you are one of our external collaborators, you can also use our `finder <https://github.com/SED-EEW/FinDer/pkgs/container/finder>`_ 
container (requires permission) to perform playbacks that also include `finder`. This requires `docker <https://docs.docker.com/engine/install/>`_ and `ssh` to be installed and enabled.

In the following, we will use the variables ``IMAGE`` and ``TAG`` to remain general. You can define those according to the container you will use.

For the scpbd docker image (unrestricted access)::
    
    IMAGE="ghcr.io/fmassin/scpbd"
    TAG="main"

And for the finder docker image (restricted access)::

    IMAGE="ghcr.io/sed-eew/finder"
    TAG="v5.5.3.multipolygon"

.. note::
    The version of the finder docker image for playback (``TAG="v5.5.3.multipolygon"``) is currently different than the one 
    for :ref:`real time monitoring<DOCKERFINDER>` (``TAG="master"``).
    This will change shortly as we update all our playback containers to the latest SeisComP version.

#. First make sure to `authenticate to the container registry <https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry#authenticating-to-the-container-registry>`_. You need to generate a new Token (classic) from your github account with the ``read:packages`` option enabled:: 

    docker login $IMAGE

#. Download the docker image (only once):: 

    docker pull $IMAGE:$TAG 

#. Start the docker (only once or when updating the docker image). For old docker versions: replace ``host-gateway`` by ``$(ip addr show docker0 | grep -Po 'inet \K[\d.]+')``:: 

    docker stop scpbd && docker rm scpbd # That is in case you update an existing one 
    docker run -d \
        --add-host=host.docker.internal:host-gateway  \
        -p 18022:18000 -p 222:22 \
        --name scpbd \
        $IMAGE:$TAG 

#. Allow the container to access data from the host:: 
    
    docker exec -u 0 -it scpbd ssh-keygen -t rsa -N '' 
    docker exec -u 0 -it scpbd ssh-copy-id $USER@host.docker.internal


#. Define a shortcut function:: 

    scpbd () { docker exec -u 0  -it scpbd main $@ ; }


Configuration 
-------------

#. Enable the required SeisComP automatic processing modules::     
    
    docker exec -u sysop  -it scpbd /opt/seiscomp/bin/seiscomp enable \
    scautopick scamp scautoloc scevent sceewenv scvsmag scfinder

#. Import your own configuration (optional), using the user directory. These can be your `.cfg` files, bindings, `finder.config`, and finder mask::
    
    docker cp <path to your config file> scpbd:/home/sysop/.seiscomp/

#. Revise the default configuration::
    
    ssh -X -p 222 sysop@localhost /opt/seiscomp/bin/seiscomp exec scconfig 
    # nb: sysop password is "sysop"


Example dataset
---------------

To test the playback setup, you can use our `example dataset <https://doi.org/10.5281/zenodo.11192289>`_ 
from the MLh 4.3 earthquake on October 25, 2020 in Elm (Switzerland).


Usage
-----

.. note::
    File permissions: make sure everyone has reading rights on the data and metadata files you will use. 
    The station metadata should be formatted in the `fdsnxml` or `scml` format, and units should be `M`, `M/S`, or `M/S/S`.

#. Prepare the data. The mseed data should include at least 1 min of noise before the origin time (OT) and end at least 2 min after the OT. The record length must be 512 Bytes and can be prepared if needed with::

    ms512 () { python -c "from obspy import read; \
    read('$1').write('$2', format='MSEED', reclen=512); \
    exit()"; }
    ms512 data_not512.mseed data.mseed

#. Start the playback::

    # For a scml inventory format, use:
    scpbd $USER@host.docker.internal:$(pwd)/data.mseed \
    $USER@host.docker.internal:$(pwd)/inv.xml,scml
    
    # And for a fdsnxml inventory format:
    scpbd $USER@host.docker.internal:$(pwd)/data.mseed \
    $USER@host.docker.internal:$(pwd)/inv.xml,fdsnxml


#. (Optional) Check the data stream from another terminal::
    
    slinktool -Q localhost:18022 
    scrttv -I slink://localhost:18022 

#. Get and inspect the results::
    
    docker cp scpbd:/home/sysop/event_db.sqlite ./  
    scolv -I file://data.mseed \
    -d sqlite3://event_db.sqlite  \
    --offline --debug
