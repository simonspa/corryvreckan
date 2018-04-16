## BasicTracking
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)   
**Status**: Functional   

#### Description
This module performs a basic tracking method.

Clusters from the first plane in Z (named the seed plane) are related to clusters close in time on the other detector planes using straight line tracks. The DUT plane can be excluded from the track finding and DUT clusters potentially associated with the track stored.

#### Parameters
* `timingCut`: Maximum time difference allowed between clusters for association. Default value is `200ns`.
* `spatialCut`: Maximum spatial distance in the XY plane allowed between clusters for association for the telescope planes. Default value is `0.2mm`.
* `spatialCutDUT`: Maximum spatial distance in the XY plane allowed between clusters for association for the DUT. Default value is `0.2mm`.
* `timingCutDUT`: Maximum time difference allowed between clusters for association for the DUT. Default value is `200ns`.
* `minHitsOnTrack`: Minium number of associated clusters needed to create a track, equivalent to the minimum number of planes required for each track. Default value is `6`.
* `excludeDUT`: Boolean to chose if the DUT plane is included in the track finding. In either case, the DUT clusters possibly associated with the track are saved. Default value is `true`.
* `DUT`: Name of the DUT plane.

#### Plots produced
* Track chi^2 histogram
* Track chi^2 /degrees of freedom histogram
* Clusters per track histogram
* Tracks per event histogram
* Track angle with respect to X-axis histogram
* Track angle with respect to Y-axis histogram

For each detector the following plots are produced:
* Residual in X
* Residual in Y
* Residual in X for clusters with a width in X of 1
* Residual in Y for clusters with a width in X of 1
* Residual in X for clusters with a width in X of 2
* Residual in Y for clusters with a width in X of 2
* Residual in X for clusters with a width in X of 3
* Residual in Y for clusters with a width in X of 3

#### Usage
```toml
[BasicTracking]
minHitsOnTrack = 4
spatialCut = 0.3 #300um
timingCut = 0.0000002 #200ns
excludeDUT = true
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
