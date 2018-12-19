# FileReader
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)   
**Module Type**: *GLOBAL*  
**Status**: Functional   

### Description
This module reads in an input file containing trees with data previously written out by the `FileWriter`. The read in objects are stored on the clipboard. It reads in `pixel`, `cluster`, `track`, and/or `MCParticle` objects.

### Parameters
* `only_dut`: Boolean to decide if data is read in from only the DUT. Default value is `false`.
* `read_pixels`: Boolean to choose if pixel objects are to be read in. Default value is `true`.
* `read_clusters`: Boolean to choose if cluster objects are to be read in. Default value is `false`.
* `read_tracks`: Boolean to choose if track objects are to be read in. Default value is `false`.
* `read_mcparticles`: Boolean to choose if Monte-Carlo particle objects are to be read in. Default value is `false`.
* `file_name`: Name of the file from which data will be read.
* `time_window`: Data with time lower than this value will be read in. Default value is `1s`.

### Usage
```toml
[FileReader]
only_dut = true
read_pixels = true
read_clusters = true
read_tracks = false
read_mcparticles = true
file_name = "input_file.root"
time_window = 1
```
