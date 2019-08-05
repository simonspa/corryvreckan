# TextWriter
**Maintainer**: Paul Schuetze (paul.schuetze@desy.de)
**Module Type**: *GLOBAL*  
**Status**: Testing

### Description
This module writes objects to an ASCII text file. For this, the data content of the selected objects available on the clipboard are printed to the specified file.

Events are separated by an event header

```
=== <event number> ===
```

and individual object types by the type marker:

```
--- <detector name> ---
```

With `include` and `exclude` certain object types can be selected to be printed.

### Parameters
* `file_name` : Name of the data file to create, relative to the output directory of the framework. The file extension `.txt` will be appended if not present.
* `include` : Array of object names to write to the ASCII text file, all other object names are ignored (cannot be used together simultaneously with the *exclude* parameter).
* `exclude`: Array of object names that are not written to the ASCII text file (cannot be used together simultaneously with the *include* parameter).

### Usage
```toml
[TextWriter]
file_name = "exampleFileName"
include = "Pixel"

```
