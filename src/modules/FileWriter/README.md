# FileWriter
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
Creates an output file with the specified clipboard data depending on the format:
* `root`: Reads all objects from the clipboard into a vector of base class object pointers. The first time a new type of object is received, a new tree is created bearing the class name of this object. For every detector, a new branch is created within this tree. A leaf is automatically created for every member of the object. The vector of objects is then written to the file for every event it is dispatched, saving an empty vector if an event does not include the specific object.
* `txt`: The output is created using  `corryvreckan::object::print()` and is equivalent to the output of  `TextWriter`.
* `json`: The output file contains a JSON-Array of all objects using `TBufferJSON::ToJASON(object)`. Beware that this results in a flat structure unlike the root file.

### Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension will be appended if not present.
* `format`: Format of the data file to create. Currently supported are `root`, `txt` and `json`. Defaults to `root`.
* `include` : Array of object names (without `corryvreckan::` prefix) to write to the file, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names (without `corryvreckan::` prefix) that are not written to the file  (cannot be used together simultaneously with the *include* parameter).

### Usage
To create the default file (with the name *data.root*) containing trees for all objects except for Cluster, the following configuration can be placed at the end of the main configuration:

```ini
[FileWriter]
file_name = "data.root"
exclude = "Cluster"
format = "root"
```
