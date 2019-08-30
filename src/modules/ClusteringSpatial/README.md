# ClusteringSpatial
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functioning

### Description
This module clusters the input data of a detector without individual hit timestamps. The clustering method only uses positional information: charge-weighted centre of gravity calculation using touching neighbours method, and no timing information. If the pixel information is binary (i.e. no valid charge-equivalent information is available), the arithmetic mean is calculated for the position. These clusters are stored on the clipboard for each device.

### Parameters
* `use_trigger_timestamp`: If true, set trigger timestamp of Corryvreckan event as cluster timestamp. If false, set pixel timestamp. Default value is `false`.

### Plots produced
For each detector the following plots are produced:

* Cluster size histogram
* Cluster seed charge histogram
* Cluster width (rows, in X) histogram
* Cluster width (columns, in Y) histogram
* Cluster charge histogram
* 2D cluster positions in global coordinates
* 2D cluster positions in local  coordinates
* Cluster times

### Usage
```toml
[SpatialClustering]
use_trigger_timestamp = true
```
