Part of the :ref:`EEW` package.

*sceewenv* was originally part of the SeisComP implementation of the `Virtual
Seismologist`_(VS) Earthquake Early Warning algorithm (Cua, 2005; Cua and
Heaton, 2007) and its core library is now integrated into :ref:`scfinder`, the
wrapper for the `Finite-Fault Rupture Detector`_ (FinDer) Earthquake Early
Warning algorithm (Böse et al., 2012; Böse et al., 2015; Böse et al., 2018).
*sceewenv* is released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It generates real-time envelope values
for horizontal and vertical acceleration, velocity and displacement from raw
acceleration and velocity waveforms. It was implemented to handle the waveform
pre-processing necessary for the :ref:`scvsmag` module. It provides in effect
continuous real-time streams of PGA, PGV and PGD values which could also be used
independently of :ref:`scvsmag`.

The processing procedure is as follows:

#. gain correction
#. baseline correction
#. combination of the two horizontal components to a root-mean-squared horizontal component
#. integration or differentiation to velocity, acceleration and displacement
#. high-pass filter with a corner frequency of 3 s period
#. computation of the absolute value within 1 s intervals

The resulting envelope values are sent as messages to the messaging system via
the "VS" message group. Depending on the number of streams that are processed
this can result in a significant number of messages (#streams/s).

In order to save the messages in the database and to provide them to other
modules, the messaging system must to be able to handle these messages.
Therefore, the plugins *dmvs* must be available to :ref:`scmaster` and the "VS"
group must be added.

The plugins can be most easily **added** through the configuration parameters
in :file:`global.cfg`:

.. code-block:: sh

   plugins = dmvs, ...

**Add** the "VS" group the the other message groups defined by :ref:`scmaster`
in :file:`scmaster.cfg`:

.. code-block:: sh

   msgGroups = VS, ...

and let *sceewenv* send the messages to the "VS" group instead of "AMPLITUDE".
Adjust :file:`sceewenv.cfg`:

.. code-block:: sh

   connection.primaryGroup = VS

.. note::

   When changing :confval:`connection.primaryGroup`, the "VS" group must also
   be added to the subscriptions in :ref:`scvsmag`.

References
==========

.. target-notes::

.. _`Virtual Seismologist` : http://www.seismo.ethz.ch/en/research-and-teaching/products-software/EEW/Virtual-Seismologist/
