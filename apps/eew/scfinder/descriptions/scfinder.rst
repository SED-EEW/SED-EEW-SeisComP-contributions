*scfinder* is a wrapper for the `Finite fault rupture Detector`_ 
(FinDer) Earthquake Early Warning algorithm (Böse et al., 2012; Böse et al., 
2015; Böse et al., 2018). *scfinder* is released under the 
`SED Public License for SeisComP Contributions`_. It uses the same library 
than :ref:`sceewenv` to compute acceleration envelope values provided as 
input for the FinDer API.

This module requires FinDer to be installed and SeisComP to be compiled from
source. The source code for FinDer is distributed separatly by SED.

scevent configuration
=====================

*scfinder* can produce an origin as soons as the seismic waves reach 4 stations. 
For :ref:`scevent` to create an event from an origin with 4 phases requires the
following setting:

.. code-block:: sh

   # Minimum number of Picks for an Origin that is automatic and cannot be
   # associated with an Event to be allowed to form an new Event.
   eventAssociation.minimumDefiningPhases = 4

.. note::

   :ref:`scvsmag` uses the preferred origin for VS magnitude computation, and it
   should not run on a FinDer origin. In order to run *scfinder* and :ref:`scvsmag`
   on the same system, *scfinder* should be excluded from the list of potential
   preferred origins. One way to do this is to exclude the FinDer # *methodID* 
   from preferred origins in the configuration of :ref:`scevent`:

.. code-block:: sh

   # The method priority list. When the eventtool comes to the point to select a
   # preferred origin it orders all origins by its methodID priority and selects
   # then the best one among the highest priority method. It also defines the
   # method priority for custom priority checks (eventAssociation.priorities). A
   # defined method string must match exactly the string in Origin.methodID.
   eventAssociation.methods = "NonLinLoc(L2)","NonLinLoc(EDT)","Hypo71","iLoc","LOCSAT"

References
==========

Böse, M., Heaton, T. H., & Hauksson, E., 2012: Real‐time Finite Fault Rupture Detector (FinDer) for large earthquakes. Geophysical Journal International, 191(2), 803–812. doi:10.1111/j.1365-246X.2012.05657.x

Böse, M., Felizardo, C., & Heaton, T. H., 2015: Finite-Fault Rupture Detector ( FinDer): Going Real-Time in Californian ShakeAlertWarning System. Seismological Research Letters, 86(6), 1692–1704. doi:10.1785/0220150154

Böse, M., Smith, D., Felizardo, C., Meier, M.-A., Heaton, T. H., & Clinton, J. F., 2017: FinDer v.2: Improved Real-time Ground-Motion Predictions for M2-M9 with Seismic Finite-Source Characterization. Geophysical Journal International.

.. target-notes::

.. _`Finite fault rupture Detector` : http://www.seismo.ethz.ch/en/research-and-teaching/products-software/EEW/finite-fault-rupture-detector-finder/
.. _`SED Public License for SeisComP Contributions` : http://www.seismo.ethz.ch/static/seiscomp_contrib/license.txt
