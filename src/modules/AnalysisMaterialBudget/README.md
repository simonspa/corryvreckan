# AnalysisMaterialBudget
**Maintainer**: Paul Schuetze (paul.schuetze@desy.de)
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This module analyses the kink angle distributions at the position of a scatterer. This requires the prior use of `TrackingMultiplet`.
A material budget image is computed, which represents the widths of the scattering distributions of individual image cells, taking into account all particles traversing this cell.
Further information on this technique can be found in [@material_budget_imaging]

### Parameters
* `image_size`: Two dimensional Field of view extent for all histograms. Defaults to 10mm by 10 mm.
* `cell_size`: Binning of histograms and image cell sizes for Material Budget Images in two dimensions. Defaults to 50um by 50um.
* `angle_cut`: Maximum kink angle to evaluate. Defaults to 100 mrad.
* `min_cell_content`: Minimum number of registered kink angles per image cell required for the cell evaluation in the Material Budget Image.
* `live_update`: Determines whether the Material Budget Image is updated during run time. Otherwise this process is done only once during the finalisation. Defaults to `true`.

### Plots produced
* Histogram of kink angles in x/y
* Profile of squared kink angles along x/y
* 2D profile of squared kink angles (Material Budget Image)
* 2D histogram of angle distribution width per image cell (Material Budget Image)
* Histogram of the number of registered kink angles per image cell

### Usage
```toml
[AnalysisMaterialBudget]
image_size = 20mm 10mm
cell_size = 100um 100um

```

[@material_budget_imaging]: https://doi.org/10.1063/1.5005503