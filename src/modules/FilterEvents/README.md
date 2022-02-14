# FilterEvents
**Maintainer**: Lennart Huth (lennart.huth@desy.de)  
**Module Type**: *GLOBAL*  
**Status**: Work in progress  

### Description
This module allows for a filtering of events based on the number of tracks/clusters on the clipboard.
Events can be skipped based on the number of tracks and clusters per plane. The accepted cluster range is defined for all planes, that are not a `DUT` or a `Auxiliary`.
Events can also be skipped based on tag values where the tag value is either checked to be witihin a specified range, or matching at least one value of a specified list.

### Parameters

* `min_tracks`: Minimum number of tracks to continue analyzing event. Defaults to `0`.
* `max_tracks`: Maximum number of tracks to continue analyzing event. Defaults to `10`.
* `min_clusters_per_plane`: Minimum number of clusters on each plane to continue analyzing event. Planes of type `DUT` and `auxiliary` are excluded in this cut. Defaults to `0`.
* `max_clusters_per_plane`: Maximum number of clusters on each plane to continue analyzing event. Planes of type `DUT` and `auxiliary` are excluded in this cut. Defaults to `100`.
* `filter_tags` : List of tag value requirements to be fulfilled to continue analyzing event. The list of tag-value pairs should be enclosed in global pair of brackets `[ list_of_pairs ]`. Each tag-value pair should be enclosed in local pair of square brackets `[]`, and each element enclosed in a pair of quotes `""` (ie : `["tag_name", "tag_filter"]`). If the tag filter is to be a range of values (casted to `double` type) the minimum and maximum values should be enclosed in a pair of squared brackets `[]` and separated by a semicolomn `:`. If the tag filter is to be a list of values, they should be separated by a comma `'`.

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
