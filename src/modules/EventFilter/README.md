# EventFilter
**Maintainer**: Lennart Huth (lennart.huth@desy.de)
**Module Type**: *GLOBAL*
**Status**: Work in progress

### Description
This module allows for a filtering of events based on the number of tracks/clsuters on the clipboard.
Currently, events can be skipped based on the number of tracks and clusters per plane. The accepted cluster range is defined for all planes, that are not a `DUT` or a `Auxiliary`. It is planned to also allow for more specific cuts on individual detectors.

### Parameters
*`minTracks`: Minimum number of tracks to continue analysing event. Defaults to `0`.
*`maxTracks`: Maximum number of tracks to continue analysing event. Defaults to `10`.
*`minClusters_per_plane`: Minimum number of clusters on each plane to continue analysing event. Planes of type `DUT` and `auxiliary` are excluded in this cut. Defaults to `0`.
*`maxClusters_per_plane`: Maximum number of clusters on each plane to continue analysing event. Planes of type `DUT` and `auxiliary` are excluded in this cut. Defaults to `100`.
### Plots produced
None
### Usage
```toml
[EventFilter]
mimTracks = 0
maxTracks = 10
minClusters_per_plane = 0
maxClusters_per_plane = 100
```
