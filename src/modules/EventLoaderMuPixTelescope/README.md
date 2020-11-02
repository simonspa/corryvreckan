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
The correct type is given by the `type` in the geometry.


Eventdefinition for the `EventLoaderMuPixTelescope`:
* Sorted telescope data: The loader can define its own events as the full
information of all systems is stored in an telescope event.
* Unsorted telescope data: External event definition is required. This has to
be a `Metronome` (8192ns are recommended
to match the internal time structure) for standalone telescope data. Otherwise
any other event definition (`EventLoaderEUDAQ2` `EventDefinitionM26`) can be used.

### Parameters
* `input_directory`: Defines the input file. No default
* `Run`: 6 digit Run number, with leading zeros beeing automatically added, to open the data file with the standart format `telescope_run_RUN.blck`
* `input_file`: Overwrite  the default file created based on the `Run`. No default.
* `is_sorted`: Defines if data recorded is on FPGA timestamp sorted. Defaults to `false`
* `ts2_is_gray`: Defines if the timestamp is gray encoded or not. Defaults to
`false`, currently not supported.
* `time_offset`: Set an offset to correct for the expected delay between
issuing the synchronoues reset and receiving it. Only used if
`is_sorted==false`. Defaults to `0`
* `input_file`: Overwrite the input filename if the filename has a non-default
structure. Inactive if not given.

### Plots produced

For all detectors, the following plots are produced:

* 2D histogram of pixel hit positions
* 1D histogram of the pixel timestamps
* 1D histogram of the pixel ToT - currently not filled

### Usage
```toml
[EventLoaderMuPixTelescope]
input_directory = "/path/to/file"
Run = 1234
is_sorted = false 
ts2_is_gray = false

```
