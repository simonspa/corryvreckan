# SpatialClustering
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functioning

### Description
This module clusters the input data of a Timepix1, ATLASpix, or CLICpix device. The clustering method only uses positional information (centre of gravity calculation using touching neighbours method, no timing information). These clusters are stored on the clipboard for each device.

### Parameters
No parameters are used from the configuration file.

### Plots produced
For each detector the following plots are produced:

* Cluster size histogram
* Cluster width (rows, in X) histogram
* Cluster width (columns, in Y) histogram
* Cluster ToT histogram
* 2D cluster positions in global coordinates

### Usage
```toml
[SpatialClustering]

```
