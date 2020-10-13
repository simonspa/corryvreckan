# EventDefinitionM26
**Maintainer**: Lennart Huth (lennart.huth@desy.de), Jens Kroeger (jens.kroeger@cern.ch)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This global module allows to fully utilize the PIVOT pixel behaviour of the
EUDET type telescopes based on the NI MIMOSA26 readout. The event begin and
end are defined based on the  pivot pixel provided in the MIMOSA data
stream. Currently, the module assumes that the full two data frames are read
out, which is not the case in the standard converter.
However, the converter only returns all pixels after the pivot pixel of the
first frame and those before the pivot pixel of the second frame.

Event definition example:
If a triggerID has a TLU event from 425.000us to 425.025us (default
25 ns events) and the pivot pixel-row is 512 the event will be defined as:

Please note: In about 1 permille of the cases, zero triggers per event are
observed, which should in principle not be possible.
Presumably, the reason is a delay between an incoming trigger signal and the
moment when the pivot pixel is defined.
This causes that in about 1 permille of the cases, the "time before" and
"time after" a trigger spanning the event is not set correctly when the
pivot pixel is close to its roll-over.

```
begin = 125.012.5us - (512 * (115.2 / 576)) us
end = begin + 230us
```

### Parameters
* `detector_event_time`: Specify the detector type used to define the event timestamp.
* `file_timestamp`: Data file containing the `detector_event_time` data
* `file_m26`: Data file containing the  `MIMOSA26` data used to define the extend of the event.
* `time_shift`: Optional shift of the event begin/end point. Defaults to `0`
* `shift_triggers`: Shift the trigger ID of the `detector_event_time`. This allows to correct trigger ID offsets between different devices such as the TLU and MIMOSA26. Note that if using the module `EventLoaderEUDAQ2` the same value for `shift_triggers` needs to be passed in both cases. Defaults to `0`.
* `skip_time`: Time that can be skipped at the start of a run. All events with earlier timestamps are discarded. Default is `0ms`.
* `eudaq_loglevel`: Verbosity level of the EUDAQ logger instance of the converter module. Possible options are, in decreasing severity, `USER`, `ERROR`, `WARN`, `INFO`, `EXTRA` and `DEBUG`. The default level is `ERROR`. Please note that the EUDAQ verbosity can only be changed globally, i.e. when using instances of `EventLoaderEUDAQ2` below this module, the last occurrence will determine the (global) value of this parameter.


### Plots produced
* 1D histogram of time between trigger timestamps
* 1D histogram of time between two frames with MIMOSA hits

### Usage
```toml
[EventDefinitionM26]
detector_event_time = TLU
file_timestamp = tlu_data.raw
file_duration = mimosa_data.raw
time_shift = 0
shift_triggers = -1
```
