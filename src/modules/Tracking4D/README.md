# Tracking4D
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This module performs a basic tracking method.

Clusters from the first plane in Z (named the seed plane) are related to clusters close in time on the other detector planes using straight line tracks. The DUT plane can be excluded from the track finding.

### Parameters
* `timing_cut`: Maximum time difference allowed between clusters for association. Default value is `200ns`.
* `spatial_cut`: Maximum spatial distance in the XY plane allowed between clusters for association for the telescope planes. Default value is `0.2mm`.
* `min_hits_on_track`: Minium number of associated clusters needed to create a track, equivalent to the minimum number of planes required for each track. Default value is `6`.
* `exclude_dut`: Boolean to choose if the DUT plane is included in the track finding. Default value is `true`.
* `require_detector`: Name of detector which is required to have a cluster on the track. If empty, no detector is required. Default is empty.
* `use_avg_cluster_timestamp`: Boolean to choose how track timestamp is determined. If `true`, the average timestamp of all clusters on the track will be used. If `false` a detector can be defined using the `detector_to_set_track_timestamp`. Default is `true`.
* `detector_to_set_track_timestamp`: If `use_avg_cluster_timestamp = false`, a detector can be defined to provide the track timestamp. This detector also needs to be chosen set as `required_detector`.

### Plots produced
* Track chi^2 histogram
* Track chi^2 /degrees of freedom histogram
* Clusters per track histogram
* Tracks per event histogram
* Track angle with respect to X-axis histogram
* Track angle with respect to Y-axis histogram

For each detector the following plots are produced:

* Residual in X
* Residual in Y
* Residual in X for clusters with a width in X of 1
* Residual in Y for clusters with a width in X of 1
* Residual in X for clusters with a width in X of 2
* Residual in Y for clusters with a width in X of 2
* Residual in X for clusters with a width in X of 3
* Residual in Y for clusters with a width in X of 3

### Usage
```toml
[BasicTracking]
min_hits_on_track = 4
spatial_cut = 300um
timing_cut = 200ns
exclude_dut = true
```
