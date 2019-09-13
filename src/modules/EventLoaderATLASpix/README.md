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
* `high_tot_cut`: "high ToT" histograms are filled if pixel ToT is larger than this cut. Default is `40`.
* `calibration_file` (optional): input file for pixel-wise calibration from ToT to charge in electrons. If not provided, the pixel charge is equivalent to pixel ToT.
* `buffer_depth`: Depth of buffer in which pixel hits are timesorted before being added to an event. If set to `1`, effectively no timesorting is done. Default is `1000`.

### Plots produced
* 2D hit map
* 2D hit map for high ToT events (ToT>high_tot_cut)
* 2D ToT-weighted hit map
* 1D pixel ToT histogram
* 1D pixel ToT histogram before timestamp overflow correction
* 1D pixels charge histogram (currently not calibrated -> equivalent to ToT)
* 1D pixel ToA histogram
* 1D pixel multiplicity histogram
* 1D pixels over time histogram (3 second range)
* 1D pixels over time histogram (3000 second range)
* 1D pixel TS1 histogram
* 1D pixel TS2 histogram
* 1D pixel TS1 bits histogram
* 1D pixel TS2 bits histogram
* 1D pixel TS1 histogram for high ToT events (ToT>high_tot_cut)
* 1D pixel TS2 histogram for high ToT events (ToT>high_tot_cut)
* 1D pixel TS1 bits histogram for high ToT events (ToT>high_tot_cut)
* 1D pixel TS2 bits histogram for high ToT events (ToT>high_tot_cut)
* 1D trigger per event histogram
* 1D pixel time minus event begin residual histogram
* 1D pixel time minus event begin residual histogram (larger interval, coarser binning)
* 2D pixel time minus event begin residual over time histogram
* map of all available 1D pixel time minus trigger time residual histograms
* map of all available 2D pixel time minus trigger time residual over time histograms

### Usage
```toml
[ATLASpixEventLoader]
input_directory = /user/data/directory
clock_cycle = 8ns
clkdivend2 = 7
buffer_depth = 100
```
