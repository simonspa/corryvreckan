# AlignmentDUTResidual
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module performs translational and rotational DUT alignment. The alignment is performed with respect to the reference plane set in the configuration file.

This module uses tracks for alignment. The module moves the detector is is instantiated for and minimises the unbiased residuals calculated from the track intercepts with the plane.

### Parameters
* `number_of_tracks`: Number of tracks used in the alignment method chosen. Default value is `20000`.
* `iterations`: Number of times the chosen alignment method is to be iterated. Default value is `3`.
* `alignPosition`: Boolean to select whether to align the X and Y displacements of the detector or not. Note that the Z displacement is never aligned. The default value is `true`.
* `alignOrientation`: Boolean to select whether to align the three rotations of the detector under consideration or not. The default value is `true`.
* `prune_tracks`: Boolean to set if tracks with a number of associated clusters > `max_associated_clusters` or with a track chi^2 > `max_track_chi2ndof` should be excluded from use in the alignment. This parameter was designed for `alignmentMethod=1`. The number of discarded tracks is outputted on terminal. Default is `False`.
* `max_associated_clusters`: Maximum number of associated clusters per track allowed when `prune_tracks=True` for the track to be used in the alignment. Default value is `1`.
* `max_track_chi2ndof`: Maximum track chi^2 value allowed when `prune_tracks=True` for the track to be used in the alignment. Default value is `10.0`.

### Plots produced
For the detector under consideration, the following plots are produced:

* Residual in X (calculated in local coordinates)
* Residual in Y
* Profile plot of residual in X vs. X position
* Profile plot of residual in X vs. Y position
* Profile plot of residual in Y vs. X position
* Profile plot of residual in Y vs. Y position


### Usage
```toml
[AlignmentDUTResidual]
number_of_tracks = 1000000
log_level = INFO
```
