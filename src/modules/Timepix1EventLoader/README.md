# Timepix1EventLoader
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functional  

### Description
This module loads raw data from Timepix1 devices and adds it to the clipboard as `pixel` objects. The input file must have extension `.txt`, and these files are sorted into time order using the file titles.

### Parameters
* `inputDirectory`: Path of the directory above the data files.

### Usage
```toml
[Timepix1EventLoader]
inputDirectory = "path/to/directory"
```
