# EventLoaderMuPixTelescope
**Maintainer**: Lennart Huth (lennart.huth@desy.de)
**Module Type**: *GLOBAL*  
**Status**: work in progress with some hard coded parts - needs polishing

### Description
This module reads in and converts data taken with the MuPix-Telescope.

### Parameters
* `input_directory`: Defines the input file. No default
* `Run`: not in use. Defaults to `-1`
* `is_sorted`: Defines if data recorded is on FPGA timestamp
sorted. Defaults to `false`
* `ts2_is_gray`: Defines if the timestamp is gray encoded or not. Defaults to `false`.

### Plots produced

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
