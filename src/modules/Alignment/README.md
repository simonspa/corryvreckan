## Alignment
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Functional

#### Description
This module performs translational and rotational telescope plane alignment. The alignment is performed with respect to the reference plane set in the configuration file.

This module uses the tracks produced by the `BasicTracking` module to align the telescope planes. If fewer than half of the tracks have associated clusters, a warning is produced on terminal.

There are two methods available for alignment:

##### 1) Minimising the track chi^2
For each telescope detector except the reference plane, this method moves the detector, refits all of the tracks, and minimises the chi^2 of these new tracks. The parameters `detectorToAlign` and `DUT` are not used in this method as it automatically iterates through all telescope planes except the DUT.

##### 2) Minimising the track (unbiased) residuals
For the detector specified by the `detectorToAlign` parameter, this method moves the detector, refits all the tracks, and minimises the unbiased residuals calculated from the track intercepts with the plane.

#### Parameters
* `number_of_tracks`: Number of tracks used in the alignment method chosen. Default value is `20000`.
* `iterations`: Number of times the chosen alignment method is to be iterated. Default value is `3`.
* `alignmentMethod`: Determines the alignment method used. To use the track chi^2 minimisation `alignmentMethod = 0`; to use the track residual minimisation `alignmentMethod = 1`.
* `detectorToAlign`: Parameter to set a particular plane to align. This parameter is only used in the residuals method (`alignmentMethod = 1`). The default is the `DUT` plane.
* `DUT`: Name of the DUT plane.
* `reference`: Name of the detector used as the alignment reference plane. All other telescope planes are aligned with respect to the reference plane.
* `alignPosition`: Boolean to select whether to align the X and Y displacements of the detector or not. Note that the Z displacement is never aligned. This parameter is only used in `alignmentMode = 1`. The default value is `true`.
* `alignOrientation`: Boolean to select whether to align the three rotations of the detector under consideration or not.  This parameter is only used in `alignmentMode = 1`. The default value is `true`.
* `prune_tracks`: Boolean to set if tracks with a number of associated clusters > `max_associated_clusters` or with a track chi^2 > `max_track_chi2ndof` should be excluded from use in the alignment. This parameter was designed for `alignmentMethod=1`. The number of discarded tracks is outputted on terminal. Default is `False`.
* `max_associated_clusters`: Maximum number of associated clusters per track allowed when `prune_tracks=True` for the track to be used in the alignment. Default value is `1`.
* `max_track_chi2ndof`: Maximum track chi^2 value allowed when `prune_tracks=True` for the track to be used in the alignment. Default value is `10.0`.

#### Plots produced
For each detector the following plots are produced when using `alignmentMethod = 0`:
* Translational shift along X-axis vs. iteration number
* Translational shift along Y-axis vs. iteration number
* Translational shift along Z-axis vs. iteration number
* Rotational shift around X-axis vs. iteration number
* Rotational shift around Y-axis vs. iteration number
* Rotational shift around Z-axis vs. iteration number

For the `detectorToAlign` the following plots are produced when using `alignmentMethod = 1`:
* Residual in X (calculated in local coordinates)
* Residual in Y
* Profile plot of residual in X vs. X position
* Profile plot of residual in X vs. Y position
* Profile plot of residual in Y vs. X position
* Profile plot of residual in Y vs. Y position


#### Usage
```toml
[Alignment]
alignmentMethod = 0
masked = "W0005_H03" #excluding the DUT from the alignment
number_of_tracks = 1000000
log_level = INFO
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
