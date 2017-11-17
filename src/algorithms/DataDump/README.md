## Algorithm: DataDump
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functional

#### Description
This algorithm reads in raw data files with extension `-1.dat`, and outputs the pixel data in hexidecimal in an output file called `outputHexDump.dat`.

#### Parameters
* `DeviceToDumpData`: The name of the device data is to be read from.
* `inputDirectory`: Path to the directory above the data directory named `DeviceToDumpData`.

#### Plots produced
No plots are produced.

#### Usage
```toml
[DataDump]
DeviceToDumpData = "W0005_H03"
inputDirectory = "path/to/directory"

```