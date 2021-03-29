# Correlations
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>), Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module collects `pixel` and `cluster` objects from the clipboard and creates correlation and timing plots with respect to the reference detector.
No plots are produced for `aux` devices.

### Parameters
* `do_time_cut`: Boolean to switch on/off the cut on cluster times for correlations. Defaults to `false`.
* `time_cut_rel`: Number of standard deviations the `time_resolution` of the detector plane will be multiplied by. This value is then used as the maximum time difference for cluster correlation if `do_time_cut = true`. A relative time cut is applied by default when `do_time_cut = true`. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference allowed for cluster correlation if `do_time_cut = true`. Absolute and relative time cuts are mutually exclusive. No default value.
* `correlation_vs_time`: Enable plotting of spatial and time correlation as a function of time. Default value is `false` because of the time required to fill the histogram with many bins.
* `time_binning`: Specifies the binning of the time correlations plots. Defaults to `1ns`.

### Plots produced
For each device the following plots are produced:

* 2D histograms:
    * Hitmaps on pixel and cluster-level
    * Time correlation over time (on cluster and pixel level)
    * Time correlation over raw value (both on cluster and pixel level)
    * Correlations in X/Y, columns/columns, columns/rows, rows/rows and rows/columns in local coordinates
    * Correlations in X and Y in global coordinates

* 1D histograms:
    * Correlations between the device and the reference in X/X, Y/Y, X/Y, Y/X
    * Correlations in X and Y in global coordinates versus time
    * Correlation times (nanosecond binning) histogram, range covers 2 * `timing_cut`
    * Correlation times (on pixel level, all other histograms take clusters)
    * Correlation times (integer values) histogram

### Usage
```toml
[Correlations]
do_time_cut = true
time_cut_rel = 5.0
```
