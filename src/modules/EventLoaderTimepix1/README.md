# EventLoaderTimepix1
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional  

### Description
This module loads raw data from Timepix1 devices and adds it to the clipboard as `pixel` objects. The input file must have extension `.txt`, and these files are sorted into time order using the file titles.

### Parameters
* `input_directory`: Path of the directory above the data files.

### Usage
```toml
[Timepix1EventLoader]
input_directory = "path/to/directory"
```
