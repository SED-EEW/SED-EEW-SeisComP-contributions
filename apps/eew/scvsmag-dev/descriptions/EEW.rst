========================
The SeisComP EEW modules
========================

The architecture of the EEW package is based on a common envelope processing
library that generates real-time envelope values for horizontal and vertical
acceleration, velocity and displacement from raw acceleration and velocity
waveforms. The envelopes are uses by two EEW algorithms:

- The :ref:`VS` (`Virtual Seismologist`) algorithm, receives envelopes from the
the :ref:`sceewenv` module, and provides near instantaneous estimates of
earthquake magnitude as soon as SeisComP origins are available.
- The :ref:`FinDer` (`Finite-Fault Rupture Detector`) algorithm, is integrated
in a single module with the envelope and the *finder* libraries, and matches
emerging patterns of strong motion from the seismic network with most likely
seismic sources to predict the location and size of finite faults.

In addition, the :ref:`sceewlog` module creates log output and mails solutions
once a new event is fully processed. It also provides an interface to send
alerts in real-time.

EEW License
===========

The SeisComP EEW modules are free and open-source, and VS modules are part of
the SeisComP distribution from Seattle v2013.200. They are distributed under the
GNU Affero General Public License (Free Software Foundation, version 3 or
later).
