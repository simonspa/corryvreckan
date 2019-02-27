# EventLoaderEUDAQ2
**Maintainer**: Jens Kroeger (kroeger@physi.uni-heidelberg.de)  
**Module Type**: *DETECTOR*     
**Detector Types**: *CLICpix2*, *TLU*, *MIMOSA26*, (near future: *Timepix3*, *ATLASpix*)   
**Status**: Immature

### Description
This module allows data recorded by EUDAQ2 and stored in the EUDAQ2-native raw format (Is that correct?) to be read into Corryvreckan.
For each detector type, the corresponding converter modules in EUDAQ2 are used to transform the data into the \texttt{StandardPlane} event type before storing the individual \texttt{Pixel} objects on the Corryvreckan clipboard.
TLU event are not converted into \texttt{StandardEvents} but their timestamps can be used directly without conversion.

The detectors need to be named according to the following scheme: \texttt<detector_type>_<plane> where \texttt{detector_type} is either of the types above and \texttt{plane} is an iterative number of the plane.

If the data of different detectors is stored in separate files, the parameters \texttt{name} or \texttt{name} can be used as shown in the usage example below.

### Requirements
This module requires an installation of [EUDAQ2](https://eudaq.github.io/). The installation path needs to be set to
```bash
export EUDAQ2PATH=/path/to/eudaq2
```
when running CMake to find the library to link against and headers to include.

### Parameters
* `file_name`: File name of the EUDAQ2 raw data file. This parameter is mandatory.

### Plots produced
For each detector the following plots are produced:
* 2D hitmap
* Hit timestamps
* Pixels per frame
* event begin times
* TLU trigger time difference to frame begin (filled only if TLU events exist)

### Usage
```toml
[EventLoaderEUDAQ2]
name = "CLICpix2_0"
file_name = /path/to/data/examplerun_clicpix2.raw

[EventLoaderEUDAQ2]
type = "MIMOSA26"
file_name = /path/to/data/examplerun_telescope.raw
```
