# MaskCreator
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Work in progress

### Description
This module reads in `pixel` objects for each device from the clipboard, and masks pixels considered noisy.

Currently, two methods are available. The `localdensity` noise estimation method is taken from the [Proteus framework](https://gitlab.cern.ch/unige-fei4tel/proteus) developed by Université de Genève.
It uses a local estimate of the expected hit rate to find pixels that are a certain number of standard deviations away from this estimate.
The second method, `frequency`, is a simple cut on a global pixel firing frequency which masks pixels with a hit rate larger than `frequency_cut` times the mean global hit rate.

The module writes new mask file with all masked pixels for each device. Already existing masks are maintained. No masks are applied as this is done by other modules directly when reading input data.

### Parameters
* `method`: Select the method to evaluate noisy pixels. Can be either `localdensity` or `frequency`, where the latter is chosen by default.
* `frequency_cut`: Frequency threshold to declare a pixel as noisy, defaults to 50. This means, if a pixel exhibits 50 times more hits than the average pixel on the sensor, it is considered noisy and is masked. Only used in `frequency` mode.
* `binsOccupancy`: Number of bins for occupancy distribution histograms, defaults to 128.
* `density_bandwidth`: Bandwidth for local density estimator, defaults to `2` and is only used in `localdensity` mode.
* `sigma_above_avg_max`: Cut for noisy pixels, sigma above average, defaults to `5`. Only used in `localdensity` mode.
* `rate_max`: Maximum rate, defaults to `1`. Only used in `localdensity` mode.

### Plots produced
For each detector the following plots are produced:
* Map of masked pixels

### Usage
```toml
[MaskCreator]
frequency_cut = 10
```
