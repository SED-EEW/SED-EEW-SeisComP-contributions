# Jakarta

## Release 2021.??? patch?

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