## NAME
**Maintainer**: Florian Pitters (<florian.pitters@cern.ch>)
**Status**: Work in progress

#### Description
Replaces the existing reference timestamp (earliest hit on reference plane) by either the trigger timestamp (method 0) or the average track timestamp (method 1). For method 0 to work, a trigger timestamp has to be saved as SPIDR signal during data taking.

#### Parameters
* `improvementMethod`: Determines which method to use. Trigger timestamp is 0, average track timestamp is 1. Default value is `1`.
* `signalSource`: Determines which detector plane carries the trigger signals. Only relevant for method 0. Default value is `"W0013_G02"`.

#### Plots produced
No plots are produced.

#### Usage
```toml
[ImproveReferenceTimestamp]
improvementMethod = 0
signalSource = "W0013_G02"
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
