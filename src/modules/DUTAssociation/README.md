# DUTAssociation
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
Module to establish an association between clusters on a DUT plane and a reference track.
The association allows for cuts in position and time.

### Parameters
* `spatialCut`: Maximum spatial distance in x- and y-direction allowed between cluster and track for association with the DUT. Expects two values for the two coordinates, defaults to twice the pixel pitch.
* `timingCut`: Maximum time difference allowed between cluster and track for association for the DUT. Default value is `200ns`.

### Plots produced
No histograms are produced.

### Usage
```toml
[DUTAssociation]

```
