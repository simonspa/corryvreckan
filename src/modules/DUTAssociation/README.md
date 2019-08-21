# DUTAssociation
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
Module to establish an association between clusters on a DUT plane and a reference track.
The association allows for cuts in position and time.
For the spatial cut, two options are implemented which can be chosen using `use_cluster_centre`.

By default, the distance of the closest pixel of a clusters to the track intercept is compared to the `spatial_cut` in local coordinates.
If larger than the cut, the cluster is not associated to the track.
This option can be chosen, e.g. for an efficiency analysis, when the cluster centre might be pulled away from the track intercept by a delta electron in the silicon.
The other option is to compare the distance of the cluster centre to the track intercept (also in local coordinates).

### Parameters
* `spatial_cut`: Maximum spatial distance in local coordinates in x- and y-direction allowed between cluster and track for association with the DUT. Expects two values for the two coordinates, defaults to twice the pixel pitch.
* `timing_cut`: Maximum time difference allowed between cluster and track for association for the DUT. Default value is `200ns`.
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
timing_cut = 200ns
use_cluster_centre = true

```
