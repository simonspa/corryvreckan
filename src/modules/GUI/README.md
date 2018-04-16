## GUI
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functional - needing small fix

#### Description
This algorithm is a GUI for looking at plots made during the `BasicTracking`, `DUTAnalysis`, and `TestAlgorithm` algorithms. It is not functional.

#### Parameters
* `updateNumber`: Determines how often the plots are updated on screen. Default number is `500`.
* `DUT`: Name of the DUT plane.

#### Plots produced
Plots produced on screen:
* Track chi^2 plot from `BasicTracking`
* Track angle in x from `BasicTracking`

Plots produced on screen for each device:
* Hitmap from `TestAlgorithm`
* Residual in X from `BasicTracking` for telescope planes, from `DUTAnalysis` for the DUT
* Cluster ToT histogram from `TestAlgorithm` - currently not filled

#### Usage
```toml
[GUI]
DUT = "W000_H03"
updateNumber = 400
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
