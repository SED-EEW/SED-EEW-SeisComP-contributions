======================
SCEEWLOG DOCUMENTATION
======================

INTRODUCTION
============

Part of the :ref:`EEW` package.

*sceewlog*  logs the VS and FinDer magnitude messages received from :ref:`scvsmag` and 
:ref:`scfinder` (configurable), and broadcasts these alerts and generates and disseminates reports.

*sceewlog* is released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It requires the Python package
`dateutil`_ to be installed. sceewlog replaces scvslog that was originally part of the SeisComP implementation of the
`Virtual Seismologist`_ (VS) Earthquake Early Warning algorithm.

Once an event has timed out, the modules generates
report files. These report files are saved to disk and can also be sent via
email.

It also implements an `ActiveMQ`_ interface which can
send alert messages in real-time. It supports regionalized filter profiles (based on polygons and EQ parameter threshold values).
Currently, messages can be sent in four
different formats (SeisComPML, QuakeML, ShakeAlertML, CAP). The SED-ETHZ team provide a client that can
display these alert messages, the `Earthquake Early Warning Display (EEWD)`_
an OpenSource user interface developed within the European REAKT project and
based on early versions of the ShakeAlert `UserDisplay`_. 

To receive alerts with the EEWD set the format to *qml1.2-rt*. There are
currently no clients which can digest SeisComPML and the UserDisplay format
(*shakealert*) is not supported. Using pipelines alerts can be sent out in
more than one format.

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

To filter alerts to be sent out through ActiveMQ, it is necessary to set profiles on ActiveMQ section.
Since this is using regions which are basically polygons, then the first step is to provide a BNA file that contains the polygons.
If the user does not provide a BNA file, then the other profile parameters will be evaluated globally.

.. code-block:: sh

   activeMQ.bnaFile = /opt/seiscomp3/share/sceewlog/closedpolygons.bna
   
Then profile names have to be set. Two profile examples will be presented below.

.. code-block:: sh

   activeMQ.profiles = global, America
   
For global profile it won't be used a closed polygon since this spans on the entire world. For America profile it will be used 
the "America" closed polygon which has to be in the activeMQ.bnaFile.

.. code-block:: sh

   activeMQ.global.bnaPolygonName = none
   activeMQ.America.bnaPolygonName = America

The magnitude and threshold values will be:

.. code-block:: sh

   activeMQ.global.magThresh = 6.0
   activeMQ.global.likelihoodThresh = 0.5
   activeMQ.America.magThresh = 5.0
   activeMQ.America.likelihoodThresh = 0.3

There are also a depth filter based on min-max range for each profile. Following the example for the global profile, it will be only for shallow EQs, whereas
for America one the min depth will be 0 km and max depth 100 km.

.. code-block:: sh

   activeMQ.global.minDepth = 0
   activeMQ.global.maxDepth = 33
   activeMQ.America.minDepth = 0
   activeMQ.America.maxDepth = 100

Finally, to avoid sending alerts which are far from being considered ontime warnings, then a maxTime value can be set.
This maxTime value is the maximum value in seconds of magnitude creation time minus origin time. For the examples, on the global
profile this parameter will be -1 which means no considering this filter, whereas for America it is set to 60 seconds.

 
.. code-block:: sh

   #No considering this filter
   activeMQ.global.maxTime = -1
   #
   activeMQ.America.maxTime = 60

Headline Change for CAP1.2 XML alerts
=====================================
The converted CAP1.2 xml alert message for every EQ and its updates contains a headline for both English and Spanish languages.
The default message in the headline is: 

@AGENCY@ Magnitude X.X Date and Time (UTC): YYYY-MM-dd HH:mm:s.sssZ.

To change this headline with the format of:

ENGLISH:

@AGENCY@/Earhquake Magnitude X.X, XX km NNW of SOMECITY, SOMECOUNTRY

SPANISH:


@AGENCY@/Sismo Magnitud X.X, XX km al SSO de SOMECITY, SOMECOUNTRY

There is one option on the configuration file that must be enable:

.. code-block:: sh
   
   #enalbe this if you want to change the headline
   ActiveMQ.changeHeadline = true

If this is true then it is mandatory to specify the language and the world cities CSV file of the corresponding selected language. Both the selected language and CSV file must be in the same language.

For languages there are two options that can be selected, Spanish and English:

.. code-block:: sh
  
   #Uncomment the next line to select English
   ActiveMQ.hlLanguage = en-US
   #Uncomment the next line to select Spanish
   #ActiveMQ.hlLanguage = es-US

About the world cities csv file, this must be in the next format:

.. code-block:: sh
  
   city,country,lon,lat
   Tokyo,Japan,139.6922,35.6897
   Jakarta,Indonesia,106.8451,-6.2146
   Delhi,India,77.23,28.66
   Mumbai,India,72.8333,18.9667
   Manila,Philippines,120.9833,14.6
   Shanghai,China,121.4667,31.1667
   Sao Paulo,Brazil,-46.6339,-23.5504


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
