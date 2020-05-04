# Clustering4D
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module performs clustering for detectors with valid individual hit timestamps.
The clustering method is either an arithmetic mean or a a charge-weighted centre-of-gravity calculation, using a positional cut and a timing cut on proximity.
If the pixel information is binary (i.e. no valid charge-equivalent information is available), the arithmetic mean is calculated for the position.
Also, if one pixel of a cluster has charge zero, the arithmetic mean is calculated even if charge-weighting is selected because it is assumed that the zero-reading is false and does not to represent a low charge but an unknown value.
Thus, the  arithmetic mean is safer.

Split clusters can be recovered using a larger search radius for neighbouring pixels.

### Parameters
* `time_cut_rel`: Number of standard deviations the `time_resolution` of the detector plane will be multiplied by. This value is then used as the maximum time difference allowed between pixels for association to a cluster. By default, a relative time cut is applied. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference allowed between pixels for association to a cluster. Absolute and relative time cuts are mutually exclusive. No default value.
* `neighbour_radius_col`: Search radius for neighbouring pixels in column direction, defaults to `1` (do not allow split clusters)
* `neighbour_radius_row`:  Search radius for neighbouring pixels in row direction, defaults to `1` (do not allow split clusters)
* `charge_weighting`: If true, calculate a charge-weighted mean for the cluster centre. If false, calculate the simple arithmetic mean. Defaults to `true`.
* `use_earliest_pixel` : If `true`, the pixel with the earliest timestamp will be used to set the cluster timestamp. If `false`, the pixel with the largest charge will be used. Defaults to `false`.

### Plots produced
For each detector the following plots are produced:

* Histograms for cluster size, seed charge, width (columns/X and rows/Y)
* Cluster charge histogram for all clusters as well as 1-px, 2-px, 3-px clusters
* 2D cluster positions in global coordinates
* Cluster times
* Cluster multiplicity
* Histogram with time difference of pixel time and cluster time for all pixels in a cluster

### Usage
```toml
[Clustering4D]
time_cut_rel = 3.0
```
