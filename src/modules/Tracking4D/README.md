# Tracking4D
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>), Paul Schütze (<paul.schuetze@desy.de>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This module performs a basic tracking method.

The track finding works as follows.
All combinations of clusters in the first and the last hit detector plane are connected to form a straight line.
Clusters in further detectors are consecutively added if they are within the spatial cuts (in local coordinates) and time cuts, updating the reference track at each stage.
The DUT plane can be excluded from the track finding.

### Parameters
* `time_cut_rel`: Factor by which the `time_resolution` of each detector plane will be multiplied, either the `time_resolution` of the first plane in Z or the current telescope plane, whichever is largest. This calculated value is then used as the maximum time difference allowed between clusters and a track for association to the track. This allows the time cuts between different planes to be detector appropriate. By default, a relative time cut is applied. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference allowed between clusters and a track for association to the track. Absolute and relative time cuts are mutually exclusive. No default value.
* `spatial_cut_rel`: Factor by which the `spatial_resolution` in x and y of each detector plane will be multiplied. These calculated values are defining an ellipse which is then used as the maximum distance in the local XY plane allowed between clusters and a track for association to the track. This allows the spatial cuts between different planes to be detector appropriate. By default, a relative spatial cut is applied. Absolute and relative spatial cuts are mutually exclusive. Defaults to `3.0`.
* `spatial_cut_abs`: Specifies a set of absolute value (x and y) which defines an ellipse for the maximum spatial distance in the local XY plane between clusters and a track for association to the track. Absolute and relative spatial cuts are mutually exclusive. No default value.
* `min_hits_on_track`: Minimum number of associated clusters needed to create a track, equivalent to the minimum number of planes required for each track. Default value is `6`.
* `exclude_dut`: Boolean to choose if the DUT plane is included in the track finding. Default value is `true`.
* `exclude_from_seed`: Names of detectors which should not be used as the first detector for the creation of the straigh line track seed, even if they are used in the tracking. This can be useful to build better seed tracks e.g. if the first plane in z has a much worse spatial resolution than the following tracking planes. Default is empty.
* `require_detectors`: Names of detectors which are required to have a cluster on the track. If a track does not have a cluster from all detectors listed here, it is rejected. If empty, no detector is required. Default is empty.
* `timestamp_from`: Defines the detector which provides the track timestamp. This detector is by default added to `required_detector`. If empty, the average timestamp of all clusters on the track will be used. Empty by default.
* `track_model`: Select the track model used for reconstruction. A simple line fit ignoring scattering (`straightline`) and a General-Broken-Lines (`gbl`) are currently supported. Defaults to  `straightline`.
* `momentum`: Set the beam momentum. Defaults to 5 GeV
* `volume_scattering`: Select if volume scattering will be taken into account - defaults to false
* `volume_radiation_length`: Define the radiation length of the volume around the telescope. Defaults to dry air with a radiation length of`304.2 m`
* `reject_by_roi`: If true, tracks intercepting any detector outside its ROI will be rejected. Defaults to `false`.
* `unique_cluster_usage`: Only use a cluster for one track - in the case of multiple assignments, the track with the best chi2/ndof is kept. Defaults to `false`
* `max_plot_chi2`: Option to define the maximum chi2 in plots for chi2 and chi2/ndof - with an ill-aligned telescope, this is necessary for an initial alignment step. Defaults to `50.0`

### Plots produced

The following plots are produced only once:

* Histograms of the track chi2 and track chi2/ndf
* Histogram of the clusters per track, and tracks per event
* Histograms of the track angle with respect to the X/Y-axis

For each detector, the following plots are produced:

* Histograms of the track residual in X/Y for various cluster sizes (1-3)

### Usage
```toml
[Tracking4D]
min_hits_on_track = 4
spatial_cut_abs = 300um, 300um
time_cut_abs = 200ns
exclude_dut = true
require_detectors = "ExampleDetector_0", "ExampleDetector_1"
track_model = "straightline"
```
