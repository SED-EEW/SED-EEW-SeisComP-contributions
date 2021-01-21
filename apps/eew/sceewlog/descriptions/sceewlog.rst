Part of the :ref:`EEW` package.

*sceewlog* was originally part of the SeisComP implementation of the
`Virtual Seismologist`_ (VS) Earthquake Early Warning algorithm (Cua, 2005; Cua
and Heaton, 2007) released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It requires the Python package
`dateutil`_ to be installed.

It logs the VS and FinDer magnitude messages received from :ref:`scvsmag` and 
:ref:`scfinder` (configurable), and once an event has timed out, generates
report files. These report files are saved to disk and can also be sent via
email.

It also implements an `ActiveMQ`_ interface which provides the possibility to
send alert messages in real-time. Currently, messages can be sent in three
different formats (SeisComPML, QuakeML, ShakeAlertML). The recommended client to
display these alert messages is the `Earthquake Early Warning Display (EEWD)`_
an OpenSource user interface developed within the European REAKT project and
based on the `UserDisplay`_. The UserDisplay is not openly available, however,
people with permission to run the UserDisplay can use it to receive alert
messages from *sceewlog*.

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
