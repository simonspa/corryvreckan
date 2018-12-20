# AnalysisEfficiency
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module measures the efficiency of the device under test by comparing its cluster positions with the interpolated track position at the DUT.

### Parameters
* `time_cut_frameedge`: Parameter to discard telescope tracks at the frame edges (start and end of the current event window). Defaults to `20ns`.
* `chi2ndof_cut`: Acceptance criterion for telescope tracks, defaults to a value of `3`.

### Plots produced
* 2D Map of in-pixel efficiency
* 2D Map of the chip efficiency in local coordinates
* 2D Map of the chip efficiency on global coordinates

### Usage
```toml
[AnalysisEfficiency]

```
