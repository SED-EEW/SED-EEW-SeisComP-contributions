.. _EEW:

=================
EEW DOCUMENTATION
=================

Since 2021, the SED-ETHZ SeisComP EEW framework includes two EEW algorithms,
the Virtual Seismologist and the Finite-Fault Rupture Detector. They are
supported by generic pre-processing and post-processing modules. Their
architecture is designed to be flexible and extensible to also include other
EEW algorithms in the future.

The architecture of the SED-ETHZ SeisComP EEW package is based on a common,
generic envelope processing library, compiled within the :ref:`sceewenv` module,
that generates real-time envelope values for horizontal and vertical
acceleration, velocity, and displacement from raw acceleration and velocity
waveforms. The envelopes are uses by two EEW algorithms:

- The :ref:`VS` (`Virtual Seismologist`) algorithm, receives envelopes from the
  :ref:`sceewenv` module, and provides near instantaneous estimates of
  earthquake magnitude as soon as SeisComP origins are available.

- The :ref:`FinDer` (`Finite-Fault Rupture Detector`) algorithm, is integrated
  in a single module with the envelope and the *finder* libraries, and matches
  emerging patterns of strong motion from the seismic network with most likely
  seismic sources to predict the location and size of finite faults.

In addition, the :ref:`sceewlog` module creates log output and mails solutions
once a new event is fully processed. It also provides an interface to send
alerts in real-time.

.. _fig-EEW:

.. figure:: base/media/EEW.png
   :width: 16cm
   :align: center

   The SED-ETHZ SeisComP EEW modules

Table of Contents
=================

.. toctree::
   :maxdepth: 3

   /base/FinDer
   /base/VS


EEW License
===========

The SeisComP EEW modules are free and open-source. Currently, they are
distributed under the `GNU Affero General Public License`_ (Free Software
Foundation, version 3 or later).

Timeline
--------

- 2013, SeisComP3 Seattle v2013.200: Initial set of SED EEW modules based only
  on VS, under the `SED Public License for Seiscomp Contributions`_, consisting
  of `scenvelope`, `scvsmag`, and `scvsmaglog`.
- 2020, SeisComP v4.0.0: Licence for the VS modules changed from `SED Public
  License for Seiscomp Contributions`_ to `GNU Affero General Public License`_.
- 2021: FinDer added to the SED-ETHZ SeisComP EEW package, modules repackaged,
  from  SeisComP v4.6.0. Key changes:

1. `scfinder` module added.
2. Generic EEW pre-processing module `sceewenv` replaces VS-specific
   `scenvelope` (deprecated).
3. Generic EEW post-processing module `sceewlog` replaces VS-specific
   `scvsmaglog` (depracated). Licence for these new modules unchanged.

References
==========

Massin, F., J. F. Clinton, M. Boese (2021) Status of Earthquake Early Warning in
     Switzerland, Frontiers in Earth Science,  9:707654. 
     doi:10.3389/feart.2021.707654
     
Behr, Y., J. F. Clinton, C. Cauzzi, E. Hauksson, K. Jónsdóttir, C. G. Marius, A.
     Pinar, J. Salichon, and E. Sokos (2016) The Virtual Seismologist in
     SeisComP: A New Implementation Strategy for Earthquake Early Warning
     Algorithms, Seismological Research Letters, March/March 2016, v. 87, p.
     363-373, doi:10.1785/0220150235


.. target-notes::

.. _`GNU Affero General Public License` : http://www.gnu.org/licenses/agpl-3.0.en.html
.. _`SED Public License for Seiscomp Contributions` : http://www.seismo.ethz.ch/static/seiscomp_contrib/license.txt
