# Change Log

## Release 2022 ?

## Release 2022 July 26 tag 4.9.2

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

  * New optional magnitude comment with EEW usage status (0: not used for EEW, 1: used for 
    initial EEW, 2+: used for updating EEW).
  
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
