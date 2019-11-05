# TrackingSpatial
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)
**Module Type**: *GLOBAL*
**Status**: Functional

### Description
This module performs track finding using only positional information (no timing information). It is based on a linear extrapolation along the Z-axis, followed by a nearest neighbour search.


### Parameters
* `spatial_cut`: Cut on the maximum distance between the track and cluster for them to be considered associated. Default value is `200um`.
* `min_hits_on_track`: The minimum number of planes with clusters associated to a track for it to be stored. Default value is `6`.
* `exclude_dut`: Boolean to set if the DUT should be included in the track fitting. Default value is `true`.
* `track_model`: Select the track model used for reconstruction. Defaults to
the only supported fit at the "straightline"

### Plots produced
* Track chi^2 histogram
* Clusters per track histogram
* Tracks per event histogram
* Track angle in X histogram
* Track angle in Y histogram

Plots produced per device:

* Residual in X
* Residual in Y

### Usage
```toml
[SpatialTracking]
spatial_cut = 0.2mm
min_hits_on_track = 5
exclude_dut = true
track_model = "straightline"

```
