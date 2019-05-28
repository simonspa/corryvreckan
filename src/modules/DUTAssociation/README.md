# DUTAssociation
**Maintainer**: Simon Spannagel (simon.spannagel@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
Module to establish an association between clusters on a DUT plane and a reference track.
The association allows for cuts in position and time.

### Parameters
* `spatial_cut`: Maximum spatial distance in x- and y-direction allowed between cluster and track for association with the DUT. Expects two values for the two coordinates, defaults to twice the pixel pitch.
* `timing_cut`: Maximum time difference allowed between cluster and track for association for the DUT. Default value is `200ns`.
* `use_cluster_centre`: If set true, the cluster centre will be compared to the track position for the spatial cut. If false, the nearest pixel will be used. Defaults to `true`.

### Plots produced
No histograms are produced.

### Usage
```toml
[DUTAssociation]

```
