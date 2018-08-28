# scvsmag : software specifications and required developments

## Introduction
In the following we outline the technical specifications for the next version of the `scvsmag` software. This new version will include the eewenv library (`libeewenv`) for builtin envelope computation.  `scvsmag`  will keep or improve all the functionnalities released in 2014, including receiving envelopes computed separately by `scenvelope` or `sceewenv`.

### EEW plans at SED
The  `scvsmag`  program is part of the SED-EEW package. This package first introduced in 2014 a SeisComP3 implementation of the Virtual Seismologist compatible with SED' EEW display software ([EEWD code repository link](https://gitlab.seismo.ethz.ch/SED-EEW/EEWD). The SED-EEW package has been extended in 2016 with the implementation of FinDer in private addons. The development of `sceewlog` is part of a renewed effort since 2018 to use the envelope processing developed for FinDer and to bring both VS and FinDer early warnings into EEWD using SeisComP3.  

The overall scheme of EEW plans at SED is illustrated below.
![eew plans illustrated](../scvsmaglog/specifications/eewplan.png)

### Package structure 
`scvsmag` will be developed inside the SED-EEW package and,  when ready,  pushed to the main SeisComP3 repository. The SED-EEW  package is in `sed-addons/apps/eew` and can be included in `SeisComP3/src/sed/` for seamless compilation within SeisComP3. When `scvsmag` will be be ready for open-source distribution, it will be included inside `SeisComP3/src/sed/eew/`.

`SeisComP3/src/sed-addons/apps/eew` includes a general documentation (an `rst` file)  listing all the modules currently provided by the package:
- `sceewenv` (already existing, see the [sed-addons repository](https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons))
- `scfinder` (already existing, see the [sed-addons repository](https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons))
- scvsmag (to do)


## Specifications
- Keep all existing capacities of scvsmag (see <http://seiscomp3.org/doc/jakarta/current/apps/scvsmag.html>).
- libeewenv:
  - Add the capacity to use libeewenv instead of scenvelop (or `sceewenv`).
  - Add a parameter to switch from using a `sc*env*` module to `libeewenv`.
  - Add required default `*eewenv` parameters for the use of libeewenv (e.i. `vsfndr.enable = true`, `envelopeInterval = 1`).
- write PDF of Mvs in database.  
- logic:
  - Add the capacity to evaluate Mvs on last preferred origin with pick when preferred origin has no pick (e.g. scfinder)
  - Add a parameter to control the use of last preferred origin with picks.

### Tests
- Demonstrate that the configuration of envelope processing is correct.
- Demonstrate all outputs are the same while using `libeewenv` or `sceewenv`.
- Demonstrate all capacities of `scenvelope` are available while using `libeewenv` instead.
- Demonstrate all outputs are highly similar while using `libeewenv` or `scenvelope`.

### Documentation
- Update documentation with additions and changes in the last version of the code.
- Delete references to scenvelope inside the VS documentation.
- Add required references to sceewenv and superseeding documentation of the EEW package.
