.. _FINDER:

=============================
Finite-Fault Rupture Detector
=============================

Part of the :ref:`EEW<EEW>` package. 

The FinDer software is distributed by the Swiss Seismological Service (SED) at 
ETH Zurich. Access to the source code is limited to users who have an established 
collaboration with the SED. In other cases, compiled binaries (in a docker image) 
may be provided instead. Please contact Dr. Maren Böse (SED) for requests and 
further information.

The ground motion observed during large earthquakes is controlled by the
distance to the rupturing fault, not by the hypocentral distance. Traditional
point-source algorithms can only provide the hypocentral distance. The
`Finite-Fault Rupture Detector`_ (FinDer) Earthquake Early Warning algorithm
(Böse et al., 2012) can determine fast and robust line-source models of
large earthquakes in order to enhance ground-motion predictions for earthquake
early warning (EEW) and rapid response. The algorithm quantifies model
uncertainties in terms of likelihood functions (Böse et al., 2015), and can be
applied across the entire magnitude range from M2 to M9 (Böse et al., 2018).

The FinDer algorithm is based on template matching, in which the evolving spatial
distribution of high-frequency ground-motion amplitudes (usually peak ground
acceleration, PGA) observed on a seismic monitoring network is continuously compared
with theoretical template maps. These templates are pre-computed from empirical
ground-motion prediction equations (GMPEs) for line-sources of different
lengths and magnitudes, and can be rotated on-the-fly to constrain the strike of
the earthquake fault rupture.

The template that shows the highest correlation with the observed ground-motion
pattern is determined from a combined grid-search and divide-and-conquer
approach (Böse et al., 2018). The resulting finite-source model is characterized
by the line-source centroid, length, strike and corresponding likelihood
functions. The model is updated every second until peak shaking is reached, thus
allowing to keep track of fault ruptures while they are still evolving.

Compared to traditional point-source EEW algorithms, FinDer has a number of
interesting features (see Böse et al., 2018 for details):

- Characterization of seismic ground-motions rather than earthquake sources;
- Consistent models and uncertainties for both small and large earthquakes;
- No magnitude saturation in large earthquakes;
- Applicable to complex earthquake sequences;
- No station averages, but true network solutions;
- Independent from traditional phase picker and pick associators;
- Unlikely to trigger during teleseisms;
- Enables joint seismic-geodetic real-time finite-fault models (e.g. for tsunami warning);
- Can resolve fault-plane ambiguities, including those of small earthquakes.


Development
-----------

The implementation of FinDer proceeds in close collaboration of the Seismic
Network group at the SED in ETH Zurich with the US Geological Survey (USGS) and
the California Institute of Technology (Caltech).


FinDer and SeisComP
-------------------

FinDer has been implemented with an API. To integrate FinDer within SeisComP, a
wrapper module :ref:`scfinder` uses this API. The :ref:`scfinder` module
requires FinDer to be installed and SeisComP to be compiled from source.

The library version of the generic EEW pre-processing module :ref:`sceewenv` is used
within :ref:`scfinder` to provide continuously updated envelope amplitudes to FinDer.
FinDer outputs magnitudes (Mfd). Mfd is updated when significant changes are
observed with updates continuing until a stable solution is reached.

An additional generic EEW module, :ref:`sceewlog`, creates log output and mails
solutions once a new event is fully processed. It also provides an interface to
send alerts in real-time using ActiveMQ. The `Earthquake Early Warning Display`_
(EEWD, Cauzzi et al., 2016), an open-source java application, can receive and
display EEW messages broadcast via ActiveMQ.


FinDer in a Docker
------------------

See :ref:`DOCKERFINDER`.
    

EEW License
-----------

The SeisComP EEW modules are free and open source. They are distributed
under the GNU Affero General Public License (Free Software Foundation, version 3
or later). For licence information on SED-ETHZ SeisComP EEW modules released
before SeisComP v4.0.0 see the Timeline in :ref:`EEW<EEW>`.


References
----------

Böse, M., Heaton, T. H., & Hauksson, E., 2012: 
    Real‐time Finite Fault Rupture Detector (FinDer) for large earthquakes. 
    Geophysical Journal International, 191(2), 803–812, doi:10.1111/j.1365-246X.2012.05657.x

Böse, M., Felizardo, C., & Heaton, T. H., 2015: 
    Finite-Fault Rupture Detector (FinDer): Going Real-Time in Californian 
    ShakeAlertWarning System. Seismological Research Letters, 86(6), 1692–1704, 
    doi:10.1785/0220150154

Böse, M., Smith, D., Felizardo, C., Meier, M.-A., Heaton, T. H., & Clinton, J. F., 2018: 
    FinDer v.2: Improved Real-time Ground-Motion Predictions for M2-M9
    with Seismic Finite-Source Characterization. Geophysical Journal
    International, 212(1), 725-742, doi:10.1093/gji/ggx430
    
Cauzzi, C., Behr, Y. D., Clinton, J., Kastli, P., Elia, L., & Zollo, A., 2016:
     An Open-Source Earthquake Early Warning Display. Seismological Research
     Letters, 87(3), 737–742, doi:10.1785/0220150284

.. target-notes::

.. _`Finite-Fault Rupture Detector` : http://www.seismo.ethz.ch/en/knowledge/earthquake-data-and-analysis-tools/EEW/finite-fault-rupture-detector-finder/
.. _`Earthquake Early Warning Display` : https://github.com/SED-EEW/EEWD
