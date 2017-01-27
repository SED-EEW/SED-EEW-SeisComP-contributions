This branch is now retired. It comes from the SED-EEW fork of SeisComP3 where scfinder and eewamps were first developped.

# Description

The *sed-addons* module includes SeisComP3-based product of SED with restricted distribution. It includes:
  - **scfinder** matches emerging patterns of strong motion from the seismic network with most likely seismic sources to predict location and size of finite faults, and from this, it infers event magnitude and resultant expected ground motion patterns.
  - **eewamps** produce ground motion enveloppe values for EEW product as **scfinder** and **scvsmag** (to be moved to main SeisComP3).

# Compile

In order to use this module the sources have to be compiled to an executable. Merge them into the Seiscomp3 sources and compile Seiscomp3 as usual.
```bash
# merge bgr-mags-addons and Seiscomp3
git clone https://github.com/SeisComP3/seiscomp3.git
cd seiscomp3
git submodule add -f https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons.git src/sed-addons
```
