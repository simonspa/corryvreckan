## Algorithm: CLICpixEventLoader
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)   
**Status**: Functional   

#### Description
This algorithm reads in data for a CLICpix device from an input file.

The algorithm opens and reads one data file in the specified input directory with the ending `.dat`. For each hit it stores the detectorID, row, column, and ToT. The shutter rise and fall time information are used to set the current time and event length.

#### Parameters
* `inputDirectory`: Path to the directory containing the `.dat` file. This path should lead to the directory above the CLICpix directory, as this string is added onto the input directory in the algorithm.
* `DUT`: Name of the DUT plane.

#### Plots produced
* 2D Hit map
* 1D Pixel ToT histogram
* 1D Pixels per frame histogram
* 1D Shutter length histogram

#### Usage
```toml
[CLICpixEventLoader]
DUT = "W0005_H03"
inputDirectory = /user/data/directory
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
