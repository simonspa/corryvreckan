# AnalysisDUT
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
Generic analysis module for all types of detectors. Produces a number of commonly used plots to gauge detector performance and allows to discard tracks based on their chi2/ndf value.
If a region of interest (ROI) is defined for the detector under investigation, only tracks from within this region are evaluated, all others are discarded.

### Parameters
* `time_cut_frameedge`: Parameter to discard telescope tracks at the frame edges (start and end of the current CLICpix2 frame). Defaults to `20ns`.
* `spatial_cut_sensoredge`: Parameter to discard telescope tracks at the sensor edges in fractions of pixel pitch. Defaults to `0.5`.
* `chi2ndof_cut`: Acceptance criterion for the maximum telescope tracks chi2/ndf, defaults to a value of `3`.
* `use_closest_cluster`: If `true` the cluster with the smallest distance to the track is used if a track has more than one associated cluster. If `false`, loop over all associated clusters. Defaults to `true`.
* `n_time_bins`: Number of bins in the time residual and correlation histograms. Defaults to `20000`.
* `time_binning`: Bin width in the time residual and correlation histograms. Defaults to `0.1ns`.
* `n_charge_bins`: Number of bins for charge in histograms. Defaults to `1000`.
* `charge_histo_range`: Axis range for charge axes in histograms. Defaults to `1000`.
* `n_raw_bins`: Number of bins for pixel raw values in histograms. Defaults to n_charge_bins if not specified.
* `raw_histo_range`: Axis range for pixel raw values axes in histograms. Defaults to charge_histo_range if not specified.
* `correlations`: If `true`, correlation plots between all (before and after applying cuts) tracks and all clusters on the DUT (i.e. associated + non-associated) are created. Defaults to `false`.
* `inpixel_bin_size`: The bin size for inpixel plots. Defaults to `500 nm`
### Plots produced

For the DUT, the following plots are produced:

* 2D histograms:
    * Maps of the position, size, and charge/raw value of associated clusters
    * Maps of all pixels of associated clusters
    * Maps of the in-pixel efficiencies in local/global coordinates
    * Maps of matched/non-matched track positions
    * Maps of cluster/seed charge vs. row
    * Maps of in-pixel time/spatial resolution, mean time delay
* 1D histograms:
    * Histograms of the cluster size of associated clusters in X/Y
    * Histogram of the charge/raw values of associated clusters
    * Various histograms for track residuals and correlations for different cluster sizes
    * Histograms of pixel timestamp minus seed pixel timestamp for different cluster sizes
    * Profiles of cluster charge vs. column/row

### Usage
```toml
[AnalysisDUT]
time_cut_frameedge = 50ns
chi2ndof_cut = 5.
use_closest_cluster = false
```
