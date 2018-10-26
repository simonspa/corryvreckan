# AnalysisEfficiency
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Status**: Functional

### Description
This module measures the efficiency of the device under test by comparing its cluster positions with the interpolated track position at the DUT.

### Parameters
* `timeCutFrameEdge`: Parameter to discard telescope tracks at the frame edges (start and end of the current event window). Defaults to `20ns`.
* `chi2ndofCut`: Acceptance criterion for telescope tracks, defaults to a value of `3`.

### Plots produced
* 2D Map of in-pixel efficiency
* 2D Map of the chip efficiency in local coordinates
* 2D Map of the chip efficiency on global coordinates

### Usage
```toml
[AnalysisEfficiency]

```
