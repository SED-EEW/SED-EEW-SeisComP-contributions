# Change Log

## Release 2022 ?

* libs/eewamps, sceewenv, scfinder: 

  * Ignore envelopes with delays higher than 30s (configurable)

  * Average delay logged (info channel) every minutes

* scvsmag

  * Fix appname in log files

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
