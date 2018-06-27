# DataOutput
**Maintainer**: Morag Williams (<morag.williams@cern.ch>)   
**Status**: Functional  

### Description
This module writes out data from a Timepix3 DUT for timing analysis. The outputted ROOT TTree contains data in branches. This is intended for analysis of the timing capabilities of Timepix3 devices of different thicknesses.

For each track associated DUT `cluster` object the following information is written out:
* Event ID
* Size in X
* Size in Y
* Number of pixels in the cluster

For each `pixel` object in an associated `cluster` the follwing information is written out:
* X position
* Y position
* ToT
* ToA

For each `track` with associated DUT `clusters` the following information is written out:
* Intercept with the DUT (3D position vector)

### Parameters
* `DUT`: Name of the DUT plane.
* `fileName`: Name of the outputted ROOT file. Default value is `outputTuples`.
* `treeName`: Name of the tree inside the outputted ROOT file. Default value is `tree`.

### Plots produced
No plots are produced.

### Usage
```toml
[DataOutput]
DUT = "W0005_H03"
fileName = "myOutputFile.root"
treeName = "myTree"
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
