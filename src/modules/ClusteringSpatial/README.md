# ClusteringSpatial
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functioning

### Description
This module clusters the input data of a detector without individual hit timestamps.
The clustering method only uses positional information: either charge-weighted center-of-gravity or arithmetic mean calculation using touching neighbors method, and no timing information.
If the pixel information is binary (i.e. no valid charge-equivalent information is available), the arithmetic mean is calculated for the position.
Also, if one pixel of a cluster has charge zero, the arithmetic mean is calculated even if charge-weighting is selected because it is assumed that the zero-reading is false and does not to represent a low charge but an unknown value.
These clusters are stored on the clipboard for each device.

### Parameters
* `use_trigger_timestamp`: If true, the first trigger timestamp of the Corryvreckan event is set as the cluster timestamp. Caution when using this method for very long events containing multiple triggers. If false, the last pixel added to the cluster defines the timestamp. Default value is `false`.
* `charge_weighting`: If true, calculate a charge-weighted mean for the cluster center. If false, calculate the simple arithmetic mean. Defaults to `true`.
* `reject_by_roi`: If true, clusters positioned outside the ROI set for the detector will be rejected. Defaults to `false`.

### Plots produced
For each detector the following plots are produced:

* Histograms for cluster size, seed charge, width (columns/X and rows/Y)
* Cluster charge histogram
* 2D cluster positions in global and local coordinates
* Cluster times
* Cluster multiplicity

### Usage
```toml
[ClusteringSpatial]
use_trigger_timestamp = true
```
