## RegionOfInterest
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional

#### Description
This module allows marking tracks to be within a region of interest (ROI) on the detector marked as `DUT`. This information can be used in later analyses to restrict the selection of tracks to certain regions of the DUT.

The ROI is defined as a polynomial in local pixle coordinates of the DUT. E.g. a rectangle could be defined by providing the four corners of the shape via

```toml
roi = [1, 1], [1, 120], [60, 120], [60, 1]
```

If this module is used but no `roi` parameter is provided in the configuration, all tracks which have an intercept with the DUT are considered to be within the ROI.
If this module is not used, all tracks default to be outside the ROI.

#### Parameters
* `roi`: A region of interest, given as polygon by specifying a matrix of pixel coordinates. Defaults to no ROI defined.

#### Plots produced
No plots are produced.

#### Usage
```toml
[RegionOfInterest]
# Define a rectangle as region of interest:
roi = [1, 1], [1, 120], [60, 120], [60, 1]
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
