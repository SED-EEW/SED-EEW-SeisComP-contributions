Part of the :ref:`EEW<EEW>` package (used by :ref:`VS<VS>` and :ref:`FinDer<FINDER>`).

*sceewlog*  logs the VS and FinDer magnitude messages received from :ref:`scvsmag` and 
:ref:`scfinder` (configurable), and broadcasts these alerts and generates and disseminates reports.

*sceewlog* is released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It requires the Python package
`dateutil`_ to be installed. sceewlog replaces scvslog that was originally part of the SeisComP implementation of the
`Virtual Seismologist`_ (VS) Earthquake Early Warning algorithm.

Once an event has timed out, the modules generates
report files. These report files are saved to disk and can also be sent via
email.

It also implements `ActiveMQ`_ and `Firebase Cloud Messaging`_ interfaces which can
send alert messages in real-time. It supports regionalized filter profiles (based on polygons and EQ parameter threshold values).
Currently, messages can be sent in four different formats (*sc3ml*, *qml1.2-rt*, *shakealertml* the UserDisplay format, *CAP1.2*). Optionally, a comment 
with ID **EEW** storing the EEW update number, can be added to each magnitude that
passes all configured filters and scoring.

The SED-ETHZ team provides a client that can display *qml1.2-rt* messages sent via ActiveMQ, the `Earthquake Early Warning Display (EEWD)`_
an OpenSource user interface developed within the European REAKT project and
based on early versions of the ShakeAlert `UserDisplay`_. 

To receive alerts with the EEWD set the format to *qml1.2-rt*. There are
currently no clients which can digest *sc3ml* and the *shakealertml* is not 
supported. Using pipelines alerts can be sent out in more than one format.

The real-time ActiveMQ interface requires the Python packages 
`stompy`_ and `lxml`_ to be installed.

It is beyond the scope of this documentation to explain the complete setup of an
ActiveMQ broker. However, since sceewlog uses the STOMP protocol to send
messages to the broker it is essential to add the following line
to configuration of the ActiveMQ broker.

.. code-block:: sh

   <connector>
   <serverTransport uri="stomp://your-server-name:your-port"/>
   </connector>

Please refer to `ActiveMQ`_ for setting up an ActiveMQ broker.


Firebase Cloud Messaging
========================
It is beyond the scope of this documentation to explain the complete setup 
Firebase Cloud Messaging. It sends EEW messages based on the magnitude prefered 
internally (see 
:ref:`the sceewlog internal scoring method<Magnitude Association and Scoring>`). 
In order to understand the Firebase Cloud Messaging interface see 
`Firebase Cloud Messaging`_ and `HTTP protocol`_. This interface is 
activated with:

.. code-block:: sh

   FCM.activate = true

In order to send notifications, an `authorization key`_ and a `notification topic`_ 
must be provided in a separate file (that can be configured with *FCM.dataFile*) 
in the following format:

.. code-block:: python 
   
   [AUTHKEY]
   key=YOUR-AUTHORIZATION-KEY_GOES_HERE
   [TOPICS]
   topic=YOUR_TOPIC_NAME_GOES_HERE


Reports
=======

Below is an example of the first few lines of a report file:

.. code-block:: sh

                                                                      |#St.   |                                                              
   Tdiff |Type|Mag.|Lat.  |Lon.   |Depth |origin time (UTC)      |Lik.|Or.|Ma.|Str.|Len. |Author   |Creation t.            |Tdiff(current o.)
   ------------------------------------------------------------------------------------------------------------------------------------------
     5.24| MVS|2.40| 46.05|   6.89| 20.53|2020-06-23T06:25:38.55Z|0.40|  4|  2|    |     |scvsmag2@|2020-06-23T06:25:45.99Z|  7.44
     6.24| MVS|3.69| 46.05|   6.89| 20.53|2020-06-23T06:25:38.55Z|0.40|  4|  4|    |     |scvsmag2@|2020-06-23T06:25:46.99Z|  8.45
     6.79| MVS|3.71| 46.05|   6.89| 20.53|2020-06-23T06:25:38.55Z|0.40|  4|  3|    |     |scvsmag@s|2020-06-23T06:25:47.54Z|  8.99
     7.24| MVS|3.65| 46.05|   6.89| 22.30|2020-06-23T06:25:38.33Z|0.99|  6|  5|    |     |scvsmag2@|2020-06-23T06:25:48.00Z|  9.67
     7.79| MVS|3.53| 46.05|   6.89| 22.30|2020-06-23T06:25:38.33Z|0.99|  6|  5|    |     |scvsmag@s|2020-06-23T06:25:48.54Z| 10.21
     8.24| MVS|3.61| 46.05|   6.89| 22.30|2020-06-23T06:25:38.33Z|0.99|  6|  5|    |     |scvsmag2@|2020-06-23T06:25:48.99Z| 10.66
     8.62| Mfd|4.00| 46.04|   6.88|  5.00|2020-06-23T06:25:41.93Z|0.88|  0|   |  80| 0.28|scfdalpin|2020-06-23T06:25:49.37Z|  7.44
     8.62| Mfd|3.90| 46.04|   6.88| 12.00|2020-06-23T06:25:40.29Z|0.85|  0|   | 140| 0.38|scfdforel|2020-06-23T06:25:49.37Z|  9.07

*Creation time* is the time the VS magnitude message was generated, *tdiff* is
the time difference between *creation time* and last *origin time* in seconds,
*lik.* is the likelihood that this event is a real event (see documentation of
:ref:`scvsmag`), *#St.(Or.)* is the number of stations that contributed to the
origin and  *#St.(Ma.)* the number of envelope streams that contributed to the
magnitude. *Str.* and *Len.* are the strike and length of the fault line
provided by :ref:`scfinder`.

Regionalized Filters
====================

To filter alerts to be sent out through ActiveMQ, it is necessary to set 
profiles on ActiveMQ section. Since this is using regions defined as closed 
polygons, then the first step is to provide a BNA file that contains the 
polygons. If the user does not provide a BNA file, then the other profile 
parameters will be evaluated globally.

.. code-block:: sh

   activeMQ.bnaFile = /opt/seiscomp3/share/sceewlog/closedpolygons.bna
   
Then profile names have to be set. Two profile examples are provided below.

.. code-block:: sh

   activeMQ.profiles = global, America
   
The **global** profile is not configured with polygon since this spans on the 
entire world. The **America** profile uses the "America" closed polygon defined 
in :confval:`activeMQ.bnaFile`.

.. code-block:: sh

   activeMQ.global.bnaPolygonName = none
   activeMQ.America.bnaPolygonName = America

The magnitude and likelihood threshold values might be:

.. code-block:: sh

   activeMQ.global.magThresh = 6.0
   activeMQ.global.likelihoodThresh = 0.5
   activeMQ.America.magThresh = 5.0
   activeMQ.America.likelihoodThresh = 0.3

There might also be a depth filter for each profile. The following parameters 
might be used to configure the **global** profile with shallow events, and 
the **America** profile with events from 0 to 100 km deep.

.. code-block:: sh

   activeMQ.global.minDepth = 0
   activeMQ.global.maxDepth = 33
   activeMQ.America.minDepth = 0
   activeMQ.America.maxDepth = 100

Finally, to avoid sending alerts for events outside of the network of interest 
for EEW applications, a :confval:`maxTime` can be set. The :confval:`maxTime` 
is the maximum delay in seconds between the magnitude creation time since the 
origin time. For the examples, on the **global** profile this parameter might 
be "-1" in order to skip this filter, whereas it could be set to 60 seconds for 
**America**. However, each of the :ref:`VS` and :ref:`FinDer` algorithms have 
their own default thresholds superseding :confval:`maxTime` defined in 
:ref:`sceewlog`.

.. code-block:: sh

   activeMQ.global.maxTime = -1
   activeMQ.America.maxTime = 60


Magnitude Association and Scoring
=================================
The magnitude association and scoring works similarly to :ref:`scevent` 
preferred-origin selection. The magnitude association scoring is activated 
with:

.. code-block:: sh
   
   magAssociation.activate = false
  
The following priorities are available:

.. code-block:: sh
  
   magAssociation.priority = magThresh,likelihood,authors

*magThresh* is a list of minimal magnitude to be allowed for each type of magnitude:

.. code-block:: sh
   
   magAssociation.typeThresh = Mfd:6,MVS:3.5,Mlv:2.5

The authors can be also used and its priority depends on the position on the list. For example:

.. code-block:: sh

   #if magAssociation.priority contains author then
   #the next parameter must contain valid magnitude authors' names
   magAssociation.authors = scvsmag@@@hostname@, \
   scvsmag0@@@hostname@, \
   scfd85sym@@@hostname@, \
   scfd20asym@@@hostname@, \
   scfdcrust@@@hostname@

In this list of authors the highest value is for *scvsmag* if it is the author of the magnitude evaluated. In this case, this author has a value of 6. The author value reduces after each comma separator. For the same example *scvsmag0* is 5, *scfd85sym* is 4, and so. If likelihood is listed on priorities then its value is added to the scoring list and at the end it is multiplied for the other priorities. Finally, for the scoring the number of arrival used to locate the event is added to the scoring list.

The final product of the score is:

    *score = magVal x author x likelihood x num. arrivals*

This score is set for each update. Score can be 0 in case that the magnitude value for a specific magnitude type is lower than the set on the magThresh.


Headline Change for CAP1.2 XML alerts
=====================================

The converted CAP1.2 xml alert messages contains a headline. The default 
headline is: 

.. code-block:: sh
   
   @AGENCY@ Magnitude X.X Date and Time (UTC): YYYY-MM-dd HH:mm:s.sssZ.

An alternative headline format might be preferred. The following alternative 
format can be selected:

.. code-block:: sh
   
   @AGENCY@/Earthquake Magnitude X.X, XX km NNW of SOMECITY, SOMECOUNTRY

The aternative format supports both spanish and english languages. The 
spanish version is:

.. code-block:: sh
   
   @AGENCY@/Sismo Magnitud X.X, XX km al SSO de SOMECITY, SOMECOUNTRY

The alternative format can be enable as follows:

.. code-block:: sh
   
   ActiveMQ.changeHeadline = true

The alternative format requires to specify the language and the corresponding 
file listing the world cities :confval:`ActiveMQ.hlCitiesFileCSV`. The language
can be selected as follows:

.. code-block:: sh
  
   #Uncomment the next line to select English
   ActiveMQ.hlLanguage = en-US
   #Uncomment the next line to select Spanish
   #ActiveMQ.hlLanguage = es-US

The file listing the world cities :confval:`ActiveMQ.hlCitiesFileCSV` must have 
the following format:

.. code-block:: sh
  
   city,country,lon,lat
   Tokyo,Japan,139.6922,35.6897
   Jakarta,Indonesia,106.8451,-6.2146
   Delhi,India,77.23,28.66
   Mumbai,India,72.8333,18.9667
   Manila,Philippines,120.9833,14.6
   Shanghai,China,121.4667,31.1667
   Sao Paulo,Brazil,-46.6339,-23.5504

Both an english and a spanish verion are provided in "@DATADIR@/sceewlog/world_cities_english.csv"
and "@DATADIR@/sceewlog/world_cities_spanish.csv".


References
==========

.. target-notes::

.. _`Virtual Seismologist` : http://www.seismo.ethz.ch/en/research-and-teaching/products-software/EEW/Virtual-Seismologist/
.. _`dateutil` : https://pypi.python.org/pypi/python-dateutil/
.. _`ActiveMQ` : http://activemq.apache.org/
.. _`Earthquake Early Warning Display (EEWD)` : http://www.seismo.ethz.ch/en/research-and-teaching/products-software/EEW/earthquake-early-warning-display-eewd/
.. _`UserDisplay` : http://www.eew.caltech.edu/research/userdisplay.html
.. _`stompy` : https://pypi.python.org/pypi/stompy/
.. _`lxml` : http://lxml.de/
.. _`Firebase Cloud Messaging` : https://firebase.google.com/docs/cloud-messaging
.. _`HTTP protocol` : https://firebase.google.com/docs/cloud-messaging/http-server-ref
.. _`authorization key` : https://stackoverflow.com/questions/37673205/what-is-the-authorization-part-of-the-http-post-request-of-googles-firebase-d
.. _`Notification by Topics` : https://firebase.google.com/docs/cloud-messaging/android/topic-messaging
