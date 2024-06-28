.. _DOCKERFINDER:

============
FinDer setup
============

Containerization  
----------------

This requires `docker <https://docs.docker.com/engine/install/>`_ and ``ssh`` to be installed and enabled.  

#. First make sure that you complete ``docker login ghcr.io/sed-eew/finder`` (`authenticating to the container registry <https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry#authenticating-to-the-container-registry>`_)
#. Start the docker (only once or when updating docker image, old docker version: replace ``host-gateway`` by ``$(ip addr show docker0 | grep -Po 'inet \K[\d.]+')``):: 

    docker stop finder && docker rm finder # That is in case you update an existing one 
    docker run -d \
        --add-host=host.docker.internal:host-gateway  \
        -p 9878:22 \
        -v finder:/home/sysop \
        --name finder \
        ghcr.io/sed-eew/finder:master


#. Setup ``ssh`` authentification key (see ``ssh-keygen``) and define a shortcut function to manage :ref:`scfinder` (and SeisComP) inside the docker container (once per host session, or add to your :file:`.profile` or :file:`.bashrc` to make it permanent):: 

    ssh-copy-id  -p 9878 sysop@localhost
    seiscomp-finder () { ssh -X -p 9878 sysop@localhost -C "/opt/seiscomp/bin/seiscomp  $@"; }


Configuration 
-------------

#. Configure :ref:`finder` based on the example in :file:`/usr/local/src/FinDer/config/finder.config`.  e.g.:: 

    # Basic docker configuration 
    mkdir myconf
    docker cp  finder:/usr/local/src/FinDer/config/finder.config  myconf/ 
    
    # Edit myconf/finder.config with paths related to container
    
    # Copy your FinDer config to container
    docker cp myconf/finder.config finder:/home/sysop/.seiscomp/

#. Configure :ref:`scfinder` (and SeisComP) with the ``seiscomp-finder`` shortcut, e.g.:: 

    # Basic scfinder container configuration 
    mkdir myconf
    echo 'connection.server = host.docker.internal/production
    database = host.docker.internal/seiscomp
    finder.config = /home/sysop/.seiscomp/finder.config ' > myconf/scfinder.cfg

    ## In case the db setup is more complex you might need to adjust the following and add it to the above:
    #database.inventory = mysql://host.docker.internal/seiscomp
    #database.config = mysql://host.docker.internal/seiscomp
    
    # Copy your scfinder config to container
    docker cp myconf/scfinder.cfg finder:/home/sysop/.seiscomp/

    # Review and adjust configuration
    seiscomp-finder exec scconfig


#. Backup your configuration, e.g.,::
    
    docker cp  finder:/home/sysop/.seiscomp/finder.config  myconf/ 
    docker cp  finder:/home/sysop/.seiscomp/scfinder.cfg myconf/


.. note::

    ``host.docker.internal`` is defined as a alias to the docker host that can be used in :ref:`scfinder` 
    configuration (:file:`scfinder.cfg`) in the docker container with parameter :confval:`connection.server` 
    to connect to SeisComP host system. If the host :ref:`scmaster` does not forward database parameters, 
    :confval:`database`, :confval:`database.config`, and :confval:`database.inventory` parameters using the 
    ``host.docker.internal`` docker host alias might also be needed.

    Alternatively :ref:`scimex` could be configured to push origins and magnitudes from :ref:`scfinder` 
    from within the docker to another SeisComP system.


Operation
---------

#. Manage :ref:`scfinder` (and SeisComP) with the ``seiscomp-finder`` shortcut, e.g.::

    # debug and test:
    seiscomp-finder exec scfinder --debug

    # enable modules
    seiscomp-finder enable scfinder 

    # restart modules
    seiscomp-finder restart    


#. Eventually, after restarting docker or the host system, once the ``seiscomp-finder`` alias is permanent, restart the finder container and its seiscomp as follows::
    
    # restart docker container 
    docker start finder
    docker exec -u 0 -it  finder /etc/init.d/ssh start 
    seiscomp-finder restart


.. note::
    
    You may also use FinDer without SeisComP with :file:`/usr/local/src/FinDer/finder_file` and related 
    utilities in ``/usr/local/src/FinDer/``.



