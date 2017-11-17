## Algorithm: Timepix3EventLoader
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)

**Status**: Outdated  

#### Description
This algorithm loads raw data from a Timepix3 device and adds it to the clipboard. The input file must have extension `.dat` and are sorted into time order via the data file serial numbers. This code also identifies `trimdac` files and applies this mask to the pixels.

The data can be split into events using an event length in time, or using a maximum number of hits on a detector plane. `SpidrSignal` and `pixel` objects are loaded to the clipboard for each detector. 

#### Parameters
* `inputDirectory`: Path to the directory above the data directory for each device. The device name is added to the path during the algorithm.
* `applyTimingCut`: Boolean to set if a timing cut should be applied. Default value is `false`. Not currently used.
* `timingCut`: Value of the timing cut. This will be applied if `applyTimingCut = true`. Default value is `0.0`. Not currently used.
* `minNumberOfPlanes`: Minimum number of planes with loaded data required for each event to be stored. Default value is `1`.
* `eventLength`: Length in time for each event. Default value is `0.0`. This is only used if `eventLength` is present in the configuration file, otherwise the data is split into events using the `number_of_pixelhits` parameter.
* `number_of_pixelhits`: Maximum number of pixel hits on each detector per event. Default value is `2000`. This is only used if `eventLength` is not present in the configuration file, otherwise the data is split into events using the `eventLength` parameter.
* `DUT`: Name of the DUT plane.

#### Plots produced
No plots are produced.

#### Usage
```toml
[Timepix3EventLoader]
inputDirectory = "path/to/directory"
applyTimingCut = false
timingCut = 0.0
minNumberOfPlanes = 5
eventLength = 0.0000002
number_of_pixelhits = 0
DUT = "W0005_H03"
```
