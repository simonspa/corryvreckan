# EventLoaderCLICpix
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)   
**Module Type**: *DETECTOR*  
**Detector Type**: *CLICpix*  
**Status**: Functional   

### Description
This module reads in data for a CLICpix device from an input file.

The module opens and reads one data file in the specified input directory with the ending `.dat`. For each hit it stores the detectorID, row, column, and ToT. The shutter rise and fall time information are used to set the current time and event length.

### Parameters
* `input_directory`: Path to the directory containing the `.dat` file.

### Plots produced
* 2D Hit map
* 1D Pixel ToT histogram
* 1D Pixels per frame histogram
* 1D Shutter length histogram

### Usage
```toml
[CLICpixEventLoader]
input_directory = /user/data/directory/CLICpix/
```
