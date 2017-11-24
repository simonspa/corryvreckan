## EtaCorrection
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)   
**Status**: Under development

#### Description
This algorithm takes data from Timepix3 devices from the clipboard and plots the pixel hit positions to create the eta correction. Eta correction has still to be implemented in this algorithm (under development).

#### Parameters
No parameters are used from the configuration file.

#### Plots produced
* Histogram of event numbers

For each detector the following plots are produced:
* 2D histogram of pixel hit positions

#### Usage
```toml
[EtaCorrection]

```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
