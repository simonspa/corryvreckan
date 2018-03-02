## MaskCreator
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  

#### Description
This algorithm reads in `pixel` objects for each device from the clipboard, masks all pixels with a hit rate larger than `frequency_cut` times the mean hit rate, and writes a new mask file for the device with these masked pixels. Already existing masks are maintained.

#### Parameters
* `frequency_cut`: Frequency threshold to declare a pixel as noisy, defaults to 50. This means, if a pixel exhibits 50 times more hits than the average pixel on the sensor, it is considered noisy and is masked.

#### Plots produced
For each detector the following plots are produced:
* Map of masked pixels

#### Usage
```toml
[MaskCreator]
frequency_cut = 10
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
