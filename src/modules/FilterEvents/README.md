# FilterEvents
**Maintainer**: Lennart Huth (lennart.huth@desy.de)  
**Module Type**: *GLOBAL*  
**Status**: Work in progress  

### Description
This module allows for a filtering of events based on the number of tracks/clusters on the clipboard.
Currently, events can be skipped based on the number of tracks and clusters per plane. The accepted cluster range is defined for all planes, that are not a `DUT` or a `Auxiliary`.

### Parameters

* `min_tracks`: Minimum number of tracks to continue analyzing event. Defaults to `0`.
* `max_tracks`: Maximum number of tracks to continue analyzing event. Defaults to `10`.
* `min_clusters_per_plane`: Minimum number of clusters on each plane to continue analyzing event. Planes of type `DUT` and `auxiliary` are excluded in this cut. Defaults to `0`.
* `max_clusters_per_plane`: Maximum number of clusters on each plane to continue analyzing event. Planes of type `DUT` and `auxiliary` are excluded in this cut. Defaults to `100`.

### Plots produced
* 1D histogram of the events filtered by a certain parameter and the total number of filtered events.

### Usage
```toml
[FilterEvents]
min_tracks = 0
max_tracks = 10
min_clusters_per_plane = 0
max_clusters_per_plane = 100
```
