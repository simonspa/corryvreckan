# Clustering4D
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module performs clustering for detectors with valid individual hit timestamps. The clustering method is a charge-weighted centre of gravity calculation, using a positional cut and a timing cut on proximity. If the pixel information is binary (i.e. no valid charge-equivalent information is available), the arithmetic mean is calculated for the position.

Split clusters can be recovered using a larger search radius for neighbouring pixels.

### Parameters
* `timing_cut`: The maximum value of the time difference between two pixels for them to be associated in a cluster. Default value is `100ns`.
* `neighbour_radius_col`: Search radius for neighbouring pixels in column direction, defaults to `1` (do not allow split clusters)
* `neighbour_radius_row`:  Search radius for neighbouring pixels in row direction, defaults to `1` (do not allow split clusters)

### Plots produced
For each detector the following plots are produced:

* Cluster size histogram
* Cluster seed charge histogram
* Cluster width (rows, in X) histogram
* Cluster width (columns, in Y) histogram
* Cluster charge histogram
* 2D cluster positions in global coordinates
* Cluster times
* Cluster multiplicity

### Usage
```toml
[Clustering4D]
timing_cut = 200ns
```
