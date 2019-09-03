# EventLoaderCLICpix2
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>), Morag Williams (<morag.williams@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *CLICpix2*  
**Status**: Functional

### Description
This module reads in data for a CLICpix2 device from an input file. It defines the reconstruction time structure as being the CLICpix2 frame by setting the `eventStart` to the begin of the frame, the `eventEnd` to the end of the shutter and the `eventLength` to the length of the readout frame. These times are stored on the persistent clipboard storage and can be picked up by other modules to allow synchronization.
Thus, this module should not be used in conjunction with the Metronome but should be placed at the very beginning of the module chain.

The module opens and reads one data file in the specified input directory.
The input directory is searched for a data file with the file extension `.raw` and a pixel matrix configuration file required for decoding with the file extension `.cfg` and a name starting with `matrix`.
The data is decoded using the CLICpix2 data decoder shipped with the Peary DAQ framework. For each pixel hit, the detectorID, the pixel's column and row address as well as ToT and ToA values are stored, depending on their availability from data. The shutter rise and fall time information are used to set the current time and event length as described above.

### Parameters
* `input_directory`: Path to the directory containing the `.csv` file. This path should lead to the directory above the CLICpix directory, as this string is added onto the input directory in the module.
* `discard_zero_tot`: Discard all pixel hits with a ToT value of zero. Defaults to `false`.

### Plots produced
* 2D Hit map
* 2D maps of masked pixels, encoded with online masked (value 1) and offline masked (value 2), or in both (value 3)
* 1D Pixel ToT histogram (if value is available)
* 1D Pixel ToA histogram (if value is available)
* 1D Pixel particle count histogram (if value is available)
* 2D map of profiles for ToT values
* 1D Pixel mutliplicity per Corryvreckan event histogram

### Usage
```toml
[CLICpix2EventLoader]
input_directory = /user/data/directory
```
