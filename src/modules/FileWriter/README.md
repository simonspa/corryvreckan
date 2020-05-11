# FileWriter
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
Reads all objects from the clipboard into a vector of base class object pointers. The first time a new type of object is received, a new tree is created bearing the class name of this object. For every detector, a new branch is created within this tree. A leaf is automatically created for every member of the object. The vector of objects is then written to the file for every event it is dispatched, saving an empty vector if an event does not include the specific object.

In addition to the objects, the configuration is written to the ROOT file. The main configuration file is copied directly and all key/value pairs are written to a directory *config* in a subdirectory with the name of the corresponding module.

### Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension `.root` will be appended if not present.
* `include` : Array of object names (without `corryvreckan::` prefix) to write to the ROOT trees, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names (without `corryvreckan::` prefix) that are not written to the ROOT trees (cannot be used together simultaneously with the *include* parameter).

### Usage
To create the default file (with the name *data.root*) containing trees for all objects except for Cluster, the following configuration can be placed at the end of the main configuration:

```ini
[FileWriter]
file_name = "data.root"
exclude = "Cluster"
```
