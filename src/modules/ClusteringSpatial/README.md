# ClusteringSpatial
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functioning

### Description
This module clusters the input data of a detector without individual hit timestamps (such as Timepix1 or CLICpix). The clustering method only uses positional information: charge-weighted centre of gravity calculation using touching neighbours method, and no timing information. If the pixel information is binary (i.e. no valid charge-equivalent information is available, all pixels are weighed equally). These clusters are stored on the clipboard for each device.

### Parameters
No parameters are used from the configuration file.

### Plots produced
For each detector the following plots are produced:

* Cluster size histogram
* Cluster width (rows, in X) histogram
* Cluster width (columns, in Y) histogram
* Cluster charge histogram
* 2D cluster positions in global coordinates
* 2D cluster positions in local  coordinates

### Usage
```toml
[SpatialClustering]

```
