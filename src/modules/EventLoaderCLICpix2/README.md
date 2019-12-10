# EventLoaderCLICpix2
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>), Morag Williams (<morag.williams@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *CLICpix2*  
**Status**: Functional

### Description
This module reads in data for a CLICpix2 device from an input file. This module always attempts to define the current event on the clipboard using the begin and end of the shutter read from data.
Thus, this module can only be used as the first event loader module in the reconstruction chain.

The module opens and reads one data file in the specified input directory.
The input directory is searched for a data file with the file extension `.raw` and a pixel matrix configuration file required for decoding with the file extension `.cfg` and a name starting with `matrix`.
The data is decoded using the CLICpix2 data decoder shipped with the Peary DAQ framework. For each pixel hit, the detectorID, the pixel's column and row address as well as ToT and ToA values are stored, depending on their availability from data. If no ToA information is available, the pixel timestamp is set to the center of the frame.
+The shutter rise and fall time information are used to set the current time and event length as described above.

### Parameters
* `input_directory`: Path to the directory containing the `.csv` file. This path should lead to the directory above the CLICpix directory, as this string is added onto the input directory in the module.
* `discard_zero_tot`: Discard all pixel hits with a ToT value of zero. Defaults to `false`.

### Plots produced

For each detector, the following plots are produced:

* 2D histograms:
    * Hit map
    * Maps of masked pixels
    * Maps of pixel ToT vs. ToA
* 1D histograms:
    * Histograms of the pixel ToT, ToA, and particle count (if values are available)
    * Histogram of the pixel multiplicity

### Usage
```toml
[EventLoaderCLICpix2]
input_directory = /user/data/directory
```
