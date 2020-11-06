Part of the :ref:`VS` package.

*sceewenv* is part of the ETH-developed modules that can provide Earthquake Early Warning (EEW) inside SeisComP. All modules in this suite are released
under the GNU Affero General Public License.

*sceewenv* provides extremely low latency estimates of ground motion parameters based on incoming data packets intended to be of use for all downstream EEW modules. Currently, the ETH-developed modules include the
`Virtual Seismologist`_ (VS) Earthquake
Early Warning algorithm (Cua, 2005; Cua and Heaton, 2007) and the `Finite-Fault Rupture Detector`_ 
(FinDer) Earthquake Early Warning algorithm (Böse et al., 2012; Böse et al., 
2015; Böse et al., 2018). 

*sceewenv* superceedes *scvsmag*. *scvsmag*  was developed directly for the VS EEW algorithm suite and provided
real-time envelope values horizontal and vertical acceleration, velocity and
displacement from raw acceleration and velocity waveforms. In particular it 
handled the waveform pre-processing necessary for the :ref:`scvsmag` module.

*sceewenv* extends the output from *scvsmag* and additionally provides in effect continuous real-time streams of PGA, PGV and PGD values which
are used in FinDer. By design, it can be extended to include any other EEW station amplitude parameter for any other EEW algorithm that may be developed.

(please check if all downstream text is still correct, its seems too VS specific. Examples for FinDer sohuld be included.)

The processing procedure is as follows:

#. gain correction
#. baseline correction
#. combination of the two horizontal components to a root-mean-squared horizontal component
#. integration or differentiation to velocity, acceleration and displacement
#. high-pass filter with a corner frequency of 3 s period
#. computation of the absolute value within 1 s intervals

The resulting envelope values are sent as messages to the messaging system via the
"VS" message group. Depending on the number of streams that are processed this can
result in a significant number of messages (#streams/s).

In order to save the messages in the database and to provide them to other modules, 
the messaging system must to be able to handle these messages. Therefore, the plugins 
*dmvs* must be available to :ref:`scmaster` and the "VS" group must be added.

The plugins can be most easily **added** through the configuration parameters
in :file:`global.cfg`:

.. code-block:: sh

   plugins = dmvsi, ...

**Add** the "VS" group the the other message groups defined by :ref:`scmaster` in :file:`scmaster.cfg`:

.. code-block:: sh

   msgGroups = VS, ...

and let *sceewenv* send the messages to the "VS" group instead of "AMPLITUDE".
Adjust :file:`sceewenv.cfg`:

.. code-block:: sh

   connection.primaryGroup = VS

.. note::

   When changing :confval:`connection.primaryGroup`, the "VS" group must also be
   added to the subscriptions in :ref:`scvsmag`.

References
==========

.. target-notes::

.. _`Virtual Seismologist` : http://www.seismo.ethz.ch/en/research-and-teaching/products-software/EEW/Virtual-Seismologist/
.. _`SED Public License for SeisComP Contributions` : http://www.seismo.ethz.ch/static/seiscomp_contrib/license.txt
