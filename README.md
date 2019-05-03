# Description

The *sed-addons* module includes SeisComP3-based sofwares for SED with restricted distribution. It includes:
  - **scfinder** matches emerging patterns of strong motion from the seismic network with most likely seismic sources to predict location and size of finite faults, and from this, it infers event magnitude and resultant expected ground motion patterns.
  - **sceewenv** produces ground motion enveloppe values for EEW modules as **scfinder** and **scvsmag**.


# Requirements
  - [SeisComP3](http://www.seiscomp3.org/doc/jakarta/current/base/installation.html#requirements) is required for compilation.
  - The [FinDer](https://gitlab.seismo.ethz.ch/SED-EEW/FinDer)'s library is required for scfinder.

# Compilation
In order to use this module the sources have to be compiled to an executable. Merge them into the Seiscomp3 sources and compile Seiscomp3 as usual.
  - If you never did, merge sed-addons and Seiscomp3 
```bash
git clone https://github.com/SeisComP3/seiscomp3.git
cd seiscomp3
```

  - Make sure your select the branch of the last release :
```bash
git checkout release/jakarta
git submodule add -f https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons.git src/sed-addons
```

  - If you have already cloned, and just want to update to last version: 
```bash
cd <Path to your clone of>/seiscomp3
git pull
cd src/sed-addons
git pull
```

  - Start configuration tool  
```bash
cd <Path to your clone of>/seiscomp3
make -f Makefile.cvs
```

  - Configure the **main installation path of seiscomp**
    - indicate  **where FinDer was installed**, [by default](https://gitlab.seismo.ethz.ch/SED-EEW/FinDer):
      - `FinDer_INCLUDE_DIR           /usr/local/include/finder`
      - `FinDer_LIBRARY               /usr/local/lib/libFinder.a`
    - configure all other requirement for your seiscomp system.
  - Compile and pre-install (this may be long) 
```bash
cd build
make 
```

  - If fails  : use `make -j 4`  for debug.
  - If passes : install, your previous install (if any) is still untouched but new executables are ready to be installed, to install them do: 
```bash
make install
seiscomp restart [scfinder]
```
