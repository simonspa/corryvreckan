# EventLoaderEUDAQ2
**Maintainer**: Jens Kroeger (jens.kroeger@cern.ch)  
**Module Type**: *DETECTOR*  
**Status**: under development  

### Description
This module allows data recorded by EUDAQ2 and stored in a EUDAQ2 binary file as raw detector data to be read into Corryvreckan.
For each detector type, the corresponding converter module in EUDAQ2 is used to transform the data into the `StandardPlane` event type before storing the individual `Pixel` objects on the Corryvreckan clipboard.
TLU event are not converted into `StandardEvents` but their timestamps can be used directly without conversion.

The detectors need to be named according to the following scheme: `<detector_type>_<plane>` where `detector_type` is either of the types above and `<plane>` is an iterative number over the planes of the same type.

If the data of different detectors is stored in separate files, the parameters `name` or `type` can be used as shown in the usage example below.
It should be noted that the order of the detectors is crucial.
The first detector that appears in the configuration defines the event window to which the hits of all other detectors are compared.
In the example below this is the CLICpix2.

For each event, the algorithm checks for an event on the clipboard.
If none is available, the current event defines the event on the clipboard.
Otherwise, it is checked whether or not the current event lies within the clipboard even.
If yes, the corresponding pixels are added to the clipboard for this event.
If earlier, the next event is read until a matching event is found.
If later, the pointer to this event is kept and it continues with the next detector.

### Requirements
This module requires an installation of [EUDAQ2](https://eudaq.github.io/). The installation path needs to be set to
```bash
export EUDAQ2PATH=/path/to/eudaq2
```
when running CMake to find the library to link against and headers to include.

### Parameters
* `file_name`: File name of the EUDAQ2 raw data file. This parameter is mandatory.
* `time_before_tlu_timestamp`: Defines how much earlier than the TLU timestamp the event window should begin
* `time_after_tlu_timestamp`: Defines how much later than the TLU timestamp the event window should end.
* `search_time_before_tlu_timestamp`: Defines the begin of the window around the TLU timestamp that needs to lie within the current event window for the respective frame of the TLU triggered detector to be matched.
* `search_time_after_tlu_timestamp`: Defines the end of the window around the TLU timestamp that needs to lie within the current event window for the respective frame of the TLU triggered detector to be matched.

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
