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
* ATLASPix3
* all Run2020 sensors
* (additional/ older sensors can be added on request)

The detector name in the geometry file is used to determine the tag which identifies the sensor in the datastream:

[mydetetcor_TAG]

Everything behind the first `_` is used as tag, if none found the tag is assumed to be the detector name.
The correct type is given by the `type` in the geometry.
It is assumed that the timestamp has 10bit and runs at a frequency of 125 MHz (8ns bins)

### Event definition:

* Sorted telescope data: The loader can define its own events as the full
information of all systems is stored in an telescope event.
* Unsorted telescope data: External event definition is required. This has to
be a `Metronome` (8192ns are recommended
to match the internal time structure) for standalone telescope data. Otherwise
any other event definition (`EventLoaderEUDAQ2` `EventDefinitionM26`) can be used.

### Dependencies
This module requires a installation of the mupix8_daq package that is used by the Mu3e pixel group to read out the sensors. This library is non-public. Authorized users can download it via https://bitbucket.org/mu3e/mupix8_daq.git

### Parameters
* `input_directory`: Defines the input file. No default.
* `run`: 6 digit run number, with leading zeros being automatically added, to
open the data file with the standard format `telescope_run_RUN.blck`.`run` and
`input_file` are mutually exclusive.
* `is_sorted`: Defines if data recorded is on FPGA timestamp sorted. Defaults to `false`.
* `time_offset`: Subtract an offset to correct for the expected delay between
issuing the synchronous reset and receiving it. Only used if
`is_sorted=false`. Defaults to `0`.
* `input_file`: Filename of the input file. `run` and `input_file` are mutually exclusive.
* `buffer_depth`: Depth of the pixel buffer that is used to sort the hits with
respect to time. Defaults to `1000`.
* `reference_frequency`: Defines the reference frequency of the FPGA in MHz - defaults to `125`.
* `use_both_timestamps`: Decide if the the timestamps sampled on positive and
negative edge are used. This doubles the effective timestamp
frequency. Defaults to `false`.
* `nbits_timestamp`: Number of bits available for the timestamp. Defaults to `10`.
* `nbits_tot`: Number of bits available for the tot. Defaults to `6`.
* `ckdivend`: Clock divider for the timestamp clock. Defaults to `0`.
* `ckdivend2`: Clock divider for the ToT clock. Defaults to `7`.
### Plots produced

For all detectors, the following plots are produced:

* 2D histogram of pixel hit positions
* 2D histogram of pixel read in but not added to an event
* 1D histogram of the pixel timestamps
* 1D histogram of the pixel ToT
* 1D histogram of number of pixels per event
* 1D histogram with the number of pixels averaged over 1k events as function of event number

### Usage
```toml
[EventLoaderMuPixTelescope]
input_directory = "/path/to/file"
Run = 1234
is_sorted = false 
ts2_is_gray = false

```
