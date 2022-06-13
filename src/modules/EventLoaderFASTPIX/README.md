# EventLoaderFASTPIX
**Maintainer**: Eric Buschmann (eric.buschmann@cern.ch)
**Module Type**: *DETECTOR*
**Detector Type**: *FASTPIX*
**Status**: Functional

### Description
This module loads pre-decoded data from a FASTPIX device. It requires trigger numbers for synchronisation and data without a corresponding trigger number are discarded.
Timestamps are used as cross-check to identify incomplete events with missing triggers and to discard the event.

### Parameters
* `input_file`: Path to the `.dat` file containing the FASTPIX data.
* `time_scaler`: Scaling factor to compensate for drift in the recorded timestamps.


### Plots produced

For each detector the following plots are produced:
* 2D histogram of pixel hit positions
* Histograms of seed pixel ToT for inner and outer pixels
* Plot of timestamps
* Data decoder status

### Usage
```toml
[EventLoaderFASTPIX]
input_file = "path/to/directory/data.dat"

```
