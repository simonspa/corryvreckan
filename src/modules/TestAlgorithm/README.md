# TestAlgorithm
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>) ; Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functional   

### Description
This module collects `pixel` and `cluster` objects from the clipboard and creates correlation and timing plots.


### Parameters
* `makeCorrelatons`: Boolean to change if correlation plots should be outputted. Default value is `false`.
* `reference`: Name of the plane to be used as the reference for the correlation plots.
* `timingCut`: maximum time difference between clusters to be taken into account. Defaults to `100ns`.

### Plots produced
For each device the following plots are produced:

* 2D hitmap
* 2D event times histogram
* Correlation in X
* Correlation in Y
* 2D correlation in X in global coordinates
* 2D correlation in Y in global coordinates
* 2D correlation in X in local coordinates
* 2D correlation in Y in local coordinates
* Correlation times (nanosecond binning) histogram, range covers 2 * event length
* Correlation times (integer values) histogram

### Usage
```toml
[TestAlgorithm]
makeCorrelations = true
reference = "W0013_E03"
```
