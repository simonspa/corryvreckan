## EUDAQEventLoader
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  

#### Description
This algorithm allows to read data recorded by EUDAQ and stored in the EUDAQ-native raw format. The EUDAQ decoder plugins are used to transform the data into the `StandardPlane` event type before storing the individual `Pixel` objects on the Corryvreckan clipboard.

The detector IDs are taken from the plane name and IDs, two possible naming options for Corryvreckan are available: When setting `long_detector_id = true`, the name of the sensor plane and the ID are used in the form `<name>_<ID>`, while only the ID is used otherwise as `plane<ID>`. Only detectors known to Corryvreckan are decoded and stored, data from other detectors are ignored.

#### Parameters
* `file_name`: File name of the EUDAQ raw data file. This parameter is mandatory.
* `long_detector_id`: Boolean switch to configure using the long or short detector ID in Corryvreckan, defaults to `true`.

#### Plots produced
No plots are produced.

#### Usage
```toml
[EUDAQEventLoader]
file_name = "rawdata/eudaq/run020808.raw"
long_detector_id = true
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
