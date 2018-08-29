# Timepix1Correlator
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)   
**Status**: Functional   

### Description
This module collects `pixel` and `cluster` objects from the clipboard for a Timepix1 device and creates correlation and hitmap plots.

### Parameters
* `reference`: Name of the plane to be used as the reference for the correlation plots.

### Plots produced
For each device the following plots are produced:

* 2D hitmap in local coordinates
* 2D hitmap in global coordinates
* Cluster size histogram
* Clusters per event histogram
* Correlation plot in X
* Correlation plot in Y

### Usage
```toml
[Timepix1Correlator]
reference = "W0013_E03"
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
