Part of the :ref:`EEW<EEW>` package (in :ref:`VS<VS>`).

*sceewenv* is released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It provides in effect continuous
real-time streams of envelope values for horizontal and vertical acceleration,
velocity and displacement from raw acceleration and velocity waveforms.
*sceewenv* was extended from the original SED-ETHZ VS-specific pre-processing
module `scenvelope`, implemented to handle the waveform pre-processing necessary
for the :ref:`scvsmag` module (`Virtual Seismologist`_). Further parameters can also being calculated
using the core library included in *sceewenv*, such as spectral amplitudes and
tauP, though these are not accessible via the current *sceewenv* implementation.

The processing procedure for envelope computation is as follows:

#. gain correction
#. baseline correction
#. combination of the two horizontal components to a root-mean-squared horizontal component
#. integration or differentiation to velocity, acceleration and displacement (both velocity and displacement are high pass pre-filtered over 0.075 Hz)
#. high-pass filter with a corner frequency of 3 s period
#. computation of the absolute value within 1 s intervals

In the example below, the resulting envelope values are sent as messages to the 
messaging system via the "EEW" message group. Depending on the number of streams 
that are processed this can result in a significant number of messages 
(#streams/s).

In order to save the messages in the database and to provide them to other
modules, the messaging system must to be able to handle these messages.
Therefore, the plugins *dmvs* must be available to :ref:`seiscomp:scmaster`  and the "EEW"
group must be added.

The plugins can be most easily **added** through the configuration parameters
in :file:`global.cfg`:

.. code-block:: sh

   plugins = ${plugins}, dmvs

**Add** the "ENVELOPE" group the the other message groups defined by
:ref:`scmaster` in :file:`scmaster.cfg`:

.. code-block:: sh

   msgGroups = ENVELOPE, ...

and let *sceewenv* send the messages to the "ENVELOPE" group instead of
"AMPLITUDE". Adjust :file:`sceewenv.cfg`:

.. code-block:: sh

   connection.primaryGroup = ENVELOPE

.. note::

   When changing :confval:`connection.primaryGroup`, the "ENVELOPE" group must
   also be added to the subscriptions in :ref:`scvsmag`.

References
==========

.. target-notes::

.. _`Virtual Seismologist` : http://www.seismo.ethz.ch/en/knowledge/earthquake-data-and-analysis-tools/EEW/Virtual-Seismologist/index.html
