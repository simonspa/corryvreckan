# TestAlgorithm
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>), Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functional   

### Description
This module collects `pixel` and `cluster` objects from the clipboard and creates correlation and timing plots with respect to the reference detector.


### Parameters
* `make_correlations`: Boolean to change if correlation plots should be outputted. Default value is `false`.
* `do_timing_cut`: Boolean to switch on/off the cut on cluster times for correlations. Defaults to `false`.
* `timing_cut`: maximum time difference between clusters to be taken into account. Only used if `do_timing_cut` is set to `true`, defaults to `100ns`.
* `correlation_time_vs_time`: Enable plotting of time correlation as a function of time. Default value is `false` because of the time required to fill the histogram with many bins.

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
