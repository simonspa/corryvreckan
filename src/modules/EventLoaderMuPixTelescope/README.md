# EventLoaderMuPixTelescope
**Maintainer**: Lennart Huth (<lennart.huth@desy.de>)
**Module Type**: *DETECTOR*
**Status**: Work in progress

### Description
This module reads in and converts data taken with the MuPix telescope or a single mupix plane.
It requires one input file which contains the data of all planes used.
The `EventLoaderMuPixTelescope` supports a list of sensors:
* MuPix8
* MuPix9
* MuPix10
* all Run2020 sensor
* (additional/ older sensors can be added on request)

The detector name in the geometry file is used to determine the tag:

[mydetetcor_TAG]

Everything behin the first `_` is used as tag, if none found the tag is assumed to be the detector name.
The correct type is given by the `type` in the geometry

Currently a `Metronome` is required to open the events (8192ns are recommended to match the internal time structures). For sorted data this is rather inefficient.

### Parameters
* `input_directory`: Defines the input file. No default
* `Run`: 6 digit Run number, with leading zeros beeing automatically added, to open the data file with the standart format `telescope_run_RUN.blck`
* `input_file`: Overwrite  the default file created based on the `Run`. No default.
* `is_sorted`: Defines if data recorded is on FPGA timestamp sorted. Defaults to `false`
* `ts2_is_gray`: Defines if the timestamp is gray encoded or not. Defaults to `false`.

### Plots produced

For all detectors, the following plots are produced:

* 2D histogram of pixel hit positions
* 1D histogram of the pixel timestamps
* 1D histogram of the pixel ToT

### Usage
```toml
[EventLoaderMuPixTelescope]
input_directory = "/path/to/file"
Run = 1234
is_sorted = false 
ts2_is_gray = false

```
