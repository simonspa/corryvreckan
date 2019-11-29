# EventLoaderMuPixTelescope
**Maintainer**: Lennart Huth (lennart.huth@desy.de)  
**Module Type**: *GLOBAL*  
**Status**: Work in progress

### Description
This module reads in and converts data taken with the MuPix telescope.
It requires one input file which contains the data of all planes in the telescope.
The data contains time-ordered readout frames.
For frame, the module loops over all hits and stores the row, column, timestamp, and ToT are stored on the clipboard.

It cannot be combined with other event loaders and does not require a `Metronome`.

### Parameters
* `input_directory`: Defines the input file. No default
* `Run`: not in use. Defaults to `-1`
* `is_sorted`: Defines if data recorded is on FPGA timestamp sorted. Defaults to `false`
* `ts2_is_gray`: Defines if the timestamp is gray encoded or not. Defaults to `false`.

### Plots produced

For all detectors, the following plots are produced:

* 2D histogram of pixel hit positions
* 1D histogram of the pixel timestamps

### Usage
```toml
[EventLoaderMuPixTelescope]
input_directory = "/path/to/file"
Run = -1
is_sorted = false
ts2_is_gray = false

```
