# Tracking4D
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This module performs a basic tracking method.

Clusters from the first plane in Z (named the seed plane) are related to clusters close in time on the other detector planes using straight line tracks. The DUT plane can be excluded from the track finding.

### Parameters
* `time_cut_rel`: Number of standard deviations the `time_resolution` of one detector plane will be multiplied by, either the `time_resolution` of the reference plane or the current telescope plane, whichever is largest. This calculated value is then used as the maximum time difference allowed between clusters for association to a track. This allows the time cuts between different planes to be detector appropriate. By default, a relative time cut is applied. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference allowed between clusters for association to the track. Absolute and relative time cuts are mutually exclusive. No default value.
* `spatial_cut`: Maximum spatial distance in the XY plane allowed between clusters for association for the telescope planes. Default value is `0.2mm`.
* `min_hits_on_track`: Minium number of associated clusters needed to create a track, equivalent to the minimum number of planes required for each track. Default value is `6`.
* `exclude_dut`: Boolean to choose if the DUT plane is included in the track finding. Default value is `true`.
* `require_detectors`: Names of detectors which are required to have a cluster on the track. If a track does not have a cluster from all detectors listed here, it is rejected. If empty, no detector is required. Default is empty.
* `timestamp_from`: Defines the detector which provides the track timestamp. This detector also needs to be set as `required_detector`. If empty, the average timestamp of all clusters on the track will be used. Empty by default.

### Plots produced
* Track chi^2 histogram
* Track chi^2 /degrees of freedom histogram
* Clusters per track histogram
* Tracks per event histogram
* Track angle with respect to X-axis histogram
* Track angle with respect to Y-axis histogram

For each detector the following plots are produced:

* Residual in X
* Residual in Y
* Residual in X for clusters with a width in X of 1
* Residual in Y for clusters with a width in X of 1
* Residual in X for clusters with a width in X of 2
* Residual in Y for clusters with a width in X of 2
* Residual in X for clusters with a width in X of 3
* Residual in Y for clusters with a width in X of 3

### Usage
```toml
[BasicTracking]
min_hits_on_track = 4
spatial_cut = 300um
time_cut_rel = 3.0
exclude_dut = true
require_detectors = "ExampleDetector_0", "ExampleDetector_1"
```
