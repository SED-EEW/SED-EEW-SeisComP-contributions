[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.5948948.svg)](https://doi.org/10.5281/zenodo.5948948)
[![Docker](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/actions/workflows/docker-publish.yml/badge.svg)](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/actions/workflows/docker-publish.yml)
[![Documentation Status](https://readthedocs.org/projects/sed-eew-seiscomp-contributions/badge/?version=latest)](https://sed-eew-seiscomp-contributions.readthedocs.io/en/latest/?badge=latest)

# Description

The *SED-EEW-SeisComP-contributions* package contains SeisComP-based software developed by the ETHZ-SED seismic network team that has not been integrated into the main SeisComP distribution. Software includes the modules

  - **scfinder** which matches emerging patterns of strong motion from the seismic network with most likely seismic sources to predict
    location and size of finite faults, and from this, infers event magnitude and expected ground motion patterns.
  - **sceewenv** which produces ground motion envelope values for EEW modules such as **scfinder** and **scvsmag**.

# Requirements

To run the *SED-EEW-SeisComP-contributions* you need

  - [SeisComP](https://www.seiscomp.de/) (version 3 or higher).

Additionally, if you also intend to run the **scfinder** module, you need:

  - The [FinDer](https://github.com/SED-EEW/FinDer) library
  - The [SED SeisComP contributions](https://github.com/swiss-seismological-service/sed-SeisComP-contributions) data model extension

Please see the SeisComP [README](https://github.com/SeisComP/seiscomp/blob/master/README.md) and FinDer
[README](https://github.com/SED-EEW/FinDer/blob/master/README.md) for additional dependencies.

# Installation
For installation, the *SED-EEW-SeisComP-contributions* sources have to be added as a SeisComP submodule and compiled together with SeisComP.

## Using the latest SeisComP version

Following SeisComP's [README](https://github.com/SeisComP/seiscomp/blob/master/README.md) to compile the *SED-EEW-SeisComP-contributions* modules with the latest version of SeisComP, checkout all
required repositories and add *SED-EEW-SeisComP-contributions*:

```bash
# This is experimental
TAG=master
# But that is more safe:
TAG=5.4.0 
git clone --branch $TAG https://github.com/SeisComP/seiscomp seiscomp
git clone --branch $TAG https://github.com/SeisComP/common seiscomp/src/base/common
git clone --branch $TAG https://github.com/SeisComP/main seiscomp/src/base/main
# You might add more repo...
git clone --branch $TAG https://github.com/swiss-seismological-service/sed-SeisComP-contributions seiscomp/src/base/sed-contrib
git clone --branch latest https://github.com/SED-EEW/SED-EEW-SeisComP-contributions seiscomp/src/extras/sed-addons
```

If you have already performed the previous steps and just want to update to latest version, run

```bash
cd seiscomp
git pull
cd src/extras/sed-addons
git pull
```

## Using SeisComP3
If you have not installed SeisComP3 before, checkout the SeisComP3 source code using

```bash
git clone --branch release/jakarta https://github.com/SeisComP3/seiscomp3 seiscomp3
git clone --branch v3 https://github.com/SED-EEW/SED-EEW-SeisComP-contributions seiscomp3/src/sed-addons
```

If you have already performed the previous steps and just want to update to latest version, run

```bash
cd seiscomp3
git pull
cd src/sed-addons
git pull
git checkout jakarta
```

## Adding scfinder

If you intend to use the scfinder SeisComP module you need to indicate **where FinDer was installed** by
setting the `FinDer_INCLUDE_DIR` and `FinDer_LIBRARY` environment variables. [By default](https://github.com/SED-EEW/FinDer)
these should be

      - `FinDer_INCLUDE_DIR           /usr/local/include/finder`
      - `FinDer_LIBRARY               /usr/local/lib/libFinder.a`

If you are using bash you can add these to your `~/.bashrc` using

```bash
echo 'export FinDer_INCLUDE_DIR=/usr/local/include/finder' >> ~/.bashrc
echo 'export FinDer_LIBRARY=/usr/local/lib/libFinder.a' >> ~/.bashrc
echo 'export GMT_INCLUDE_DIR=/usr/local/include/gmt' >> ~/.bashrc
```

## Compilation

SeisComP uses cmake to to manage the build process. In the case of SeisComP (v4+) run

```bash
cd seiscomp
make
```

and in case of SeisComP3, use

```bash
cd seiscomp3
make -f Makefile.cvs
```

Press *c* to start configuring the build and *g* to save the configuration.
If you need to run the configuration in headless mode you can just run cmake
directly

``` bash
cd seiscomp
mkdir build
cd build
cmake .. -DFinDer_INCLUDE_DIR=$FinDer_INCLUDE_DIR -DFinDer_LIBRARY=$FinDer_LIBRARY -DGMT_INCLUDE_DIR=$GMT_INCLUDE_DIR
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

# Usage

To test the functionality of a particular module (e.g. scfinder) use the following command

```bash
seiscomp exec [scfinder] -u testuser --debug
```
and stop it with `CRT+c`

# Cheatsheet
 - For scfinder to include GMT: `LD_LIBRARY_PATH=/usr/<gmt path>:$LD_LIBRARY_PATH` (.profile)
 - For scfinder to include libgmt and libpostscriptlight: `c++ ... -lgmt -lpostscriptlight` (seiscomp' make)
 - For FinDer to include libgmt: `g++  ... -lgmt` (libfinder' make)
