# Description

The *sed-addons* module includes SeisComP3-based product of SED with restricted distribution. It includes:
  - **scfinder** matches emerging patterns of strong motion from the seismic network with most likely seismic sources to predict location and size of finite faults, and from this, it infers event magnitude and resultant expected ground motion patterns.
  - **eewamps** produce ground motion enveloppe values for EEW product as **scfinder** and **scvsmag** (to be moved to main SeisComP3).

# Compile

In order to use this module the sources have to be compiled to an executable. Merge them into the Seiscomp3 sources and compile Seiscomp3 as usual.
  - If you never did, merge sed-addons and Seiscomp3 

```bash
git clone https://github.com/SeisComP3/seiscomp3.git
cd seiscomp3
# Make sure your select the branch of the last release :
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

  - Compilation
     - Configure 
```bash
make -f ../Makefile.cvs
```

         - configure the **main installation path of seiscomp**
         - indicate  **where FinDer was installed** (see https://gitlab.seismo.ethz.ch/SED-EEW/FinDer)
         - configure all other requirement for your seiscomp system.
     - Compile and pre-install (this may be long) 
```bash
cd build
make 
```

     - Compile : if it went alright, your current install (if any) is still untouched but new executables are ready to be installed, to use them do: 
```bash
make install
seiscomp restart [scfinder]
```
