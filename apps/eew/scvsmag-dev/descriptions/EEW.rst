========================
The SED-ETHZ SeisComP EEW modules
========================

In 2021, the SED-ETHZ SeisComP EEW framework has been extended to include a second EEW algotihm, FinDer, that can also be used with the Virtual Seismologist. To accomodate this change, generic pre-processing and prost-processing modules have been added. This architecture is designed to be flexible and extensible to also include other EEW algorithms in future.

The architecture of the EEW package is based on a common, generic envelope processing
library, compiled within the :ref:`sceewenv` module, that generates real-time
envelope values for horizontal and vertical acceleration, velocity and
displacement from raw acceleration and velocity waveforms. The envelopes are
uses by two EEW algorithms:

- The :ref:`VS` (`Virtual Seismologist`) algorithm, receives envelopes from the :ref:`sceewenv` module, and provides near instantaneous estimates of earthquake magnitude as soon as SeisComP origins are available.

- The :ref:`FinDer` (`Finite-Fault Rupture Detector`) algorithm, is integrated in a single module with the envelope and the *finder* libraries, and matches emerging patterns of strong motion from the seismic network with most likely seismic sources to predict the location and size of finite faults.

In addition, the :ref:`sceewlog` module creates log output and mails solutions
once a new event is fully processed. It also provides an interface to send
alerts in real-time.

**add summary / overview figure showing general architecture**

EEW License
===========

The SeisComP EEW modules are free and open-source. Currently, they are distributed under the
GNU Affero General Public License (Free Software Foundation, version 3 or
later).

Timeline
========

- 2013: Iniital set of SED EEW modules based only on VS provided from Seattle v2013.200, under SED SeisComP3 licence. Consists of scenvelope, scvsmag, scvsmaglog
- ???2019: Licence for the VS modules changed from ?? to ??
- 2021: Finder added to SED EEW suite, modules repakaged, from  SeisComP v4.???. Key changes: 1. scfinder module added. 2. Generic EEW pre-processing module sceewenv replaces VS-specific scenvelope (deprecated). 3. Generic EEW post-processing module sceewlog replaces VS-specific scvsmaglog (depracated). Licence for these new modules unchanged.

General References
==================

Description of current version: Massin et al, Frontiers
Description of VS-specific version: Behr et al, SRL
