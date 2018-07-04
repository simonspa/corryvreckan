## ATLASpixEventLoader
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional

#### Description
This module reads in data for the ATLASpix device from an input file created by the CaRIBou readout system.

The module opens and reads one data file in the specified input directory with the ending `.dat`. For each hit it, stores the detectorID, row, column, and ToT.

This module requires either another event loader of another detector type before which defines the event start and end times (variables `eventStart` and `eventEnd` on the clipboard) or an instance of the Metronome module which provides this information.

#### Parameters
* `inputDirectory`: Path to the directory containing the `.dat` file. This path should lead to the directory above the ALTASpix directory, as this string is added to the input directory in the module.
* `clockCycle`: Period of the clock used to count the trigger timestamps in, defaults to `25ns`
* `DUT`: Name of the DUT plane.

#### Plots produced
* 2D Hit map
* 1D Pixel ToT histogram
* 1D Pixels per frame histogram

#### Usage
```toml
[ATLASpixEventLoader]
DUT = "W0005_H03"
inputDirectory = /user/data/directory
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
