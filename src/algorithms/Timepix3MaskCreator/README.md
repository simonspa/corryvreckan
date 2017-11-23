## Algorithm: Timepix3MaskCreator
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functional

#### Description
This algorithm reads in `pixel` objects for each device from the clipboard, masks all pixels with a hit rate larger than 10 times the mean hit rate, and updates the trimdac file for the device with these masked pixels.

#### Parameters
No parameters are used from the configuration file.

#### Plots produced
No plots are produced.

#### Usage
```toml
[Timepix3MaskCreator]
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
