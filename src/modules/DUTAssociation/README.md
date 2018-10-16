# DUTAssociation
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)
**Status**: Functional

### Description
Module to establish an association between clusters on a DUT plane and a reference track.
The association allows for cuts in position and time.

### Parameters
* `spatialCut`: Maximum spatial distance in the XY plane allowed between cluster and track for association with the DUT. Default value is `0.2mm`.
* `timingCut`: Maximum time difference allowed between cluster and track for association for the DUT. Default value is `200ns`.

### Plots produced
No histograms are produced.

### Usage
```toml
[DUTAssociation]

```
