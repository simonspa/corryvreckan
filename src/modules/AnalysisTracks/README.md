# AnalysisTracks
**Maintainer**: Lennart Huth (lennart.huth@desy.de)
**Module Type**: *GLOBAL*  
**Status**: Work in progress

### Description
Check the reconstructed tracks for track distances, double usage of clusters, etc

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

