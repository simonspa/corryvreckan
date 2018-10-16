# Timepix3MaskCreator
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Status**: Functional

### Description
This module reads in `pixel` objects for each device from the clipboard, masks all pixels with a hit rate larger than 10 times the mean hit rate, and updates the trimdac file for the device with these masked pixels.

### Parameters
No parameters are used from the configuration file.

### Usage
```toml
[Timepix3MaskCreator]
```
