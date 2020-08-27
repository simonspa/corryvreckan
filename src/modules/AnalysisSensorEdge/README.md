# AnalysisSensorEdge
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Immature

### Description
This module produces efficiency plots for the sensor edges, particularly interesting for active ende sensors.

### Parameters
* `inpixel_bin_size`: Parameter to set the bin size of the in-pixel 2D efficiency histogram. This should be given in units of distance and the same value is used in both axes. Defaults to `1.0um`.

### Plots produced
For the DUT, the following plots are produced:

* 2D Maps of the in-pixel efficiency for all sensor edges (first and last columns, first and last rows)

### Usage
```toml
[AnalysisSensorEdge]
inpixel_bin_size = 1.0um
```
