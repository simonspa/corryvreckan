# AnalysisEfficiency
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch), Jens Kroeger (jens.kroeger@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module measures the efficiency of the device under test by comparing its cluster positions with the interpolated track position at the DUT.
It also comprises a range of histograms to investigate where inefficiencies might come from.

### Parameters
* `pixel_tolerance`: Parameter to discard tracks, which are extrapolated to
the edge of the DUT. Defaults to `1.`, which excludes column/row zero and max.  
* `time_cut_frameedge`: Parameter to discard telescope tracks at the frame edges (start and end of the current event window). Defaults to `20ns`.
* `chi2ndof_cut`: Acceptance criterion for telescope tracks, defaults to a value of `3`.
* `inpixel_bin_size`: Parameter to set the bin size of the in-pixel 2D efficiency histogram. This should be given in units of distance and the same value is used in both axes. Defaults to `1.0um`.

### Plots produced
* 2D histograms:
  * 2D Map of in-pixel efficiency
  * 2D Map of the chip efficiency in local coordinates, filled at the position of the track intercept point
  * 2D Map of the chip efficiency on global coordinates, filled at the position of the track intercept point
  * 2D Map of the chip efficiency in local coordinates, filled at the position of the associated cluster centre
  * 2D Map of the chip efficiency on global coordinates, filled at the position of the associated cluster centre
  * 2D Map of the position difference of a track (with associated cluster) to the previous track
  * 2D Map of the position difference of a track (without associated cluster) to the previous track
* 1D histograms:
  * Histogram of all single-pixel efficiencies
  * Histogram of time difference of the matched track time (with associated cluster) to the previous track
  * Histogram of time difference of the non-matched track time (without associated cluster) to the previous track
  * Histogram of row difference of the matched track time (with associated cluster) to the previous track
  * Histogram of row difference of the non-matched track time (without associated cluster) to the previous track
  * Histogram of column difference of the matched track time (with associated cluster) to the previous track
  * Histogram of column difference of the non-matched track time (without associated cluster) to the previous track
  * Histogram of the time difference of a matched cluster (with associated cluster) to a previous hit (not matter if noise or track)
  * Histogram of the time difference of a non-matched cluster (without associated cluster) to a previous hit (not matter if noise or track)
* Other:
  * Value of total efficiency as `TEfficiency` including (asymmetric) error bars
  * Value of total efficency as `TName` so it can be read off easily by eye from the root file

### Usage
```toml
[AnalysisEfficiency]
chi2ndof_cut = 5
```
