# FileWriter
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)   
**Module Type**: *GLOBAL*  
**Status**: Functional   

### Description
This module writes an output file and fills it with trees containing the requested data objects. `Pixel`, `cluster`, and/or `track` objects can be written into the trees.

### Parameters
* `only_dut`: Boolean to decide if only the DUT data is to be written into the outputfile, or if all planes are to be. Default value is `true`.
* `write_pixels`: Boolean to choose if pixel objects are to be written out. Default value is `true`.
* `write_clusters`: Boolean to choose if cluster objects are to be written out. Default value is `false`.
* `write_tracks`: Boolean to choose if track objects are to be written out. Default value is `true`.
* `file_name`: Name of the data file to create, relative to the output directory of the framework. The file extension `.root` will be appended if not present. Default value is `outputTuples.root`.

### Usage
```toml
[FileWriter]
only_dut = false
write_pixels = true
write_clusters = true
write_tracks = true
file_name = "output.root"
```
