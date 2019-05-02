# AnalysisEfficiency
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module measures the efficiency of the device under test by comparing its cluster positions with the interpolated track position at the DUT.

### Parameters
* `pixel_tolerance`: Parameter to discard tracks, which are extrapolated to
the edge of the DUT. Defaults to `1.`, which excludes column/row zero and max.  
* `time_cut_frameedge`: Parameter to discard telescope tracks at the frame edges (start and end of the current event window). Defaults to `20ns`.
* `chi2ndof_cut`: Acceptance criterion for telescope tracks, defaults to a value of `3`.
* `inpixel_bin_size`: Parameter to set the bin size of the in-pixel 2D efficiency histogram. This should be given in units of distance and the same value is used in both axes. Defaults to `1.0um`.

### Plots produced
* 2D Map of in-pixel efficiency
* 2D Map of the chip efficiency in local coordinates, filled at the position of the track intercept point
* 2D Map of the chip efficiency on global coordinates, filled at the position of the track intercept point
* 2D Map of the chip efficiency in local coordinates, filled at the position of the associated cluster centre
* 2D Map of the chip efficiency on global coordinates, filled at the position of the associated cluster centre

### Usage
```toml
[AnalysisEfficiency]

```
