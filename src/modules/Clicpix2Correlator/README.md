# CLICpix2Correlator
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)

**Status**: Functional

### Description
This module makes plots of the correlations in X and Y for angles between 0 and 2pi in steps of 0.6 radians.

### Parameters
* `DUT`: Name of the DUT plane.

### Plots produced
For each angle between 0 and 2pi in steps of 0.6 radians the following plots are produced:
* Correlation (track difference) in X
* Correlation (track difference) in Y

### Usage
```toml
[CLICpix2Correlator]
DUT = "W0005_H03"
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
