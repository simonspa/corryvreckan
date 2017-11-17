## Algorithm: SpatialClustering
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)

**Status**: Functioning

#### Description
This algorithm clusters the input data of a Timepix1, ATLASpix, or CLICpix device. The clustering method only uses positional information (centre of gravity calculation using touching neighbours method, no timing information). These clusters are stored on the clipboard for each device.

#### Parameters
No parameters are used from the configuration file.

#### Plots produced
No plots are produced.

#### Usage
```toml
[SpatialClustering]

```
