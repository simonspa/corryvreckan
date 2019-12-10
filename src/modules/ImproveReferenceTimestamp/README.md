# ImproveReferenceTimestamp
**Maintainer**: Florian Pitters (<florian.pitters@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Work in progress

### Description
Replaces the existing reference timestamp (earliest hit on reference plane) by either the trigger timestamp (method 0) or the average track timestamp (method 1). For method 0 to work, a trigger timestamp has to be saved as SPIDR signal during data taking.

### Parameters
* `improvement_method`: Determines which method to use. Trigger timestamp is 0, average track timestamp is 1. Default value is `1`.
* `signal_source`: Determines which detector plane carries the trigger signals. Only relevant for method 0. Default value is `"W0013_G02"`.
* `trigger_latency`: Adds a latency to the trigger timestamp to shift time histograms. Default value is `0`.

### Usage
```toml
[ImproveReferenceTimestamp]
improvement_method = 0
signal_source = "W0013_G02"
trigger_latency = 20ns
```
