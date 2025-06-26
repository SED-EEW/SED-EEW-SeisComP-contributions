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

#. Start the test docker (only once or when updating the docker image). For old docker versions: replace ``host-gateway`` by ``$(ip addr show docker0 | grep -Po 'inet \K[\d.]+')``:

   .. code-block:: bash
        
    docker stop scpbd && docker rm scpbd # That is in case you update an existing one 
    
   .. code-block:: bash
        
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
    
    docker exec -u sysop -it scpbd /opt/seiscomp/bin/seiscomp enable \
    scautopick scamp scautoloc scevent sceewenv scvsmag scfinder

#. (Optional) You can also activate sceewlog in the docker to get a report log of the alerts::

    docker exec -u sysop -it scpbd pip3 install python-dateutil  # library needed for sceewlog
    docker exec -u sysop -it scpbd /opt/seiscomp/bin/seiscomp enable sceewlog

#. Import your own configuration in the user directory. These can be your `.cfg` files, bindings, `finder.config`, and finder mask::
    
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

#. Set the `no_mask` option in the finder configuration file (i.e., `finder.config`) to avoid calculating a mask for this playback::

    # Note that this only works because we provide solely 
    # the data from relevant stations in the mseed file.
    FINDER_CONFIG="/usr/local/src/FinDer/config/finder.config"
    docker exec -u sysop -it scpbd sed -i.BAK \
    's/REGIONAL_MASK[[:space:]]\+calculate/REGIONAL_MASK no_mask/' $FINDER_CONFIG

#. Start the playback:
   
   .. code-block:: bash
    
    # For a scml inventory format, use:
    scpbd $USER@host.docker.internal:$(pwd)/data.mseed \
    $USER@host.docker.internal:$(pwd)/inv.xml,scml
    
   .. code-block:: bash

    # And for a fdsnxml inventory format:
    scpbd $USER@host.docker.internal:$(pwd)/data.mseed \
    $USER@host.docker.internal:$(pwd)/inv.xml,fdsnxml

   .. code-block:: bash
    
    # (Optional) Check that the data is streaming from another terminal:
    slinktool -Q localhost:18022 

#. Get and inspect the results::
    
    docker cp scpbd:/home/sysop/event_db.sqlite ./  
    scolv -I file://data.mseed \
    -d sqlite3://event_db.sqlite  \
    --offline --debug

#. If you activated the `sceewlog` module, you can also check the last report log::
    
    REPORT_PATH="/home/sysop/.seiscomp/log/VS_reports/"
    LAST_REPORT=$(docker exec -it scpbd ls -At $REPORT_PATH | head -n1 | tr -d '\r\n')
    docker cp scpbd:$REPORT_PATH$LAST_REPORT ./$LAST_REPORT
    cat $LAST_REPORT

Running the playback with the provided ELM dataset, on a relatively low-resource virtual machine (2 vcores and 8GB RAM), you can expect results such as::

                                                                           |#St.   |
    Tdiff |Type|Mag.|Lat.  |Lon.   |Depth |origin time (UTC)      |Lik.|Or.|Ma.|Str.|Len. |Author   |Creation t.            |Tdiff(current o.)
    ------------------------------------------------------------------------------------------------------------------------------------------
      5.61| MVS|4.25| 46.91|   9.23| 10.00|2020-10-25T19:35:40.84Z|0.99|  3|  3|    |     |scvsmag@7|2020-10-25T19:35:48.61Z|  7.77
      6.58| MVS|4.87| 46.91|   9.23| 10.00|2020-10-25T19:35:40.84Z|0.99|  3|  3|    |     |scvsmag@7|2020-10-25T19:35:49.58Z|  8.74
      7.58| MVS|4.05| 46.90|   9.13|  5.00|2020-10-25T19:35:42.35Z|0.40|  8|  8|    |     |scvsmag@7|2020-10-25T19:35:50.58Z|  8.23
      8.69| MVS|4.21| 46.90|   9.13|  5.00|2020-10-25T19:35:42.35Z|0.99|  8|  8|    |     |scvsmag@7|2020-10-25T19:35:51.70Z|  9.35
      9.68| MVS|4.23| 46.90|   9.13|  5.00|2020-10-25T19:35:42.35Z|0.99|  8|  8|    |     |scvsmag@7|2020-10-25T19:35:52.68Z| 10.33
     10.69| MVS|4.42| 46.90|   9.13|  5.00|2020-10-25T19:35:42.35Z|0.99|  8|  8|    |     |scvsmag@7|2020-10-25T19:35:53.69Z| 11.34
     11.81| Mfd|4.50| 46.95|   9.13| 10.00|2020-10-25T19:35:41.92Z|0.71|  0|  3|  70| 1.30|scfinder@|2020-10-25T19:35:54.81Z| 12.89
     11.98| MVS|3.78| 46.91|   9.13|  5.00|2020-10-25T19:35:42.55Z|0.99| 28| 28|    |     |scvsmag@7|2020-10-25T19:35:54.98Z| 12.43
     12.95| MVS|3.78| 46.91|   9.13|  5.00|2020-10-25T19:35:42.55Z|0.99| 28| 28|    |     |scvsmag@7|2020-10-25T19:35:55.95Z| 13.40
     13.50| Mfd|4.50| 46.95|   9.20| 10.00|2020-10-25T19:35:41.80Z|0.79|  0|  3|  45| 1.30|scfinder@|2020-10-25T19:35:56.51Z| 14.71
     13.94| MVS|3.86| 46.91|   9.13|  5.00|2020-10-25T19:35:42.55Z|0.99| 28| 28|    |     |scvsmag@7|2020-10-25T19:35:56.94Z| 14.39
     14.64| MVS|3.87| 46.91|   9.13|  5.00|2020-10-25T19:35:42.55Z|0.99| 28| 28|    |     |scvsmag@7|2020-10-25T19:35:57.64Z| 15.09
     14.74| Mfd|4.50| 46.96|   9.17| 10.00|2020-10-25T19:35:41.77Z|0.81|  0|  3|  45| 1.30|scfinder@|2020-10-25T19:35:57.75Z| 15.98
     15.85| Mfd|4.50| 46.96|   9.16| 10.00|2020-10-25T19:35:41.79Z|0.81|  0|  3|  45| 1.30|scfinder@|2020-10-25T19:35:58.85Z| 17.05
     16.01| MVS|3.55| 46.90|   9.14|  5.00|2020-10-25T19:35:42.70Z|0.99| 48| 44|    |     |scvsmag@7|2020-10-25T19:35:59.01Z| 16.31
     16.83| MVS|3.52| 46.90|   9.14|  5.00|2020-10-25T19:35:42.70Z|0.99| 48| 45|    |     |scvsmag@7|2020-10-25T19:35:59.84Z| 17.13
     18.21| MVS|3.38| 46.89|   9.14|  5.00|2020-10-25T19:35:42.79Z|0.99| 68| 53|    |     |scvsmag@7|2020-10-25T19:36:01.22Z| 18.42
     18.94| MVS|3.36| 46.89|   9.14|  5.00|2020-10-25T19:35:42.79Z|0.99| 68| 54|    |     |scvsmag@7|2020-10-25T19:36:01.95Z| 19.15
     20.01| MVS|3.33| 46.89|   9.14|  5.00|2020-10-25T19:35:42.79Z|0.99| 68| 55|    |     |scvsmag@7|2020-10-25T19:36:03.01Z| 20.22
     20.92| Mfd|4.50| 46.92|   9.13| 10.00|2020-10-25T19:35:42.14Z|0.88|  0|  3|  45| 1.30|scfinder@|2020-10-25T19:36:03.92Z| 21.78
     20.98| MVS|3.34| 46.89|   9.14|  5.00|2020-10-25T19:35:42.79Z|0.99| 68| 55|    |     |scvsmag@7|2020-10-25T19:36:03.98Z| 21.19
     22.09| MVS|3.23| 46.89|   9.14|  5.00|2020-10-25T19:35:42.87Z|0.99| 88| 65|    |     |scvsmag@7|2020-10-25T19:36:05.09Z| 22.22
     23.15| MVS|3.24| 46.89|   9.14|  5.00|2020-10-25T19:35:42.87Z|0.99| 88| 65|    |     |scvsmag@7|2020-10-25T19:36:06.15Z| 23.28
     24.04| MVS|3.24| 46.89|   9.14|  5.00|2020-10-25T19:35:42.87Z|0.99| 88| 65|    |     |scvsmag@7|2020-10-25T19:36:07.04Z| 24.17
     25.01| MVS|3.24| 46.89|   9.14|  5.00|2020-10-25T19:35:42.87Z|0.99| 88| 65|    |     |scvsmag@7|2020-10-25T19:36:08.01Z| 25.14
     26.31| MVS|3.24| 46.89|   9.14|  5.00|2020-10-25T19:35:42.92Z|0.99|108| 66|    |     |scvsmag@7|2020-10-25T19:36:09.31Z| 26.39
     27.26| MVS|3.25| 46.89|   9.14|  5.00|2020-10-25T19:35:42.92Z|0.99|108| 66|    |     |scvsmag@7|2020-10-25T19:36:10.26Z| 27.34
     28.11| MVS|3.25| 46.89|   9.14|  5.00|2020-10-25T19:35:42.92Z|0.99|108| 66|    |     |scvsmag@7|2020-10-25T19:36:11.11Z| 28.18
     29.08| MVS|3.25| 46.89|   9.14|  5.00|2020-10-25T19:35:42.92Z|0.99|108| 66|    |     |scvsmag@7|2020-10-25T19:36:12.09Z| 29.16
     30.00| MVS|3.25| 46.89|   9.14|  5.00|2020-10-25T19:35:42.92Z|0.99|108| 66|    |     |scvsmag@7|2020-10-25T19:36:13.00Z| 30.08
                                                                                                                                                                           4,127         Top