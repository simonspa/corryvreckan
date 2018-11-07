# EventLoaderATLASpix
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *ATLASpix*  
**Status**: Functional

### Description
This module reads in data for the ATLASpix device from an input file created by the CaRIBou readout system. It supports binary output format.

The module opens and reads one data file named `data.bin` in the specified input directory. For each hit it, stores the detectorID, row, column, and ToT.

This module requires either another event loader of another detector type before which defines the event start and end times (variables `eventStart` and `eventEnd` on the clipboard) or an instance of the Metronome module which provides this information.

### Parameters
* `inputDirectory`: Path to the directory containing the `data.bin` file. This path should lead to the directory above the ALTASpix directory, as this string is added to the input directory in the module.
* `clockCycle`: Period of the clock used to count the trigger timestamps in, defaults to `6.25ns`.
* `DUT`: Name of the DUT plane.
* `clkdivend2`: Value of clkdivend2 register in ATLASPix specifying the speed of TS2 counter. Default is `0`.

### Plots produced
* 2D Hit map
* 1D Pixel ToT histogram
* 1D Pixels per frame histogram

### Usage
```toml
[ATLASpixEventLoader]
DUT = "W0005_H03"
inputDirectory = /user/data/directory
```
