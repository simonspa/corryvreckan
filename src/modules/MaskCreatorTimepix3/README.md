# MaskCreatorTimepix3
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *Timepix3*  
**Status**: Functional

### Description
This module reads in `pixel` objects for each device from the clipboard, masks all pixels with a hit rate larger than 10 times the mean hit rate, and updates the trimdac chip configuration file for the device with these masked pixels.
If this file does not exist yet, a new file named `<detectorID>_trimdac_masked.txt` will be created.

### Parameters
No parameters are used from the configuration file.

### Usage
```toml
[MaskCreatorTimepix3]
```
