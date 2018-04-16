## CLICpix2EventLoader
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>), Morag Williams (<morag.williams@cern.ch>)  
**Status**: Functional

#### Description
This module reads in data for a CLICpix2 device from an input file. It defines the reconstruction time structure as being the CLICpix2 frame by setting the `currentTime` to the begin of the frame and the `eventLength` to the length of the readout frame. These times are stored on the persistent clipboard storage and can be picked up by other modules to allow synchronization.

The module opens and reads one data file in the specified input directory.
The input directory is searched for a data file with the file extension `.raw` and a pixel matrix configuration file required for decoding with the file extension `.cfg` and a name starting with `matrix`.
The data is decoded using the CLICpix2 data decoder shipped with the Peary DAQ framework. For each pixel hit, the detectorID, the pixel's column and row address as well as ToT and ToA values are stored, depending on their availability from data. The shutter rise and fall time information are used to set the current time and event length as described above.

#### Dependencies

This module requires an installation of [Peary](https://gitlab.cern.ch/Caribou/peary) with its CLICPix2 component built. This is used for on-the-fly decoding of raw data.

#### Parameters
* `inputDirectory`: Path to the directory containing the `.csv` file. This path should lead to the directory above the CLICpix directory, as this string is added onto the input directory in the module.
* `DUT`: Name of the DUT plane.

#### Plots produced
* 2D Hit map
* 1D Pixel ToT histogram (if value is available)
* 1D Pixel ToA histogram (if value is available)
* 1D Pixel particle count histogram (if value is available)
* 2D map of profiles for ToT values
* 1D Pixels per frame histogram

#### Usage
```toml
[CLICpix2EventLoader]
DUT = "W0005_H03"
inputDirectory = /user/data/directory
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.