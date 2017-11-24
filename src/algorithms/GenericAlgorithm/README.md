## GenericAlgorithm
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)   
**Status**: Functional

#### Description
This algorithm takes data from a Timepix3 device from the clipboard and plots the pixel hit positions. This is to be a template to easy create other algorithms for Corryvreckan.

#### Parameters
No parameters are used from the configuration file.

#### Plots produced
* Histogram of event numbers

For each detector the following plots are produced:
* 2D histogram of pixel hit positions

#### Usage
```toml
[GenericAlgorithm]

```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
