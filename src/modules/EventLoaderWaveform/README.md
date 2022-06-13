# EventLoaderWaveform
**Maintainer**: Eric Buschmann (eric.buschmann@cern.ch)
**Module Type**: *DETECTOR*
**Status**: Immature

### Description
Loads raw waveforms recorded by an oscilloscope and stores them in the event clipboard. 
Waveforms are matched to trigger numbers stored in the event.

### Parameters
* `input_directory`: Path to the directory containing the data files.
* `channels`: List of channels for which waveforms should be loaded.

### Plots produced
* Histogram of event numbers

For each detector the following plots are produced:

* 2D histogram of pixel hit positions

### Usage
```toml
[EventLoaderWaveform]
input_directory = "path/to/directory"
channels = "ch4"
```
