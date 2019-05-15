# EventLoaderEUDAQ2
**Maintainer**: Jens Kroeger (<jens.kroeger@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)
**Module Type**: *DETECTOR*
**Status**: under development

### Description
Correlate EUDAQ2 devices, based on time.

### Requirements
This module requires an installation of [EUDAQ2](https://eudaq.github.io/). The installation path needs to be set to
```bash
export EUDAQ2PATH=/path/to/eudaq2
```
when running CMake to find the library to link against and headers to include.

### Contract between EUDAQ Decoder and EventLoader

The decoder promises to
* return `true` only when there is a fully decoded event available and `false` in all other cases.
* not return any event before a possible T0 signal in the data.
* return the smallest possible granularity of data in time either as even or as sub-events within one event.
* always return valid event time stamps. If the device does not have timestamps, it should return zero for the beginning of the event and have a valid trigger number set.

### Parameters
* `file_name`: File name of the EUDAQ2 raw data file. This parameter is mandatory.
* `skip_time`: Time that can be skipped at the start of a run. Default is `0ms`.
* `adjust_event_times`: Matrix that allows the user to shift the event start/end of all different types of EUDAQ events. The first entry of each row specifies the data type, the second is the offset which is added to the event start and the third entry is the offset added to the event end. A usage example is shown below. Default is `0ms, 0ms`.

### Plots produced
*none*

### Usage
```toml
[EventLoaderEUDAQ2]
name = "CLICpix2_0"
file_name = /path/to/data/examplerun_clicpix2.raw

[EventLoaderEUDAQ2]
type = "MIMOSA26"
file_name = /path/to/data/examplerun_telescope.raw
adjust_event_times = ["NiRawDataEvent", -10ns, +10ns], ["TluRawDataEvent", -115us, +200us]
```
