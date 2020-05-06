# TrackingSpatial
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This module performs track finding using only positional information (no timing information). It is based on a linear extrapolation along the Z-axis, followed by a nearest neighbor search.


### Parameters
* `spatial_cut_rel`: Factor by which the `spatial_resolution` in x and y of each detector plane will be multiplied. These calculated value are defining an ellipse which is then used as the maximum distance in the XY plane allowed between clusters and a track for association to the track. This allows the spatial cuts between different planes to be detector appropriate. By default, a relative spatial cut is applied. Absolute and relative spatial cuts are mutually exclusive. Defaults to `3.0`.
* `spatial_cut_abs`: Specifies a set of absolute value (x and y) which defines an ellipse for the maximum spatial distance in the XY plane between clusters and a track for association to the track. Absolute and relative spatial cuts are mutually exclusive. No default value.
* `min_hits_on_track`: The minimum number of planes with clusters associated to a track for it to be stored. Default value is `6`.
* `exclude_dut`: Boolean to set if the DUT should be included in the track fitting. Default value is `true`.
* `track_model`: Select the track model used for reconstruction. Defaults to
the only supported fit at the "straightline"

### Plots produced

The following plots are produced only once:

* Histograms of the track $\chi^2$ and track $\chi^2$/degrees of freedom
* Histogram of the clusters per track, and tracks per event
* Histograms of the track angle with respect to the X/Y-axis

For each detector, the following plots are produced:

* Histograms of the residual in X/Y

### Usage
```toml
[TrackingSpatial]
spatial_cut_rel = 5.0
min_hits_on_track = 5
exclude_dut = true
track_model = "straightline"

```
