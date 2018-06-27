# Timepix3EventLoader
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)
**Status**: Outdated

### Description
This module loads raw data from a Timepix3 device and adds it to the clipboard. The input file must have extension `.dat` and are sorted into time order via the data file serial numbers. This code also identifies `trimdac` files and applies this mask to the pixels.

The data can be split into events using an event length in time, or using a maximum number of hits on a detector plane. `SpidrSignal` and `pixel` objects are loaded to the clipboard for each detector.

The hit timestamps are derived from the 40 MHz TOA counter and the fast on-pixel oscillator, which is measuring the precise hit arrival phase within to the global 40 MHz clock.
In Timepix3, the phase of the 40 MHz clock can be shifted from one double column to the next by 22.5 degree by the clock generator in order to minimize the instant digital power supply due to the pixel matrix clock tree.
This mode is used in the CLICdp telescope, and thus, the column-to-column phase shift is taken into account when calculating the hit arrival times.
See also the Timepix3 chip manual version 1.9, section 3.2.1 and/or [@timepix3-talk], slides 25 and 48.

### Parameters
* `inputDirectory`: Path to the directory above the data directory for each device. The device name is added to the path during the module.
* `minNumberOfPlanes`: Minimum number of planes with loaded data required for each event to be stored. Default value is `1`.
* `eventLength`: Length in time for each event. Default value is `0.0`. Event length is only used if this parameter is present in the configuration file, otherwise the data is split into events using the `number_of_pixelhits` parameter.
* `number_of_pixelhits`: Maximum number of pixel hits on each detector per event. Default value is `2000`. This is only used if `eventLength` is not present in the configuration file, otherwise the data is split into events using the `eventLength` parameter.
* `calibrationPath`: Path to the calibration directory. If this parameter is set, calibration will be applied to the DUT. Assumed folder structure is `"[calibrationPath]/[detector name]/cal_thr_[thr dac]_ik_[ikrum dac]/[detector name]_cal_[tot/toa].txt"`. The assumed file structure is `[col | row | val1 | val2 | etc.]`.
* `DUT`: Name of the DUT plane.

### Plots produced
* Histogram with pixel ToT before and after calibration
* Map for each calibration parameter if calibration is used

### Usage
```toml
[Timepix3EventLoader]
inputDirectory = "path/to/directory"
calibrationPath = "path/to/calibration"
threshold = 1148
minNumberOfPlanes = 5
eventLength = 0.0000002
number_of_pixelhits = 0
DUT = "W0005_H03"
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.

[@timepix-talk] X. Llopart, The Timepix3 chip, EP-ESE seminar, https://indico.cern.ch/event/267425,
