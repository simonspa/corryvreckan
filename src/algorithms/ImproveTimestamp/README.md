## NAME
**Maintainer**: Florian Pitters (<florian.pitters@cern.ch>)
**Status**: Work in progress

#### Description
Improves the existing track timestamp (earliest hit on reference plane) by either the trigger timestamp (method 0) or the average track timestamp (method 1).

#### Parameters
* `m_method`: Determines which method to use. Trigger timestamp is 0, average track timestamp is 1. Default value is `1`.

#### Plots produced


#### Usage
```toml
[ImproveTimestamp]
improvementMethod = 1
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
