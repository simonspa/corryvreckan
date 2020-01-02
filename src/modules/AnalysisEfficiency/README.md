# AnalysisEfficiency
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>), Jens Kroeger (<jens.kroeger@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module measures the efficiency of the DUT by comparing its cluster positions with the interpolated track position at the DUT.
It also comprises a range of histograms to investigate where inefficiencies might come from.

### Parameters
* `time_cut_frameedge`: Parameter to discard telescope tracks at the frame edges (start and end of the current event window). Defaults to `20ns`.
* `chi2ndof_cut`: Acceptance criterion for telescope tracks, defaults to a value of `3`.
* `inpixel_bin_size`: Parameter to set the bin size of the in-pixel 2D efficiency histogram. This should be given in units of distance and the same value is used in both axes. Defaults to `1.0um`.

### Plots produced

For the DUT, the following plots are produced:

* 2D histograms:
  * 2D Map of in-pixel efficiency
  * 2D Maps of chip efficiency in local and global coordinates, filled at the position of the track intercept point or at the position of the associated cluster center
  * 2D Maps of the position difference of a track with and without associated cluster to the previous track
  * 2D Map of the distance between track intersection and associated cluster
  
* 1D histograms:
  * Histogram of all single-pixel efficiencies
  * Histograms of time difference of the matched and non-matched track time to the previous track
  * Histograms of the row and column difference of the matched and non-matched track time to the previous track
  * Histograms of the time difference of a matched (non-matched) cluster to a previous hit (not matter if noise or track)
* Other:
  * Value of total efficiency as `TEfficiency` including (asymmetric) error bars
  * Value of total efficiency as `TName` so it can be read off easily by eye from the root file

### Usage
```toml
[AnalysisEfficiency]
chi2ndof_cut = 5
```
