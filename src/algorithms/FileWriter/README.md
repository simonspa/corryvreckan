## Algorithm: FileWriter
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)   
**Status**: Functional   

#### Description
This algorithm writes an output file and fills it with trees containing the requested data objects. `Pixel`, `cluster`, and/or `track` objects can be written into the trees.

#### Parameters
* `DUT`: Name of the DUT plane.
* `onlyDUT`: Boolean to decide if only the DUT data is to be written into the outputfile, or if all planes are to be. Default value is `true`.
* `writePixels`: Boolean to choose if pixel objects are to be written out. Default value is `true`.
* `writeClusters`: Boolean to choose if cluster objects are to be written out. Default value is `false`.
* `writeTracks`: Boolean to choose if track objects are to be written out. Default value is `true`.
* `fileName`: Name of the output file. Default value is `outputTuples.root`.

#### Plots produced
No plots are produced.

#### Usage
```toml
[FileWriter]
DUT = "W000_H03"
onlyDUT = false
writePixels = true
writeClusters = true
writeTracks = true
fileName = "output.root"
```