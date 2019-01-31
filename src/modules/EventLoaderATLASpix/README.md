# EventLoaderATLASpix
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *ATLASpix*  
**Status**: Functional

### Description
This module reads in data for the ATLASpix device from an input file created by the Caribou readout system. It supports binary output format.

The module opens and reads one data file named `data.bin` in the specified input directory. For each hit it, stores the detectorID, row, column, and ToT.

This module requires either another event loader of another detector type before which defines the event start and end times (variables `eventStart` and `eventEnd` on the clipboard) or an instance of the Metronome module which provides this information.

### Parameters
* `input_directory`: Path to the directory containing the `data.bin` file. This path should lead to the directory above the ALTASpix directory, as this string is added to the input directory in the module.
* `clock_cycle`: Period of the clock used to count the trigger timestamps in, defaults to `6.25ns`.
* `legacy_format`: Set `true` if using legacy data format of the Karlsruhe readout system. Default is `false`, corresponding the the format of the Caribou readout system.
* `clkdivend2`: Value of clkdivend2 register in ATLASPix specifying the speed of TS2 counter. Default is `0`.
* `calibration_file` (optional): input file for pixel-wise calibration from ToT to charge in electrons. If not provided, the pixel charge is equivalent to pixel ToT.

### Plots produced
* 2D Hit map
* 1D Pixel ToT histogram
* 1D Pixels per frame histogram

### Usage
```toml
[ATLASpixEventLoader]
input_directory = /user/data/directory
```
