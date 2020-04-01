# TrackingMultiplet
**Maintainer**: Paul Schuetze (paul.schuetze@desy.de)
**Module Type**: *GLOBAL*  
**Status**: Work in progress

### Description
This module performs particle tracking based on the _Multiplet_ track model.
This track model is defined by two `StraightLineTrack`s (_upstream_ & _downstream_). These tracks are connected at a certain position along `z`, where a kink of the track is allowed, representing a scatterer.

The upstream and downstream tracks are determined by adding all combinations of clusters in the first and the last detector plane of the corresponding stream to `StraightLineTrack` candidates.
For each plane the cluster closest to the track is added, while the fit to this track is continuously updated.
Time cuts are taken into account.
Tracks with too few matched clusters can be discarded.

For finding `Multiplet`s the upstream and downstream tracks are matched as follows:
For each upstream track, the downstream track with the lowest matching distance is chosen, where the matching distance is determined via an extrapolation of both arms to the position of the scatterer.

### Parameters
* `upstream_detectors`: Names of detectors associated to the upstream track. No default value.
* `downstream_detectors`: Names of detectors associated to the downstream track. No default value.
* `min_hits_upstream`: Minimum number of associated clusters required to create an upstream track. Default value is the number of upstream detectors.
* `min_hits_downstream`: Minimum number of associated clusters required to create an downstream track. Default value is the number of downstream detectors.
* `scatterer_position`: Position of the scatterer along `z`. No default value.
* `scatterer_matching_cut_`: Maximum allowed distance between the extrapolated positions of an up- and a downstream track at the position of the scatterer. No default value.
* `time_cut_rel`: Factor by which the `time_resolution` of each detector plane will be multiplied. This calculated value is then used as the maximum time difference allowed between clusters and an upstream or downstream track for association to the track. This allows the time cuts between different planes to be detector appropriate. By default, a relative time cut is applied. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference allowed between clusters and an upstream or downstream track for association to the track. Absolute and relative time cuts are mutually exclusive. No default value.
* `spatial_cut_rel`: Factor by which the `spatial_resolution` in x and y of each detector plane will be multiplied. These calculated value are defining an ellipse which is then used as the maximum distance in the XY plane allowed between clusters and an upstream or downstream track for association to the track. This allows the spatial cuts between different planes to be detector appropriate. By default, a relative spatial cut is applied. Absolute and relative spatial cuts are mutually exclusive. Defaults to `3.0`.
* `spatial_cut_abs`: Specifies a set of absolute value (x and y) which defines an ellipse for the maximum spatial distance in the XY plane between clusters and an upstream or downstream track for association to the track. Absolute and relative spatial cuts are mutually exclusive. No default value.


### Plots produced

For both upstream and downstream tracks, the following plots are produced:

* Histogram of the tracks per event
* Histograms of the track angle with respect to the X/Y-axis
* Histograms of the position of the track in X/Y, extrapolated to the position of the scatterer

The following plots are produced only once:

* Histogram of the number of accepted multiplets per event
* Histograms of the matching distance of multiplet candidates in X/Y
* Histograms of the matching distance of accepted multiplets in X/Y
* Histograms of kink angles of accepted multiplets in X/Y

For each detector the following plots are produced:

* Histograms of the track residual in X/Y.

### Usage
```toml
[TrackingMultiplet]
spatial_cut_abs = 100um 100um
upstream_detectors = "telescope0" "telescope1" "telescope2"
downstream_detectors = "telescope3" "telescope4" "telescope5"

scatterer_position = 150mm
scatterer_matching_cut = 50um
```
