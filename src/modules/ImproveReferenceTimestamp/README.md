# ImproveReferenceTimestamp
**Maintainer**: Florian Pitters (<florian.pitters@cern.ch>), Jens Kroeger (<jens.kroeger@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Working

### Description
Replaces the existing track timestamp (set in the tracking module) by the closest trigger timestamp. For this, a trigger timestamp has to be saved as SPIDR signal during data taking.
If no suitable trigger is found for a track, the track is removed from the clipboard and thus not available any further.

### Parameters
* `signal_source`: Determines which detector plane carries the trigger signals. Only relevant for method 0. Default value is `"W0013_G02"`.
* `trigger_latency`: Adds a latency to the trigger timestamp to shift time histograms. Default value is `0`.
* `search_window`: Time window to search for SPIDR trigger signals around the track timestamp. Default value is `200ns`.

### Plots produced

The following plots are produced:

* number of triggers per event
* number of tracks per event
* time correlation between track and trigger timestamps

### Usage
```toml
[ImproveReferenceTimestamp]
signal_source = "W0013_G02"
trigger_latency = 20ns
```
