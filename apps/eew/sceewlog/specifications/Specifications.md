# sceewlog : software specifications and required developments

## Introduction
In the following we outline the technical specifications for the  `sceewlog` software, the next version of `scvsmaglog` which will be compatible with FinDer and any other magnitudes.

### EEW plans at SED
The  `scvsmaglog`  and `sceewlog` programs are part of the SED-EEW package. This package first introduced in 2014 a SeisComP3 implementation of the Virtual Seismologist compatible with SED' EEW display software (EEWD <https://gitlab.seismo.ethz.ch/SED-EEW/EEWD>). The SED-EEW package has been extended in 2016 with the implementation of FinDer in private addons. The development of `sceewlog` is part of a renewed effort since 2018 to use the envelope processing developed for FinDer and to bring both VS and FinDer early warnings into EEWD using SeisComP3.  

The overall scheme of EEW plans at SED is illustrated below.
![eew plans illustrated](eewplan.png)


### Design of the `sceewlog` software
The  `scvsmaglog`  and `sceewlog` programs are SeisComP3 modules developed by SED-ETHZ to:
- receive magnitudes-related metadata,
- transmit magnitudes-related metadata thought an activeMQ interface if configured,
- convert magnitudes-related metadata into quakeml-rt or shakealert  if requested, 
- save a summary report of the magnitude(s) evolution report to disk if requested,
- send the report though email if requested.
`sceewlog`  will incorportate all existing material from  `scvsmaglog`, see <http://seiscomp3.org/doc/jakarta/current/apps/scvsmaglog.html> for a full description.

### Implementation note
After filtering only MVS magnitudes, the last version of the `scvsmaglog`  program requires three things for a magnitude to be logged:
- the magnitude related origin needs to have arrivals and picks,
- the magnitude needs to have a comment of type "update" that provides an update number,
- the magnitude needs to have a comment of type "likelihood"  that provides a likelihood value.
The implementation of  `sceewlog` will require to get rid of, at least, the first two limitations.  

## Package structure 
`sceewlog` is integrated inside the structure of the SED-EEW package. This package is in `sed-addons/apps/eew` and can be included in `SeisComP3/src/` for seamless compilation within SeisComP3. `SeisComP3/src/sed-addons/apps/eew` includes a general documentation (an `rst` file)  explaining `libeewenv` and listing all the modules currently provided by the package:
- sceewenv (already existing, see <https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons>)
- scfinder (already existing, see <https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons>)
- sceewlog (to do)

### Note
This is relying on the separate development (and documentation) of the SED-EEW package and of sceewenv.

## Specifications
### General specifications
These specifications are required for all SED' EEW modules:
- Integrate in SeisComP3.
- Develop with git using <http://gitlab.seismo.ethz.ch/SED-EEW/sed-addons>.
- Include the full descriptions (name, type, default value and usage) of all the parameters in the SeisComP3-compatible description file i.e. <http://gitlab.seismo.ethz.ch/SED-EEW/sed-addons/eew/sceewlog/sceewlog.xml>.
 
 ### Reading notes
- Some of the required tasks below are actually already fullfiled. These are indicated with the prefix: `|Done|`. 
- The new parameters suggested in the next section are already listed in <http://gitlab.seismo.ethz.ch/SED-EEW/sed-addons/eew/sceewlog/sceewlog.xml>, direct links to suggested parameters are provided inline.
- Some of the new codes suggested in the next section are already drafted in <http://gitlab.seismo.ethz.ch/SED-EEW/sed-addons/eew/sceewlog.py>, direct links to suggested codes are provided inline.
 
### Specifications for  `sceewlog`
- Keep all existing capacities of scvsmaglog (see <http://seiscomp3.org/doc/jakarta/current/apps/scvsmaglog.html>):
  - `|Done|` Move `scvsmaglog` to its own directory inside the SED-EEW package and develop in <http://gitlab.seismo.ethz.ch/SED-EEW/sed-addons/apps/eew/sceewlog/>
  - `|Done|` Reuse the code of `scvsmaglog` and rename as sceewlog and adapt the names of all related files (e.g. `VS_reports` becomes `EEW_reports`). This is given as a parameter (`name="directory" type="dir" default="~/.seiscomp3/log/EEW_reports"`, see <https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons/blob/master/apps/eew/sceewlog/descriptions/sceewlog.xml#L95>)
  - `|Done|` Edit <CMakeLists.txt> as required so `sceewlog` is compiled within SeisComP3.
- Add the capacity to log specific magnitude type or types and improve compatibility with other magnitude types than MVS:
  - Add a parameter to configure the list of magnitude types to be logged (e.g. `name="magTypes" type="list:string" default="MVS, Mfd"`, see <https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons/blob/master/apps/eew/sceewlog/descriptions/sceewlog.xml#L78>). 
  - Add codes to pass only the configured magnitude types (see <https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons/blob/master/apps/eew/sceewlog/sceewlog.py#L247>)
  - Change the logic of magnitude ordering in the `EEW_report/` files for ordering by creation time instead of update number and move processing triggered by the reception of update number to the reception of likelyhood (see <https://gitlab.seismo.ethz.ch/SED-EEW/sed-addons/blob/master/apps/eew/sceewlog/sceewlog.py#L362>). Use a separate method to add the magnitude update to the `sceewlog`' listener cache 
  - Insure capacity to log with origins without pick or magnitude without update comments (e.g. scfinder). 
  - Add magnitude type, rupture strike and length in reports (written to disk and sent by email, see next section).
  - Change the xml conversion stylesheets to add rupture parameters in XML alert messages (sent to the ActiveMQ interface).

### Specifications of the reports
As `scvsmaglog`, `sceewlog` generates a report (written to disk and sent by email) once an event has timed out. An example is given below:

```
Tdiff   |Type|Mag.|Lat.  |Lon.   |Depth |origin time (UTC)        |likeh.|#st.(org.) |#st.(mag.) |Strike |Length |author    |      creation time (UTC) 
------------------------------------------------------------------------------------------------------------------------------------------------------
 23.6324| MVS|2.01| 46.29|   7.62|  3.65|2018-03-27T10:42:31.7209Z|  0.12|          6|      5    |       |       |   scvsmag|2018-03-27T10:42:52.7209Z
 24.6315| Mfd|2.01| 46.29|   7.62|  3.65|2018-03-27T10:42:31.7209Z|  0.12|           |     15    |    345|   0.05|  scfinder|2018-03-27T10:42:53.7209Z
```
### Further functionalities
Further functionalities might also be developed in time allows. These extra functionalities might actually end up to be required while fullfuling the inital specifications.  
- Insure capacity to log with any magnitudes without likelihood comments. 
- Add `--playback` and `-I` options for sequential post-processing of a data file containing events in a real-time manner (respecting the creation times of input elements). 
- Pass envelope and or amplitude through the  ActiveMQ interface. This will require either to rung a separe `sceewenv` instance or including the  `libeewenv`  library in `sceewlog`.

## Tests
- Demonstrate all outputs are the same while only MVS is configured in the magnitude type list.
- Demonstrate all configured outputs are correct while more magnitude types are configured.
- Demonstrate  that `sceewlog`  is at least as fast as  `scvsmaglog` to produce the same outputs.

## Documentation
- Update documentation with additions and changes in the last version of the code.


