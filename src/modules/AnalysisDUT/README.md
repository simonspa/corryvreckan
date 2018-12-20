# AnalysisDUT
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Work in progress

### Description
Analysis module for CLICpix2 prototypes. This module is still work in progress, changes to functionality and behaviour are to be expected.

### Parameters
* `time_cut_frameedge`: Parameter to discard telescope tracks at the frame edges (start and end of the current CLICpix2 frame). Defaults to `20ns`.
* `spatial_cut`: Spatial cut for associating a track with a DUT cluster, defaults to `50um`.
* `chi2ndof_cut`: Acceptance criterion for telescope tracks, defaults to a value of `3`.

### Plots produced
* 2D Map of associated cluster positions
* 2D Map of cluster sizes for associated clusters
* 2D Map of cluster ToT values from associated clusters
* 2D Map of associated hits
* 2D Map of associated hits within the defined region-of-interest
* Distribution of pixel ToT values from associated clusters
* 2D Map of pixel ToT values from associated clusters
* Track residuals in X and Y
* Track residuals for 1-pixel-clusters in X and Y
* Track residuals for 2-pixel-clusters in X and Y
* Distribution of cluster Tot values from associated clusters
* Distribution of sizes from associated clusters
* 2D Map of in-pixel efficiency
* 2D Map of the chip efficiency in local coordinates
* 2D Map of the chip efficiency on global coordinates
* 2D Map of track positions associated to a cluster
* 2D Map of track positions not associated to a cluster

### Usage
```toml
[CLICpix2Analysis]
timeCutFrameEdge = 50ns
```
