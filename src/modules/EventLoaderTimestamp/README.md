# EventLoaderTimestamp
**Maintainer**: Eric Buschmann (eric.buschmann@cern.ch)
**Module Type**: *DETECTOR*
**Status**: Immature

### Description
Stripped-down version of `EventLoaderTimepix3`. This module loads only the trigger information from the data and adds triggers to the event.
If no event is defined, a new event centered on the trigger timestamp is created. This module is intended to be used with devices connected to the TDC inputs on a SPIDR board
to synchronise the device via the recorded trigger numbers to the rest of the telescope. To compensate for external delays, the `time_offset` parameter of this module should be used
instead of the `time_offset` parameter of the device in the geometry file to make sure that the correct event is associated.

### Parameters
* `event_length`: Duration of the events. Defaults to `10us`.
* `time_offset`: Time offset to be added to the timestamps. Used to compensate time offsets for signals connected to the TDC inputs.
* `buffer_depth`: Depth of the buffer for sorting triggers. Defaults to `10`.


### Plots produced

No plots are produced.

### Usage
```toml
[EventLoaderTimestamp]
time_offset = -200ns

```
