# EventDefinerEUDAQ2
**Maintainer**: Lennart Huth (lennart.huth@desy.de)
**Module Type**: *GLOBAL*  
**Status**: Work in progress 

### Description
This global module allows to fully utilize the PIVOT pixel behaviour of the
EUDET type telescopes based on the NI MIMOSA26 readout. The event begin and
end are defined based on the  pivot pixel provided in the MIMOSA data
stream. Currently the module assumes that the full two data frames are read
out, which is not the case in the standard converter
### Parameters
- `detector_event_time`: Specify the detector used to define the event
timestamp
- `detector_event_duration`: Specify detector used to define the begin and end
of the event realtive to the timestamp provided by `detector_event_time`
- `file_timestamp`: Data file containing the `detector_event_time` data
- `file_duration`: Data file containing the  `detector_event_duration` data
- `time_shift`: Optional shift of the event begin/end point. Defaults to `0`
- `shift_triggers`: Shift the trigger ID of the
`detector_event_time`. Defaults to `0`


### Plots produced
* 1D histogram of time between trigger timestamps
* 1D histogram of time between two frames with MIMOSA hits

### Usage
```toml
[EventDefinerEUDAQ2]
detector_event_time = TLU
detector_event_duration = MIMOSA
file_timestamp = tlu_data.raw
file_duration = mimosa_data.raw
time_shift = 0
shift_triggers = 0
```
