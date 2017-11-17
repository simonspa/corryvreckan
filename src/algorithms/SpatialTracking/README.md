## Algorithm: SpatialTracking
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)   
**Status**: Functional  

#### Description
This algorithm performs track finding using only positional information (no timing information). It is based on a linear extrapolation along the Z-axis, followed by a nearest neighbour search.


#### Parameters
* `spatialCut`: Cut on the maximum distance between the track and cluster for them to be considered associated. Default value is `200um`.
* `minHitsOnTrack`: The minimum number of planes with clusters associated to a track for it to be stored. Default value is `6`.
* `excludeDUT`: Boolean to set if the DUT should be included in the track fitting. Default value is `true`.
* `DUT`: Name of the DUT plane.

#### Plots produced
* Track chi^2 histogram
* Clusters per track histogram
* Tracks per event histogram
* Track angle in X histogram
* Track angle in Y histogram

Plots produced per device:
* Residual in X
* Residual in Y

#### Usage
```toml
[SpatialTracking]
spatialCut = 0.2
minHitsOnTrack = 5
excludeDUT = true
DUT = "W0005_H03"
```
