# TestAlgorithm
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>), Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functional   

### Description
This module collects `pixel` and `cluster` objects from the clipboard and creates correlation and timing plots with respect to the reference detector.


### Parameters
* `make_correlations`: Boolean to change if correlation plots should be outputted. Default value is `false`.
* `do_time_cut`: Boolean to switch on/off the cut on cluster times for correlations. Defaults to `false`.
* `time_cut_rel`: Number of standard deviations the `time_resolution` of the detector plane will be multiplied by. This value is then used as the maximum time difference for cluster correlation if `do_time_cut = true`. A relative time cut is applied by default when `do_time_cut = true`. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference allowed for cluster correlation if `do_time_cut = true`. Absolute and relative time cuts are mutually exclusive. No default value.
* `correlation_vs_time`: Enable plotting of spatial and time correlation as a function of time. Default value is `false` because of the time required to fill the histogram with many bins.

### Plots produced
For each device the following plots are produced:

* 2D hitmap
* 2D event times histogram
* Correlation in X
* Correlation between X(reference) and Y
* Correlation in Y
* Correlation between Y(reference) and X
* 2D correlation in X in local coordinates
* 2D correlation in Y in local coordinates
* 2D correlation between columns
* 2D correlation between columns(reference) and rows
* 2D correlation between rows
* 2D correlation between rows(reference) and columns
* 2D correlation in X in global coordinates
* 2D correlation in Y in global coordinates
* Correlation in X in global coordinates versus time
* Correlation in Y in global coordinates versus time
* Correlation times (nanosecond binning) histogram, range covers 2 * `timing_cut`
* 2D correlation times over time histogram
* Correlation times (on pixel level, all other histograms take clusters)
* Correlation times (integer values) histogram

### Usage
```toml
[TestAlgorithm]
make_correlations = true
```
