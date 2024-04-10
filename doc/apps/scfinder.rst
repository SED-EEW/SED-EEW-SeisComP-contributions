Part of the :ref:`EEW<EEW>` package (used by :ref:`FinDer<FINDER>`).

*scfinder* is released under the GNU Affero General Public License (Free
Software Foundation, version 3 or later). It uses the same library as
:ref:`sceewenv` to compute acceleration envelope values provided as input for
the `Finite-Fault Rupture Detector`_ API.

This module requires FinDer to be installed and SeisComP to be compiled from
source. The FinDer software is distributed by the Swiss Seismological Service 
(SED) at ETH Zurich. Access to the source code is limited to users who have an 
established collaboration with the SED. In other cases, compiled binaries (in a 
docker image) may be provided instead. Please contact Dr. Maren Böse (SED) for 
requests and further information.

FinDer provides estimates of the rupture centroid, length and strike. These
values are attached by *scfinder* within a derived object called *strong motion
origin*, which uses the strong motion database extension. In order to save the
strong motion origins to the database and to provide them to other modules, the
messaging system must be able to handle these messages. Therefore, the
plugins *dmsm* must be available to :ref:`scmaster`.

The plugins can be most easily **added** through the configuration parameters
in :file:`global.cfg`:

.. code-block:: sh

   plugins = dmsm

.. note::

   Within :ref:`scfinder`, FinDer uses continuously updated envelope amplitudes 
   from the same library than module :ref:`sceewenv` and the same pre-processing.


scevent configuration
=====================

*scfinder* can produce an origin as soon as seismic waves have reached a minimum
number of stations as configured in finder.config. :ref:`FinDer` is not
pick-based; the phase number attached to its origin relates to the
stations exceeding :ref:`FinDer` ground motion acceleration threshold. For
:ref:`scevent` to create an event from an origin with 4 phases requires the
following setting:

.. code-block:: sh

   # Minimum number of Picks for an Origin that is automatic and cannot be
   # associated with an Event to be allowed to form a new Event.
   eventAssociation.minimumDefiningPhases = 4


Users interested in EEW may decide to run both FinDer and VS together. 
:ref:`scvsmag` uses the preferred origin for VS magnitude computation, and it
should not run on a FinDer origin. In order to run *scfinder* and 
:ref:`scvsmag` on the same system, *scfinder* should be excluded from the 
list of potential preferred origins. This can be achieved by excluding the 
FinDer # *methodID* from preferred origins in the configuration of 
:ref:`scevent`:

.. code-block:: sh

   # The method priority list. When the eventtool comes to the point to select a
   # preferred origin it orders all origins by its methodID priority and selects
   # then the best one among the highest priority method. It also defines the
   # method priority for custom priority checks (eventAssociation.priorities). A
   # defined method string must match exactly the string in Origin.methodID.
   eventAssociation.methods = "NonLinLoc(L2)",\
                              "NonLinLoc(EDT)",\
                              "iLoc",\
                              "Hypo71",\
                              "LOCSAT"

.. note::
   
   Do not include the "MVS" nor "Mfd" magnitude types within :ref:`scevent` list of preferred 
   magnitude types (:confval:`eventAssociation.magTypes`), otherwise, the first origin
   with an "MVS" or "Mfd" will remain preferred for automatic processing despite any newer origins. 

.. target-notes::

.. _`Finite-Fault Rupture Detector` : http://www.seismo.ethz.ch/en/research-and-teaching/products-software/EEW/finite-fault-rupture-detector-finder/
