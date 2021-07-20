# EventDefinitionM26
**Maintainer**: Lennart Huth (lennart.huth@desy.de), Jens Kroeger (jens.kroeger@cern.ch)
**Module Type**: *GLOBAL*
**Status**: Advanced

### Description
This global module allows to fully utilize the PIVOT pixel behaviour of the
EUDET type telescopes based on the NI MIMOSA26 readout. The MIMOSA DAQ stores two full rolling shutter frames.
The first frame corresponds to the frame where the trigger has been received. To store also particle hits at a position in front of the shutter, also the next frame is stored.
Note that the default NI-converter only returns pixels after the pivot in the first frame and in front of the pivot in the second frame. 

The event begin and
end are defined based on the  pivot pixel provided in the MIMOSA data
stream.

For a triggerID that has a TLU event from 425.000us to 425.025us (default
25 ns events), the trigger timestamp is defined as the middle of the event:
 t_trig = (425us+425.025us)/2
 and the pivot pixel-row is p the event will be defined as:

```math
begin = t_{trig} - (p * (115.2 / 576)) \mu s \\
end = begin + 230 \mu s

```
With an unknown probability the MIMOSA DAQ is not transmitting the correct two frames. Assuming frames t and t+1 are expected it can happen that frames t-1 and t are copied to disk.
This causes an artificial inefficiency if the above method is used and no additional timing layer is available. To overcome this issue, the option `pixelated_timing_layer` is added. If set to false, this enlarges the frames to cover the full possible timestamp range. This redduces the available statistics by a rate depenend fraction (Typically 30-40%).



It should be noted that in about 1 permille of the cases, zero triggers per event are
observed, which should in principle not be possible.
Presumably, the reason is a delay between an incoming trigger signal and the
moment when the pivot pixel is defined.
This causes that in about 1 permille of the cases, the "time before" and
"time after" a trigger spanning the event is not set correctly when the
pivot pixel is close to its roll-over.


### Parameters
* `detector_event_time`: Specify the detector type used to define the event timestamp.
* `file_timestamp`: Data file containing the `detector_event_time` data
* `file_duration`: Data file containing the  `MIMOSA26` data used to define the extend (duration) of the event.
* `time_shift`: Optional shift of the event begin/end point. Defaults to `0`
* `shift_triggers`: Shift the trigger ID of the `detector_event_time`. This allows to correct trigger ID offsets between different devices such as the TLU and MIMOSA26. Note that if using the module `EventLoaderEUDAQ2` the same value for `shift_triggers` needs to be passed in both cases. Defaults to `0`.
* `skip_time`: Time that can be skipped at the start of a run. All events with earlier timestamps are discarded. Default is `0ms`.
* `eudaq_loglevel`: Verbosity level of the EUDAQ logger instance of the converter module. Possible options are, in decreasing severity, `USER`, `ERROR`, `WARN`, `INFO`, `EXTRA` and `DEBUG`. The default level is `ERROR`. Please note that the EUDAQ verbosity can only be changed globally, i.e. when using instances of `EventLoaderEUDAQ2` below this module, the last occurrence will determine the (global) value of this parameter.
* `add_trigger`: Option to directly add the trigger of the TLU to the event. Note that no `EventLoaderEUDAQ2` for the TLU is required if this option is activated.  Defaults to `false`
* `pixelated_timing_layer`: Define if an additional timing layer is beeing used. If so, the event length is defined as described in the text above. Defaults to `true`.
* `use_all_mimosa_hits`: Select if all hits in the two MIMOSA frames are used for analysis, or if pivot based selection is used in the converters. If set to `true`, a full frame of `math 115 \mu s` is added at to the `begin` defined above, for `false` it sets `math begin= 115\mu s`.  If`pixelated_timing_layer=true` this has no effect.  Should always be identical to `EventLoaderEUDAQ2` and defaults to `false`.


The following parameters are for device debugging only and typically not set manually:
* `add_begin`: Shift the begin of the event further into the past. Defaults to `0`.
* `add_end`: Shift the end of the event further into the end. Defaults to `0`.
* `pivot_min`: Minimal pivot pixel to accept frames. Defaults to `0`.
* `pivot_max`: Maximal pivot pixel to accept frames. Defaults to `576`.


In addition, parameters can be forwarded to the EUDAQ2 event converters.
Please refer to the README of the `EventLoaderEUDAQ2` for more details.


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
shift_triggers = 1
add_trigger = false
```
