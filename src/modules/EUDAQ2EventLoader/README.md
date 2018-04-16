## NAME
**Maintainer**: NAME (<EMAIL>)   
**Status**: Outdated, Functional, or Work in progress   

#### Description
DESCRIPTION

#### Parameters
* `PARAMETERNAME`: EXPLANATION. Default value is `DEFAULT`.

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
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
