# Metronome
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional  

### Description
The `Metronome` module is can be used to slice data without strict event structure in arbitrarily long time slots, which serve as events for subsequent modules. This is done by configuring an event length and by setting the variables `eventStart` and `eventStop` on the clipboard.

Subsequent modules should read these values and adhere to them.

### Parameters
* `eventLength`: Length of the event to be defined in physical units (not clock cycles of a specific device). Default value is `10us`.

### Usage
```toml
[Metronome]
eventLength = 500ns
```
