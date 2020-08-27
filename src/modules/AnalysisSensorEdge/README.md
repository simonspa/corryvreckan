# AnalysisSensorEdge
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module produces in-pixel efficiency plots for the sensor edges, particularly interesting for active edge sensors.
For each of the four sensor edges, the efficiency within the pixel is measured using the track position.
All impinging tracks with associated cluster are collected and folded into single pixel cells in order to increase statistics.

In addition, a in-pixel efficiency map is created containing all four edges.
In order to emphasize the edge effect, the pixel cells from the four edges are rotated such that the sensor edge is always aligned with the left edge of the plot.
This means, the pixel cell from the last column is mirrored horizontally, the cell from the first row is rotated by 90 degrees, and the cell from the last row is mirrored vertically and rotated by 90 degrees.

### Parameters
* `inpixel_bin_size`: Parameter to set the bin size of the in-pixel 2D efficiency histogram. This should be given in units of distance and the same value is used in both axes. Defaults to `1.0um`.

### Plots produced
For the DUT, the following plots are produced:

* 2D maps of the in-pixel efficiency for the individual sensor edges (first and last columns, first and last rows)
* 2D map of the in-pixel efficiency for all sensor edges together

### Usage
```toml
[AnalysisSensorEdge]
inpixel_bin_size = 1.0um
```
