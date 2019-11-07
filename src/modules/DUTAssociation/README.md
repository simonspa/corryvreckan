# DUTAssociation
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
Module to establish an association between clusters on a DUT plane and a reference track.
The association allows for cuts in position and time.
For the spatial cut, two options are implemented which can be chosen using `use_cluster_centre`.

By default, the distance of the closest pixel of a cluster to the track intercept is compared to the `spatial_cut` in local coordinates.
If larger than the cut, the cluster is not associated to the track.
This option can be chosen, e.g. for an efficiency analysis, when the cluster centre might be pulled away from the track intercept by a delta electron in the silicon.
The other option is to compare the distance between the cluster centre and the track intercept to the `spatial_cut`(also in local coordinates).

### Parameters
* `spatial_cut`: Maximum spatial distance in local coordinates in x- and y-direction allowed between cluster and track for association with the DUT. Expects two values for the two coordinates, defaults to twice the pixel pitch.
* `time_cut_rel`: Number of standard deviations the `time_resolution` of the detector plane will be multiplied by. This value is then used as the maximum time difference allowed between a DUT cluster and track for association. By default, a relative time cut is applied. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference allowed between DUT cluster and track for association. Absolute and relative time cuts are mutually exclusive. No default value.
* `use_cluster_centre`: If set true, the cluster centre will be compared to the track position for the spatial cut. If false, the nearest pixel in the cluster will be used. Defaults to `false`.

### Plots produced
* distance in x of cluster centre to track minus closest pixel to track
* distance in y of cluster centre to track minus closest pixel to track
* distance in x of cluster centre to track minus closest pixel to track for pixels with column width = 1
* distance in y of cluster centre to track minus closest pixel to track for pixels with row width = 1
* distance in x of cluster centre to track minus closest pixel to track for pixels with column width = 2
* distance in y of cluster centre to track minus closest pixel to track for pixels with row width = 2
* distance in x of cluster centre to track minus closest pixel to track for pixels with column width = 3
* distance in y of cluster centre to track minus closest pixel to track for pixels with row width = 3
* distribution of number of associated clusters per track
* Number of clusters discarded by a given cut (currently only spatial and timing cuts are implemented)

### Usage
```toml
[DUTAssociation]
spatial_cut = 100um, 50um
time_cut_rel = 3.0
use_cluster_centre = false

```
