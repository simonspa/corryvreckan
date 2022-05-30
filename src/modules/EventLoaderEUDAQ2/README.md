# EventLoaderEUDAQ2
**Maintainer**: Jens Kroeger (<jens.kroeger@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Status**: Functional

### Description
This module allows data recorded by [EUDAQ2](https://github.com/eudaq/eudaq/) and stored in a EUDAQ2 binary file as raw detector data to be read into Corryvreckan.
For each detector type, the corresponding converter module implemented in EUDAQ2 is used to transform the data into the `StandardPlane` event type before storing the individual `Pixel` objects on the Corryvreckan clipboard.

The detectors need to be named according to the following scheme: `<detector_type>_<plane_number>` where `<detector_type>` is the type specified in the detectors file and `<plane_number>` is an iterative number over the planes of the same type.

If the data of different detectors is stored in separate files, the parameters `name` or `type` can be used as shown in the usage example below.
It should be noted that the order of the detectors is crucial.
The first detector that appears in the configuration defines the event window to which the hits of all other detectors are compared.
In the example below this is the CLICpix2.

If the data of multiple detectors is stored in the same file as sub-events, it must be ensured that the event defining the time frame is processed first.
This is achieved by instantiating two event loaders in the desired order and providing them with the same input data file.
The individual (sub-) events are compared against the detector type.

For each event, the algorithm checks for an event on the clipboard.
If none is available, the current event defines the event on the clipboard.
Otherwise, it is checked whether or not the current event lies within the clipboard event.
If yes, the corresponding pixels are added to the clipboard for this event.
If earlier, the next event is read until a matching event is found.
If later, the pointer to this event is kept and it continues with the next detector.

Data from detectors with both triggered readout and without timestamps are matched against trigger IDs stored in the currently defined Corryvreckan event.

If no timestamp is available for the individual pixels, but the EUDAQ2 event has non-zero event times (e.g. event begin = shutter open and event end = shutter close), then the pixel timestamp is set to the center of the EUDAQ2 event.
If no timestamp is available for the individual pixels and the EUDAQ2 event has zero timestamps (event begin = end = 0), then the event times are redefined to begin/end = trigger timestamp, i.e. in this case, the pixel timestamp will be set to the corresponding trigger timestamp.

Triggers from new data are added to the event if the data lies within the event range, unless the `veto_triggers` flag is set.

If no detector is capable of defining events, the `[Metronome]` module needs to be used.

If `get_tag_histograms` or `get_tag_profiles` is used, the tags stored in the EUDAQ2 event header are read, a conversion to a double value is attempted and, if successful, a histogram and/or profile with the value over the number of events in the respective run is automatically allocated and filled.
This feature can e.g. be used to log temperatures of the devices during data taking, simply storing the temperature as event tags.

### Requirements
This module requires an installation of [EUDAQ2](https://eudaq.github.io/). The installation path needs to be passed to CMake when building Corryvreckan via
```bash
cmake -Deudaq_DIR=/path/to/eudaq2_installation/cmake/
```
to find the correct libraries to link against and headers to include.

It is recommended to only build the necessary libraries of EUDAQ2 to avoid linking against unnecessary third-party libraries such as Qt5.
This can be achieved e.g. by using the following CMake configuration for EUDAQ2:

```bash
cmake -DEUDAQ_BUILD_EXECUTABLE=OFF -DEUDAQ_BUILD_GUI=OFF ..
```

It should be noted that individual EUDAQ2 modules should be enabled for the build, depending on the data to be processed with Corryvreckan.
For instance, to allow decoding of Caribou data, the respective EUDAQ2 module has to be built using

```bash
cmake -DUSER_CARIBOU_BUILD=ON ..
```
__Note:__
It is important to make sure that the same compiler version is used for the installation of Corryvreckan and all its dependencies such as EUDAQ2 (if enabled).
On `lxplus` this is achieved by running `source path/to/corryvreckan/etc/setup_lxplus.sh` before beginning the installation.

### Contract between EUDAQ Decoder and EventLoader

The decoder guarantees to

* return `true` only when there is a fully decoded event available and `false` in all other cases.
* not return any event before a possible T0 signal in the data. This is a signal that indicates the clock reset at the beginning of the run. It can be a particular data word or the observation of the pixel timestamp jumping back to zero, depending on data format of each the detector.
* return the smallest possible granularity of data in time either as even or as sub-events within one event.
* always return valid event timestamps. If the device does not have timestamps, it should return zero for the beginning of the event and have a valid trigger number set.
* provide the detector type via the `GetDetectorType()` function in the decoded StandardEvent.
* throw a `eudaq::DataInvalid()` exception in case invalid data is encountered which should end the processing of data.


### Configuring EUDAQ2 Event Converters

Some data formats depend on external configuration parameters for interpretation.
The EventLoaderEUDAQ2 takes all key-value pairs available in the configuration and forwards them to the appropriate StandardEvent converter on the EUDAQ side.
It should be kept in mind that the resulting configuration strings are parsed by EUDAQ2, not by Corryvreckan, and that therefore the functionality is reduced.
For example, it does not interpret `true` or `false` alphabetic value of a Boolean variable but will return false in both cases. Thus `key = 0` or `key = 1` have to be used in these cases.
Also, more complex constructs such as arrays or matrices read by the Corryvreckan configuration are simply interpreted as strings.

In addition, the calibration file of the detector specified in the geometry configuration is passed to the EUDAQ2 event decoder using the key `calibration_file` and its canonical path as value. Adding the same key to the module configuration will overwrite the file specified in the detector geometry.

### Parameters
* `file_name`: File name of the EUDAQ2 raw data file. This parameter is mandatory.
* `inclusive`: Boolean parameter to select whether new data should be compared to the existing Corryvreckan event in inclusive or exclusive mode. The inclusive interpretation will allow new data to be added to the event as soon as there is some overlap between the data time frame and the existing event, i.e. as soon as the end of the time frame is later than the event start or as soon as the time frame start is before the event end. In the exclusive mode, the frame will only be added to the existing event if its start and end are both within the defined event.
* `skip_time`: Time that can be skipped at the start of a run. All hits with earlier timestamps are discarded. Default is `0ms`.
* `get_time_residuals`: Boolean to change if time residual plots should be created. Default value is `false`.
* `get_tag_histograms`: Boolean to enable creation of EUDAQ2 event tag histograms. Default value is `false`.
* `get_tag_profiles`: Boolean to enable creation of EUDAQ2 event tag profiles over event number. Default value is `false`.
* `ignore_bore`: Boolean to completely ignore the Begin-of-Run event (BORE) from EUDAQ2. Default value is `true`.
* `veto_triggers`: Flag to signal that no further triggers should be added to events that already have one or more triggers defined. Defaults to `false`.
* `adjust_event_times`: Matrix that allows the user to shift the event start/end of all different types of EUDAQ events before comparison to any other Corryvreckan data. The first entry of each row specifies the data type, the second is the offset which is added to the event start and the third entry is the offset added to the event end. A usage example is shown below, double brackets are required if only one entry is provided.
* `buffer_depth`: Depth of buffer in which EUDAQ2 `StandardEvents` are timesorted. This algorithm only works for `StandardEvents` with well-defined timestamps. Setting it to `0` disables timesorting. Default is `0`.
* `shift_triggers`: Shift trigger ID of this device with respect to the IDs stored in the Corryrveckan Event. This allows to correct trigger ID offsets between different devices such as the TLU and MIMOSA26. Note that if using the module `EventDefinitionM26` the same value for `shift_triggers` needs to be passed in both cases. Defaults to `0`.
* `eudaq_loglevel`: Verbosity level of the EUDAQ logger instance of the converter module. Possible options are, in decreasing severity, `USER`, `ERROR`, `WARN`, `INFO`, `EXTRA` and `DEBUG`. The default level is `ERROR`. Please note that the verbosity can only be changed globally, i.e. when using multiple instances of `EventLoaderEUDAQ2`, the last occurrence will determine the (global) value of this parameter.
* `sync_by_trigger`: Forces synchronization by trigger number, even if the events come with a time frame.

### Plots produced

For all detectors, the following plots are produced:

* 2D histograms:
    * Hit map
    * Map of the pixel time minus event begin residual over time
    * Map of the difference of the event start and the pixel timestamp over time
    * Map of the difference of the trigger time and the pixel timestamp over time
    * Profiles of the event tag data
* 1D histograms:
    * Histograms of the pixel hit times, raw values, multiplicities, and pixels per event
    * Histograms of the eudaq/clipboard event start/end, and durations
    * Histogram of the pixel time minus event begin residual

### Usage
```toml
[EventLoaderEUDAQ2]
name = "CLICpix2_0"
file_name = /path/to/data/examplerun_clicpix2.raw

[EventLoaderEUDAQ2]
type = "MIMOSA26"
file_name = /path/to/data/examplerun_telescope.raw
adjust_event_times = [["TluRawDataEvent", -115us, +230us]]
buffer_depth = 1000
```
