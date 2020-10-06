# AlignmentDUTResidual
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module performs translational and rotational DUT alignment. The alignment is performed with respect to the reference plane set in the configuration file.

This module uses tracks for alignment. The module moves the detector it is instantiated for and minimizes the unbiased residuals calculated from the track intercepts with the plane.

### Parameters
* `iterations`: Number of times the chosen alignment method is to be iterated. Default value is `3`.
* `align_position`: Boolean to select whether to align the X and Y displacements of the detector or not. Note that the Z displacement is never aligned. The default value is `true`.
* `align_orientation`: Boolean to select whether to align the rotations of the detector under consideration or not. Specify the axes using `align_orientation_axes`. The default value is `true`.
* `align_position_axes`: Define for which axes to perform translational alignment. The default value is `xy`, which means both X and Y displacements of the detector will be aligned.
* `align_orientation_axes`: Define for which axes to perform rotational alignment if `align_orientation = true`. The default value is `xyz`, which means that rotations around X, Y and Z axis will be aligned.
* `prune_tracks`: Boolean to set if tracks with a number of associated clusters > `max_associated_clusters` or with a track chi^2 > `max_track_chi2ndof` should be excluded from use in the alignment. The number of discarded tracks is written to the terminal. Default is `false`.
* `max_associated_clusters`: Maximum number of associated clusters per track allowed when `prune_tracks = true` for the track to be used in the alignment. Default value is `1`.
* `max_track_chi2ndof`: Maximum track chi^2 value allowed when `prune_tracks = true` for the track to be used in the alignment. Default value is `10.0`.

### Plots produced
For the DUT, the following plots are produced:

* Residuals in X and Y (calculated in local coordinates)
* Profile plot of residual in X vs. X, X vs. Y, Y vs. X and Y vs. Y position

### Usage
```toml
[Corryvreckan]
# The global track limit can be used to reduce the run time:
number_of_tracks = 200000

[AlignmentDUTResidual]
log_level = INFO
```
