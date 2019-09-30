# EventLoaderEUDAQ2
**Maintainer**: Jens Kroeger (<jens.kroeger@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Status**: under development

### Description
This module allows data recorded by EUDAQ2 and stored in a EUDAQ2 binary file as raw detector data to be read into Corryvreckan.
For each detector type, the corresponding converter module in EUDAQ2 is used to transform the data into the `StandardPlane` event type before storing the individual `Pixel` objects on the Corryvreckan clipboard.

The detectors need to be named according to the following scheme: `<detector_type>_<plane_number>` where `detector_type` is the type specified in the detectors file and `<plane_number>` is an iterative number over the planes of the same type.

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

If no detector is capable of defining events, the `[Metronome]` model needs to be used.

Tags stores in the EUDAQ2 event header are read, a conversion to a double value is attempted and, if successful, a profile with the value over the number of events in the respective run is automatically allocated and filled. This feature can e.g. be used to log temperatures of the devices during data taking, simply storing the temperature as event tags.

### Requirements
This module requires an installation of [EUDAQ2](https://eudaq.github.io/). The installation path needs to be set to
```bash
export EUDAQ2PATH=/path/to/eudaq2
```
when running CMake to find the library to link against and headers to include.

It is recommended to only build the necessary libraries of EUDAQ2 to avoid linking against unnecessary third-party libraries such as Qt5.
This can be achieved e.g. by using the following CMake configuration for EUDAQ2:

```bash
$ cmake -DEUDAQ_BUILD_EXECUTABLE=OFF -DEUDAQ_BUILD_GUI=OFF ..
```

It should be noted that individual EUDAQ2 modules should be enabled for the build, depending on the data to be processed with Corryvreckan.
To e.g. allow decoding of Caribou data, the respective EUDAQ2 module has to be built using

```bash
$ cmake -DUSER_CARIBOU_BUILD=ON ..
```

### Contract between EUDAQ Decoder and EventLoader

The decoder promises to
* return `true` only when there is a fully decoded event available and `false` in all other cases.
* not return any event before a possible T0 signal in the data.
* return the smallest possible granularity of data in time either as even or as sub-events within one event.
* always return valid event time stamps. If the device does not have timestamps, it should return zero for the beginning of the event and have a valid trigger number set.
* provide the detector type via the `GetDetectorType()` function in the decoded StandardEvent.

### Configuring EUDAQ2 Event Converters

Some data formats depend on external configuration parameters for interpretation.
The EventLoaderEUDAQ2 takes all key-value pairs available in the configuration and forwards them to the appropriate StandardEvent converter on the EUDAQ side.
It should be kept in mind that the resulting configuration strings are parsed by EUDAQ2, not by Corryvreckan, and that therefore the functionality is reduced.
For example, it does not interpret `true` or `false` alphabetic value of a Boolean variable but will return false in both cases. Thus. `key = 0` or `key = 1` have to be used in these cases.
Also, more complex constructs such as arrays or matrices read by the Corryvreckan configuration are simply interpreted as strings.

### Parameters
* `file_name`: File name of the EUDAQ2 raw data file. This parameter is mandatory.
* `inclusive`: Boolean parameter to select whether new data should be compared to the existing Corryvreckan event in inclusive or exclusive mode. The inclusive interpretation will allow new data to be added to the event as soon as there is some overlap between the data time frame and the existing event, i.e. as soon as the end of the time frame is later than the event start or as soon as the time frame start is before the event end. In the exclusive mode, the frame will only be added to the existing event,if its start and end are both within the defined event.
* `skip_time`: Time that can be skipped at the start of a run. Default is `0ms`.
* `get_time_residuals`: Boolean to change if time residual plots should be created. Default value is `false`.
* `get_tag_vectors`: Boolean to enable creation of EUDAQ2 event tag histograms. Default value is `false`.
* `ignore_bore`: Boolean to completely ignore the Begin-of-Run event from EUDAQ2. Default value is `true`.
* `adjust_event_times`: Matrix that allows the user to shift the event start/end of all different types of EUDAQ events before comparison to any other Corryvreckan data. The first entry of each row specifies the data type, the second is the offset which is added to the event start and the third entry is the offset added to the event end. A usage example is shown below, double brackets are required if only one entry is provided.
* `buffer_depth`: Depth of buffer in which EUDAQ2 `StandardEvents` are timesorted. This algorithm only works for `StandardEvents` with well-defined timestamps. Setting it to `0` disables timesorting. Default is `0`.
* `shift_triggers`: Shift the trigger ID up or down when assigning it to the Corryvreckan event. This allows to correct trigger ID offsets between different devices such as the TLU and MIMOSA26.

### Plots produced
* 2D hitmap
* 1D pixel hit times (3 second range)
* 1D pixel hit times (3000 second range)
* 1D pixel raw value histogram (corresponds to chip-specific charge equivalent measurement, e.g. ToT)
* 1D pixel multiplicity per Corryvreckan event histogram
* 1D eudaq event start histogram (3 second range)
* 1D eudaq event start histogram (3000 second range)
* 1D clipboard event start histogram (3 second range)
* 1D clipboard event start histogram (3000 second range)
* 1D clipboard event end histogram
* 1D clipboard event duration histogram
* 1D pixel time minus event begin residual histogram
* 1D pixel time minus event begin residual histogram (larger interval, coarser binning)
* 2D pixel time minus event begin residual over time histogram
* map of all available 1D pixel time minus trigger time residual histograms
* 2D pixel time minus trigger time residual over time histogram for 0th trigger
* 1D triggers per event histogram

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
