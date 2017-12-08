## FileReader
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)   
**Status**: Functional   

#### Description
This algorithm reads in an input file containing trees with data previously written out by the `FileWriter`. The read in objects are stored on the clipboard. It reads in `pixel`, `cluster`, `track`, and/or `MCparticle` objects.

#### Parameters
* `onlyDUT`: Boolean to decide if data is read in from only the DUT. Default value is `false`.
* `readPixels`: Boolean to choose if pixel objects are to be read in. Default value is `true`.
* `readClusters`: Boolean to choose if cluster objects are to be read in. Default value is `false`.
* `readTracks`: Boolean to choose if track objects are to be read in. Default value is `false`.
* `readMCParticles`: Boolean to choose if Monte-Carlo particle objects are to be read in. Default value is `false`.
* `fileName`: Name of the file from which data will be read. Default value is `ouyputTuples.root`.
* `timeWindow`: Data with time lower than this value will be read in. Default value is `1s`.
* `DUT`: Name of the DUT plane.

#### Plots produced
No plots are produced.

#### Usage
```toml
[FileReader]
onlyDUT = true
readPixels = true
readClusters = true
readTracks = false
readMCParticles = true
fileName = "input_file.root"
timeWindow = 1
DUT = "W0005_H03"
```
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
