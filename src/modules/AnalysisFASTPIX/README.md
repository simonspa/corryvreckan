# AnalysisFASTPIX
**Maintainer**: Eric Buschmann (eric.buschmann@cern.ch)
**Module Type**: *DUT*
**Status**: Functional

### Description
Analysis module for FASTPIX detector.

### Parameters
* `chi2ndof_cut`: Acceptance criterion for the maximum telescope tracks chi2/ndf, defaults to a value of `3`.
* `time_cut_frameedge`: Parameter to discard telescope tracks at the event edges. Defaults to `20ns`.
* `time_cut_deadtime`: Parameter to discard telescope tracks during oscilloscope dead time. Defaults to `5us`.
* `time_cut_trigger`: Parameter to include telescope tracks around oscilloscope trigger. Defaults to +/-`250ns`.
* `use_closest_cluster`: If `true` the cluster with the smallest distance to the track is used if a track has more than one associated cluster. If `false`, loop over all associated clusters. Defaults to `true`.
* `roi_inner`: If `true` all pixels inside the matrix with a one pixel wide border around the edge are included in the ROI. If `false` the ROI is defined as a rectangle with a margin of `roi_margin_x` pixels in x and `roi_margin_y` pixels in y. Defaults to `true`.
* `roi_margin_x`: Used only when `roi_inner` is `false`. ROI margin on the left and right edge of the matrix in pixels. Defaluts to `1`.
* `roi_margin_y`: Used only when `roi_inner` is `false`. ROI margin on the top and bottom edge of the matrix in pixels. Defaults to `0.5`.
* `triangle_bins`: Controls the number of bins in a hexagonal histogram. Each hexagon is split into 6 triangles with `triangle_bins`^2 bins. Defaults to `15`, i.e. 6*15^2 = 1350 bins.
* `bin_size`: Bin size in 2D hit maps. Defaults to `2.5um`.
* `hist_scale`: Scales 2D hit maps to `hist_scale` times the width and height of the pixel matrix.  Defaults to `1.75`.

### Plots produced

For the DUT, the following plots are produced:

* 2D histograms:
    * Maps of track intercepts with and without associated FASTPIX trigger with different association cuts
    * Maps of track intercepts with associated cluster
    * Maps of cluster size, seed pixel ToT, cluster ToT
    * In-pixel versions of 2D histograms for tracks in ROI
* 1D histograms:
    * Histograms of cluster size, seed pixel ToT, cluster ToT
    * Various histograms for data decoding inefficiencies

### Usage
```toml
[AnalysisFASTPIX]
time_cut_deadtime = 5us

```
