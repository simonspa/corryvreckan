# EventLoaderTimepix3
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *Timepix3*  
**Status**: Functional

### Description
This module loads raw data from a Timepix3 device and adds it to the clipboard. The input file must have extension `.dat` and are sorted into time order via the data file serial numbers. This code also identifies `trimdac` files and applies this mask to the pixels.

The data can be split into events using an event length in time, or using a maximum number of hits on a detector plane. `SpidrSignal` and `pixel` objects are loaded to the clipboard for each detector.

The hit timestamps are derived from the 40 MHz TOA counter and the fast on-pixel oscillator, which is measuring the precise hit arrival phase within to the global 40 MHz clock.
In Timepix3, the phase of the 40 MHz clock can be shifted from one double column to the next by 22.5 degree by the clock generator in order to minimize the instant digital power supply due to the pixel matrix clock tree.
This mode is used in the CLICdp telescope, and thus, the column-to-column phase shift is taken into account when calculating the hit arrival times.
See also the Timepix3 chip manual version 1.9, section 3.2.1 and/or [@timepix3-talk], slides 25 and 48.

When running in time mode (`number_of_pixelhits` not set), this module requires either another event loader of another detector type before which defines the event start and end times (variables `eventStart` and `eventEnd` on the clipboard) or an instance of the Metronome module which provides this information.

### Parameters
* `input_directory`: Path to the directory above the data directory for each device. The device name is added to the path during the module.
* `number_of_pixelhits`: Maximum number of pixel hits on each detector per event. Default value is `2000`. This is only used if this parameter is present in the configuration file, otherwise the data is split into events using the event length information from the clipboard.
* `trigger_latency`:
* `calibration_path`: Path to the calibration directory. If this parameter is set, calibration will be applied to the DUT. Assumed folder structure is `[calibration_path]/[detector name]/cal_thr_[threshold]_ik_[ikrum dac]/[detector name]_cal_[tot/toa].txt`. The assumed file structure is `[col | row | val1 | val2 | etc.]`.
* `threshold`: String defining the `[threshold]` DAC value for loading the appropriate calibration file, See above.

### Plots produced
* Histogram with pixel ToT before and after calibration
* Map for each calibration parameter if calibration is used

### Usage
```toml
[Timepix3EventLoader]
input_directory = "path/to/directory"
calibration_path = "path/to/calibration"
threshold = 1148
number_of_pixelhits = 0
```

[@timepix-talk] X. Llopart, The Timepix3 chip, EP-ESE seminar, https://indico.cern.ch/event/267425,
