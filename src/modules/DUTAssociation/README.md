# DUTAssociation
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
Module to establish an association between clusters on a DUT plane and a reference track.
The association allows for cuts in position and time.
For the spatial cut, two options are implemented which can be chosen using `use_cluster_centre`.
By default, the distance of the cluster centre to the track intercept is compared to the `spatial_cut` in local coordinates. If larger than the cut, the cluster is not associated to the track.
The other option is to compare the distance of the closest pixel of a cluster to the track intercept (also in local coordinates).
This option can be chosen, e.g. for an efficiency analysis, when the cluster centre might be pulled away from the track intercept by a delta electron in the silicon.

### Parameters
* `spatial_cut`: Maximum spatial distance in local coordinates in x- and y-direction allowed between cluster and track for association with the DUT. Expects two values for the two coordinates, defaults to twice the pixel pitch.
* `timing_cut`: Maximum time difference allowed between cluster and track for association for the DUT. Default value is `200ns`.
* `use_cluster_centre`: If set true, the cluster centre will be compared to the track position for the spatial cut. If false, the nearest pixel will be used. Defaults to `true`.

### Plots produced
No histograms are produced.

### Usage
```toml
[DUTAssociation]
spatial_cut = 100um, 50um
timing_cut = 200ns
use_cluster_centre = true

```
