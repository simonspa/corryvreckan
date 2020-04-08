# EventLoaderATLASpix
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *ATLASpix*  
**Status**: Functional

### Description
This module reads in data for the ATLASpix device from an input file created by the Caribou readout system. It supports the binary output format, i.e. `output  = 'binary'` in the configuration file for Caribou.

It requires either another event loader of another detector type before, which defines the event start and end times by placing an Event definition on the clipboard or an instance of the `Metronome` module which provides this information.

The module opens and reads one data file named `data.bin` in the specified input directory and for each hit with a timestamp between beginning and end of the currently processed Corryvreckan event it stores the detectorID, row, column, timestamp, and ToT on the clipboard.
Since a calibration is not yet implemented, the pixel charge is set to the pixel ToT.

### Parameters
* `input_directory`: Path to the directory containing the `data.bin` file. This path should lead to the directory above the ALTASpix directory, as this string is added to the input directory in the module.
* `clock_cycle`: Period of the clock used to count the trigger timestamps in, defaults to `6.25ns`.
* `legacy_format`: Set `true` if using legacy data format of the Karlsruhe readout system. Default is `false`, corresponding the the format of the Caribou readout system.
* `clkdivend2`: Value of clkdivend2 register in ATLASPix specifying the speed of TS2 counter. Default is `0`.
* `high_tot_cut`: "high ToT" histograms are filled if pixel ToT is larger than this cut. Default is `40`.
* `buffer_depth`: Depth of buffer in which pixel hits are timesorted before being added to an event. If set to `1`, effectively no timesorting is done. Default is `1000`.
* `time_offset`: Time offset to be added to each pixel timestamp. Defaults to `0ns`.

### Plots produced

For the DUT, the following plots are produced:

* 2D histograms:
    * Regular and ToT-weighted hit maps
    * Map of the difference of the event start and the pixel timestamp over time
    * Map of the difference of the trigger time and the pixel timestamp over time

* 1D histograms:
    * Various histograms of the pixel information: ToT, charge, ToA, TS1, TS2, TS1-bits, TS2-bits
    * Histograms of the pixel multiplicity, triggers per event, and pixels over time
    * Various timing histograms

### Usage
```toml
[ATLASpixEventLoader]
input_directory = /user/data/directory
clock_cycle = 8ns
clkdivend2 = 7
buffer_depth = 100
```
