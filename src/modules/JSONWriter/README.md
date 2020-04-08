# JSONWriter
**Maintainer**: Alexander Ferk (<alexander.ferk@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This module writes objects to a file as JSON array using `TBufferJSON::ToJASON(object)`. For this, the data content of the selected objects available on the clipboard are printed to the specified file. Beware that this results in a flat structure unlike the root file.

With `include` and `exclude` certain object types can be selected to be printed.

### Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension `.json` will be appended if not present.
* `include` : Array of object names to write to the ASCII text file, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names that are not written to the ASCII text file (cannot be used together simultaneously with the *include* parameter).

### Usage
```toml
[JSONWriter]
file_name = "exampleFileName"
include = "Cluster","Track"

```
