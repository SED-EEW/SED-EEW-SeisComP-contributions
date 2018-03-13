# sceewlog, scvsmag and "decision module" : software specifications and requirements

## Introduction
In the following we outline the technical specifications for the next version of two SeisComP3 modules and the implementation of a third one:
- sceewlog : next version of scvsmaglog compatible with FinDer and any other magnitudes
- scvsmag : next version of scvsmag using eewenv library
- sc? : new decision module to define preferred magnitude and origin in EEW context.

## Generalities
- Integrate in SeisComP3.
- Develop with git using gitlab.seismo.ethz.ch/SED-EEW/sed-addons.
 
## Structure 
Integrate the three softwares in the structure of the SED-EEW package :
- EEW (rst file,  explains libeewenv and list included  modules):
  - sceewenv (separate project)
  - FinDer (separate project)
  - VS (incorportate all existing material, see seiscomp3.org/doc/jakarta/current/apps/vs.htm)
  - sceewlog (incorportate all existing material from scvsmaglog, see seiscomp3.org/doc/jakarta/current/apps/scvsmaglog.html)

### Note
This is relying on the separate development (and documentation) of the SED-EEW package and of sceewenv.


## sceewlog
### Specifications
- Keep all existing capacities of scvsmaglog (see seiscomp3.org/doc/jakarta/current/apps/scvsmaglog.html).
- Rename scvsmaglog as sceewlog and adapt the names of all related files (e.g. VS_reports becomes EEW_reports).
- Add the capacity to log several magnitude types.
- Add a parameter to configure the list of magnitude types to be logged.
- Add magnitude type in reports written to disk.
- Add capacity to log with origins which have no pick, no update comments or likelyhood comments (e.g. scfinder).
- Add --playback and -I options for sequential post-processing of a data file containing events in a real-time manner. 
- Move sceewlog to its own directory inside the SED-EEW package (e.i. gitlab.seismo.ethz.ch/SED-EEW/sed-addons/apps/eew)   

### Tests
- Demonstrate all outputs are the same while MVS only is configure in magnitude types.
- Demonstrate all configured outputs are correct while more magnitude types are configured.

### Documentation
- Update documentation with additions and changes in the last version of the code.

## scvsmag
### Specifications
- Keep all existing capacities of scvsmag (see seiscomp3.org/doc/jakarta/current/apps/scvsmag.html).
- Add the capacity to use libeewenv instead of scenvelop (or sceewenv).
- Add a parameter to switch from using a ''sc*env*'' module to libeewenv.
- Add required default parameters for the use of libeewenv (e.g. vsfndr.enable = true).

### Tests
- Demonstrate that the configuration of envelope processing is correct.
- Demonstrate all outputs are the same while using libeewenv or sceewenv.
- Demonstrate all capacities of scenvelope are available while using libeewenv instead.
- Demonstrate all outputs are highly similar while using libeewenv or scenvelope.

### Documentation
- Update documentation with additions and changes in the last version of the code.
- Delete references to scenvelope inside the VS documentation.
- Add required references to sceewenv and superseeding documentation of the EEW package.

## SeisComP3 EEW decision module
### Naming
- Consider sceeweval.
- All softwares named as ''sc'' are SeisComP3 modules, do not use "module" or "m" in name
- Certain softwares from the SED-EEW can be identify as such using 'sceew'' as a prefix, consider using it.  
- Certain evaluation softwares from GEMPA can be identify as such using ''eval'' as a suffix, consider using it.

### Specifications
- Recycle the code of scvsmag for magnitude reception, ground motion modeling and evaluation of ground motion residual and magnitude-location probability or likelyhood.
- Add the capacity to choose the preferred origin and magnitude based on simple configurable rules without overlapping with scevent capacities. 
- Add capacity to choose the preferred origin and magnitude based on evaluated magnitude-location probability or likelyhood.
- Add capacity to log and to store the evaluated magnitude-location probability or likelyhood inside the SeisComP3 database.
in progress

### Tests
in progress

### Documentation
in progress 
