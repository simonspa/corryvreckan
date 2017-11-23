## DUTAnalysis
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>) ; Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functional  

#### Description
This algorithm associates DUT clusters to telescope tracks.

Using the appropriate parameter settings in the configuration file, plots using power pulsing and/or Monte-Carlo truth information can be produced.

#### Parameters
* `DUT`: Name of the DUT plane.
* `useMCtruth`: Boolean to set if Monte-Carlo truth information is available and should be used. Default value is `false`.
*  `digitalPowerPusling`: Boolean to set if power pulsing was used and that this information should be used. Default value is `false`.

#### Plots produced
* Tracks vs time
* Associated tracks vs time
* Residual in X
* Residual in Y
* Residual in time
* Associated cluster ToT
* Associated cluster size
* Track correlations in X
* Track correlations in Y
* Track correlations in time
* Cluster ToT vs time
* Residual in time vs time
* 2D histogram of associated track global positions
* 2D histogram of unassociated track global positions
* Residual in X using MC truth information, only produced if `usingMCtruth = true`
* Tracks vs time since power on, only produced in `digitalPowerPusling = true`
* Associated tracks vs time since power on, only produced in `digitalPowerPusling = true`

#### Usage
```toml
[DUTAnalysis]
digitalPowerPusling = false
useMCtruth = true
DUT = "W0005_H03"
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
