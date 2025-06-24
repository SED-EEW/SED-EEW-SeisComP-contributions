# Change Log

## Master

## tag 5.1.1.2025

* Fix event data in unit test.  

* [#71](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/pull/71): Support API 17.  

* Documentation

  * [#72](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/pull/72): Fixes FinDer setup.

* sceewlog

  * [#74](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/pull/74): Better support any magnitude.

  * [#69](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/pull/69): New script `eew2shakemap` to prepare and run shakemap based on EEW solition via option `EEW.script`.
  
  * [#61](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/pull/61): New options `FCM.topicnotification` and `FCM.eewmessagecomment` to send notification by topic and/or create an EEW message as a comment of the relevant magnitude.

* scfinder:
   
  * [#59](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/pull/59): Adding the fault rupture parameters into the strong motion database extension, both width and vertices.

  * [#60](https://github.com/SED-EEW/SED-EEW-SeisComP-contributions/pull/60): Add the missing strong motion event description between strong motion record and strong motion parameter

## tag 5.1.1.ssa2024

* scvsmag
  
  * Fix reading for VS30 grid

* scfinder:

  * New parameter `maxEnvelopeBufferDelay`, to skip any channel not updated recently enough for input to FinDer.
  
  * Adds configuration for env. filter corner freq.: debug.filterCornerFreq

* sceewenv:

  * Removing envelope delay filter as proven to be problematic in  scfinder.
  
  * Adds configuration for env. filter corner freq.: eewenv.vsfndr.filterCornerFreq
  
* sceewlog

  * New parameter `EEW.script`, path to script to execute after sending alert

  * New external script `sceewlog2file`, example that can be used with `EEW.script`


## Release 2023 June 19 tag 5.1.2

* sceewlog:
  
  * Configuration of profile and EEW comment made cleaner
  
* scgof
  
  * New module: Geographic origin filter (#12)
   
* scfinder:

  * Scan and process intervals setup to 1s by default (avoiding multithreading issues).

  * Default envelope delay filter threshold to 30s by default (similar to shakealert) 
  
  * compilation: The path to dmsm in scfinder is fixed to `./../../../../../base/sed-contrib/libs`

* sceewenv:
  
  * Default envelope delay filter threshold to 30s by default (similar to shakealert)  

## Release 2022 November 15 tag 5.1.1

* scfinder:

  * Dynamic window length for envelope maxima.

  * Minor changes in compilation configuration for latest FinDer version. 

  * New plotting utility script 

## Release 2022 July 26 tag 5.0.0

* libs/eewamps (included in sceewenv, scfinder): 

  * Configurable envelopes delay threshold to ignore late envelope values.

  * Average delay logged every minutes (information channel).

* scvsmag:

  * Fix appname in log files.

* sceewlog:

  * Configurable selection of the magnitude preferred for EEW based on magnitude 
    attributes (available options are magnitude value, likelihood, authors, and number 
    of stations).

  * Configurable alerting profiles with geographical, magnitude, likelihood, delay and 
    depth filters. 

  * New optional magnitude comment with EEW usage status (1: used for initial EEW, 2+: 
    used for updating EEW).
  
  * New optional output format in CAP1.2.
  
  * New optional interface for Firebase Cloud Messaging.
  
  * New utility sceewdump to extract EEW results.

* Doc:

  * Corrections and clarification.

  * Automatic build in seiscomp.de.

## Release 2022 Fev 2 tag 4.8.3

* Dockerfile: 

  * Disable compilation tests

  * Fix inventory xml version issue in built test

* Documentation: Update bibliography

* License: Create LICENSE

## Release 2022 Jan 15 tag v4

* sceewenv

  * New module: sceewenv replaces scenvelope, sceewenv is based on a new EEW 
    envelope processing library, fulfiling the same functionalities

  * Horizontal components are combined before computing envelope value, instead of
    combining horizontal envelope values as in scenvelope

* scfinder

  * New module: combines the EEW envelope and if available, the FinDer libraries for 
    finite fault-line rupture detection and tracking in real-time

* sceewlog

  * Upgraded version of scvsmaglog fulfiling the same functionalities with both the new 
    FinDer, and pre-existing VS magnitudes

* scvsmag

  * Documentation refers to replacement module sceewenv (previously scenvelope), and 
    renamed module sceewlog  (previously scvsmaglog)
