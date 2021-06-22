# AnalysisTracks
**Maintainer**: Lennart Huth (lennart.huth@desy.de)
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
Study reconstructed tracks in each event by calculating the distance between
tracks in an event. In addition cluster assignments to multiple tracks is
plotted. Finally the number of tracks is correlated with the number of tracks
reconstructed.

### Parameters
No parameters are used from the configuration file.

### Plots produced
For each detector the following plots are produced:

* 2D histogram of number of tracks versus clusters
* 1D histogram of absolute distance between tracks
* 1D histogram of number of times a cluster is used in a track

### Usage
```toml
[AnalysisTracks]

```

