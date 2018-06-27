# Timepix3Clustering
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functional

### Description
This module performs clustering on data from a Timepix3 device. The clustering method is a charge-weighted centre of gravity calculation, using a positional cut and a timing cut on proximity.

Split clusters can be recovered using a larger search radius for neighbouring pixels.

### Parameters
* `timingCut`: The maximum value of the time difference between two pixels for them to be associated in a cluster. Default value is `100ns`.
* `neighbour_radius_col`: Search radius for neighbouring pixels in column direction, defaults to `1` (do not allow split clusters)
* `neighbour_radius_row`:  Search radius for neighbouring pixels in row direction, defaults to `1` (do not allow split clusters)

### Plots produced
For each detector the following plots are produced:
* Cluster size histogram
* Cluster width (rows, in X) histogram
* Cluster width (columns, in Y) histogram
* Cluster ToT histogram
* 2D cluster positions in global coordinates

### Usage
```toml
[Timepix3Clustering]
timingCut = 0.0000002
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
