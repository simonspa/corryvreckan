# EventLoaderTimepix3
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *Timepix3*  
**Status**: Functional

### Description
This module loads raw data from a Timepix3 [@timepix3] device and adds it to the clipboard. The input file must have extension `.dat` and are sorted into time order via the data file serial numbers. This code also identifies `trimdac` files and applies this mask to the pixels.

The data can be split into events using an event length in time, or using a maximum number of hits on a detector plane. `SpidrSignal` and `pixel` objects are loaded to the clipboard for each detector.

The hit timestamps are derived from the 40 MHz TOA counter and the fast on-pixel oscillator, which is measuring the precise hit arrival phase within to the global 40 MHz clock.
In Timepix3, the phase of the 40 MHz clock can be shifted from one double column to the next by 22.5 degree by the clock generator in order to minimize the instant digital power supply due to the pixel matrix clock tree.
This mode is used in the CLICdp telescope, and thus, the column-to-column phase shift is taken into account when calculating the hit arrival times.
See also the Timepix3 chip manual version 1.9, section 3.2.1 and/or [@timepix3-talk], slides 25 and 48.

This module requires either another event loader of another detector type before which defines the event start and end times (Event object on the clipboard) or an instance of the Metronome module which provides this information.
The frame-based readout mode of the Timepix3 is not supported.

The calibration is performed as described in [@Pitters_2019] [@cds-timepix3-calibration] and requires a Timepix3 plane to be set as `role = DUT`.

### Parameters
* `input_directory`: Path to the directory above the data directory for each device. The device name is added to the path during the module.
* `trigger_latency`:
* `calibration_path`: Path to the calibration directory. If this parameter is set, a calibration will be applied to the Timepix3 plane set as `role = DUT`. The assumed folder structure is `[calibration_path]/[detector name]/cal_thr_[threshold]_ik_[ikrum dac]/`. In this directory the two files `[detector name]_cal_[tot/toa].txt` have to exist.

For the ToT calibration, the file format needs to be `col | row | row | a (ADC/mV) | b (ADC) | c (ADC*mV) | t (mV) | chi2/ndf`.

For the ToA calibration, it needs to be `column | row | c (ns*mV) | t (mV) | d (ns) | chi2/ndf`.
* `threshold`: String defining the `[threshold]` DAC value for loading the appropriate calibration file, See above.

### Plots produced

For all detectors, the following plots are produced:

* 2D map of pixel positions
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

[@timepix3]: https://doi.org/10.1088/1748-0221/9/05/c05013  
[@timepix-talk]: X. Llopart, The Timepix3 chip, EP-ESE seminar, https://indico.cern.ch/event/267425  
[@Pitters_2019]: https://doi.org/10.1088%2F1748-0221%2F14%2F05%2Fp05022  
[@cds-timepix3-calibration]: https://cds.cern.ch/record/2649493  
