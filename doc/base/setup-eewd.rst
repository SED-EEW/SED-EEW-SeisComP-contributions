.. _EEWD:

==========
EEWD setup
==========

The `Earthquake Early Warning Display`_ (EEWD, Cauzzi et al., 2016) is an open-source Java application, that can receive and display
EEW messages broadcast via `ActiveMQ`_. It is not part of the :ref:`EEW<EEW>` package. 

Installation
------------

.. note::
    
    Make sure all ports used by `ActiveMQ`_ are closed to public access, connection should only be allowed via VPN and from specific client identified by their fixed IP. Do not enable `ActiveMQ`_ otherwise.


#. Install and enable `ActiveMQ`_ (requires superuser privileges):: 

    apt install activemq defaultjava-jre
    ln -s /etc/activemq/instances-available/main /etc/activemq/instances-enabled/main
    systemctl status activemq.service


#. Download the `Earthquake Early Warning Display`_ software::

    curl -L https://github.com/SED-EEW/EEWD/releases/download/latest/eewd.zip > eewd.zip
    unzip eewd.zip


Configuration
-------------

ActiveMQ configuration
^^^^^^^^^^^^^^^^^^^^^^
.. note::

    The following assumes that :ref:`sceewlog` would provide EEW solutions via the `ActiveMQ`_  topic ``/topic/your-topic-for-alert`` on port ``61619`` (adjust the username and password consistently).


#. Setup `ActiveMQ`_ with the following in the ``broker`` element of ``/etc/activemq/instances-enabled/main/activemq.xml``:: 

     <plugins xmlns:spring="http://www.springframework.org/schema/beans">
          <simpleAuthenticationPlugin>
               <users>
                    <authenticationUser username="your-ActiveMQ-username" password="your-ActiveMQ-password" groups="everyone,admins,sysWriters" />
               </users>
          </simpleAuthenticationPlugin>
          <authorizationPlugin>
               <map>
                    <authorizationMap>
                         <authorizationEntries>
                              <authorizationEntry queue=">" read="admins" write="admins" admin="admins" />
                              <authorizationEntry topic=">" read="admins" write="admins" admin="admins" />
                              <authorizationEntry topic="your-ActiveMQ-topics"   read="everyone" write="sysWriters" admin="sysWriters,admins" />
                              <authorizationEntry topic="ActiveMQ.Advisory.>" read="everyone" write="everyone" admin="everyone"/>
                         </authorizationEntries>
                         <tempDestinationAuthorizationEntry>  
                              <tempDestinationAuthorizationEntry read="tempDestinationAdmins" write="tempDestinationAdmins" admin="tempDestinationAdmins"/>
                         </tempDestinationAuthorizationEntry>               
                    </authorizationMap>
               </map>
          </authorizationPlugin>
     </plugins>
     <transportConnectors>
          <transportConnector name="stomp" uri="stomp://0.0.0.0:61618?maximumConnections=1000&amp;wireFormat.maxFrameSize=104857600"/>
          <transportConnector name="openwire" uri="tcp://0.0.0.0:61616?maximumConnections=1000&amp;wireFormat.maxFrameSize=104857600"/>
     </transportConnectors>


#. Restart `ActiveMQ`_  (requires superuser privileges)::

    systemctl status activemq.service


#. Restart :ref:`sceewlog` and check successful connection in the log (``.seiscomp/log/sceewlog.log``)::

    seiscomp restart sceewlog


EEWD configuration
^^^^^^^^^^^^^^^^^^

#. Setup `Earthquake Early Warning Display`_ with the required adjustments in:

   #. ``eewd/data/targets.csv``: The list of EEW target sites.
   #. ``eewd/data/stations.csv``: The list of seismic stations.
   #. ``eewd/eewd.properties``: The EEWD parameters, e.g., connection credential.
   #. ``eewd/openmap.properties``: The map parameters, e.g., background map.

#. Start `Earthquake Early Warning Display`_ and check successful connection to `ActiveMQ`_ (in the window title, e.g., ``EEWD (connected to ...``) and :ref:`sceewlog` (in the lower-right window corner, e.g., ``hearbeat: ...`` updating every 5s)::

    cd eewd && java -jar eewd.jar


References
----------

Cauzzi, C., Behr, Y. D., Clinton, J., Kastli, P., Elia, L., & Zollo, A. (2016)
     An Open-Source Earthquake Early Warning Display. Seismological Research
     Letters, 87(3), 737–742, doi:10.1785/0220150284

.. target-notes::

.. _`Earthquake Early Warning Display` : https://github.com/SED-EEW/EEWD

.. _`ActiveMQ` : https://activemq.apache.org

