# EUDAQEventLoader
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  

### Description
This module allows data recorded by EUDAQ and stored in the EUDAQ-native raw format to be read into Corryvreckan. The EUDAQ decoder plugins are used to transform the data into the `StandardPlane` event type before storing the individual `Pixel` objects on the Corryvreckan clipboard.

The detector IDs are taken from the plane name and IDs, two possible naming options for Corryvreckan are available: When setting `long_detector_id = true`, the name of the sensor plane and the ID are used in the form `<name>_<ID>`, while only the ID is used otherwise as `plane<ID>`. Only detectors known to Corryvreckan are decoded and stored, data from other detectors are ignored.

### Requirements
This module requires an installation of [EUDAQ 1.x](https://github.com/eudaq/eudaq). The installation path should be set as environment variable via
```bash
export EUDAQPATH=/path/to/eudaq
```
for CMake to find the library link against and headers to include.

### Parameters
* `file_name`: File name of the EUDAQ raw data file. This parameter is mandatory.
* `long_detector_id`: Boolean switch to configure using the long or short detector ID in Corryvreckan, defaults to `true`.

### Usage
```toml
[EUDAQEventLoader]
file_name = "rawdata/eudaq/run020808.raw"
long_detector_id = true
```
