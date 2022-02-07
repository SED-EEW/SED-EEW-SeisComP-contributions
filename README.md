# Description

The *sed-addons* package contains SeisComP-based software developed by the ETHZ-SED seismic network team that has not been integrated into the main SeisComP distribution. Software includes the modules

  - **scfinder** which matches emerging patterns of strong motion from the seismic network with most likely seismic sources to predict
    location and size of finite faults, and from this, infers event magnitude and expected ground motion patterns.
  - **sceewenv** which produces ground motion envelope values for EEW modules such as **scfinder** and **scvsmag**.

# Requirements

To run the *sed-addons* you need

  - [SeisComP](https://www.seiscomp.de/) (version 3 or higher).
  - The [FinDer](https://gitlab.seismo.ethz.ch/SED-EEW/FinDer) library, if you also intend to run the **scfinder** module

Please see the SeisComP [README](https://github.com/SeisComP/seiscomp/blob/master/README.md) and FinDer
[README](https://gitlab.seismo.ethz.ch/SED-EEW/FinDer/-/blob/master/README.md) for additional dependencies.

# Installing *sed-addons*
For installation, the *sed-addons* sources have to be added as a SeisComP submodule and compiled together with SeisComP.

## Using SeisComP3
If you have not installed SeisComP3 before, checkout the SeisComP3 source code using

```bash
git clone https://github.com/SeisComP3/seiscomp3.git .
```

Make sure your select the branch of the last release and add the *sed-addons* as a submodule

```bash
cd seiscomp3
git checkout release/jakarta
git submodule add -f https://github.com/SED-EEW/SED-EEW-SeisComP-contributions src/sed-addons
```

If you have already performed the previous steps and just want to update to latest version, run

```bash
cd <Path to your copy of>/seiscomp3
git pull
cd src/sed-addons
git pull
```

## Using the latest SeisComP version

To compile the *sed-addons* modules with the latest version of SeisComP (currently 4.2.0), run

```bash
git clone https://github.com/SeisComP/seiscomp.git .
```

Following SeisComP's [README](https://github.com/SeisComP/seiscomp/blob/master/README.md) checkout all
default submodules (see the step "Checkout the repositories"). Then, add *sed-addons* as a submodule

```bash
cd seiscomp
git submodule add -f https://github.com/SED-EEW/SED-EEW-SeisComP-contributions src/extras/sed-addons
```

If you have already performed the previous steps and just want to update to latest version, run

```bash
cd <Path to your copy of>/seiscomp
git pull
cd src/extras/sed-addons
git pull
```

## Adding scfinder

If you intend to use the scfinder SeisComP module you need to indicate **where FinDer was installed** by
setting the `FinDer_INCLUDE_DIR` and `FinDer_LIBRARY` environment variables. [By default](https://gitlab.seismo.ethz.ch/SED-EEW/FinDer)
these should be

      - `FinDer_INCLUDE_DIR           /usr/local/include/finder`
      - `FinDer_LIBRARY               /usr/local/lib/libFinder.a`

If you are using bash you can add these to your `~/.bashrc` using

```bash
echo 'export FinDer_INCLUDE_DIR=/usr/local/include/finder' >> ~/.bashrc
echo 'export FinDer_LIBRARY=/usr/local/lib/libFinder.a' >> ~/.bashrc
```

## Compilation

SeisComP uses cmake to to manage the build process. In the case of SeisComP3 run

```bash
cd <Path to your clone of>/seiscomp3
make -f Makefile.cvs
```

and in case of SeisComP 4, use

```bash
cd <Path to your clone of>/seiscomp
make
```
Press *c* to start configuring the build and *g* to save the configuration.
If you need to run the configuration in headless mode you can just run cmake
directly


``` bash
cd <Path to your clone of>/seiscomp
mkdir build
cd build
cmake .. -DFinDer_INCLUDE_DIR=$FinDer_INCLUDE_DIR -DFinDer_LIBRARY=$FinDer_LIBRARY
```
For other cmake option see the SeisComP [README] (https://github.com/SeisComP/seiscomp/blob/master/README.md).
To start the compilation run

```bash
cd build
make
```

If your machine has multiple CPU cores available, you can speed up the compilation with `make -j 4`.
If the build succeeds you can install the new executables with

```bash
sudo make install
```
Note that this will overwrite your previous installation of SeisComP

# Running *sed-addons*

To test the functionality of a particular module (e.g. scfinder) use the following command

```bash
seiscomp exec [scfinder] -u testuser --debug
```
and stop it with `CRT+c`

# Cheatsheet
 - For scfinder to include GMT: `LD_LIBRARY_PATH=/usr/<gmt path>:$LD_LIBRARY_PATH` (.profile)
 - For scfinder to include libgmt and libpostscriptlight: `c++ ... -lgmt -lpostscriptlight` (seiscomp' make)
 - For FinDer to include libgmt: `g++  ... -lgmt` (libfinder' make)
