## DataOutput
**Maintainer**: Morag Williams (<morag.williams@cern.ch>)   
**Status**: Functional  

#### Description
This algorithm writes out data from a Timepix3 DUT for timing analysis. The outputted ROOT TTree contains data in branches.

For each track assocaited DUT cluster the following information is written out:
* Event ID
* Size in X
* Size in Y
* Number of pixels in the cluster

For each pixel in an associated cluster the follwing information is written out:
* X position
* Y position
* ToT
* ToA

For each track with assocaited DUT clusters the following information is written out:
* Intercept with the DUT (3D position vector)

#### Parameters
* `DUT`: Name of the DUT plane.
* `fileName`:


Or No parameters are used from the configuration file.

#### Plots produced
* PLOT or No plots are produced.

For each detector the following plots are produced:
* PLOT

#### Usage
```toml
[NAME]
ALLPARAMETERS = VALUE
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
