# DUTAssociation
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
Module to establish an association between clusters on a DUT plane and a reference track.
The association allows for cuts in position and time.
For the spatial cut, two options are implemented which can be chosen using `use_cluster_centre`.

By default, the distance of the closest pixel of a cluster to the track intercept is compared to the spatial cut in local coordinates.
If larger than the cut, the cluster is not associated to the track.
This option can be chosen, e.g. for an efficiency analysis, when the cluster center might be pulled away from the track intercept by a delta electron in the silicon.
The other option is to compare the distance between the cluster center and the track intercept to the spatial cut (also in local coordinates).

### Parameters
* `spatial_cut_rel`: Factor by which the `spatial_resolution` in X and Y of each detector plane will be multiplied. These calculated value are defining an ellipse which is then used as the maximum distance in the XY plane allowed between clusters and a track for association to the track. By default, a relative spatial cut is applied. Absolute and relative spatial cuts are mutually exclusive. Defaults to `3.0`.
* `spatial_cut_abs`: Specifies a set of absolute value (X and Y) which defines an ellipse for the maximum spatial distance in the XY plane between clusters and a track for association to the track. Absolute and relative spatial cuts are mutually exclusive. No default value.
* `time_cut_rel`: Number of standard deviations the `time_resolution` of the detector plane will be multiplied by. This value is then used as the maximum time difference allowed between a DUT cluster and track for association. By default, a relative time cut is applied. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference allowed between DUT cluster and track for association. Absolute and relative time cuts are mutually exclusive. No default value.
* `use_cluster_centre`: If set true, the cluster centre will be compared to the track position for the spatial cut. If false, the nearest pixel in the cluster will be used. Defaults to `false`.

### Plots produced

For the DUT, the following plots are produced:

* Histograms of the distance in X/Y from the cluster to the pixel closest to the track for various cluster sizes
* Histogram of the number of associated clusters per track
* Histogram of the number of clusters discarded by a given cut

### Usage
```toml
[DUTAssociation]
spatial_cut = 100um, 50um
time_cut_rel = 3.0
use_cluster_centre = false

```
