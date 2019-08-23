# FileReader
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional  
**Output**: *all objects in input file*

### Description
Converts all object data stored in the ROOT data file produced by the FileWriter module back into the clipboard (see the description of FileWriter for more information about the format). Reads all trees defined in the data file that contain Corryvreckan objects. Places all objects read from the tree onto the clipboard storage.

If the requested number of events for the run is less than the number of events the data file contains, all additional events in the file are skipped. If more events than available are requested, a warning is displayed and the other events of the run are skipped.

Currently it is not yet possible to exclude objects from being read. In case not all objects should be converted to messages, these objects need to be removed from the file before the simulation is started.

### Parameters
* `file_name` : Location of the ROOT file containing the trees with the object data.
* `include` : Array of object names (without `corryvreckan::` prefix) to be read from the ROOT trees, all other object names are ignored (cannot be used simulateneously with the *exclude* parameter).
* `exclude`: Array of object names (without `corryvreckan::` prefix) not to be read from the ROOT trees (cannot be used simultaneously with the *include* parameter).

### Usage
This module should be placed at the beginning of the main configuration. An example to read only Cluster and Pixel objects from the file *data.root* is:

```ini
[FileReader]
file_name = "data.root"
include = "Cluster", "Pixel"
```
