# AlignmentTrackChi2
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This module performs translational and rotational telescope plane alignment. The alignment is performed with respect to the reference plane set in the configuration file.

This module uses tracks on the clipboard to align the telescope planes.
For each telescope detector except the reference plane, this method moves the detector, refits all of the tracks, and minimises the chi^2 of these new tracks. This method automatically iterates through all devices contributing to the track.

### Parameters
* `iterations`: Number of times the chosen alignment method is to be iterated. Default value is `3`.
* `align_position`: Boolean to select whether to align the X and Y displacements of the detector or not. Note that the Z displacement is never aligned. The default value is `true`.
* `align_orientation`: Boolean to select whether to align the three rotations of the detector under consideration or not. The default value is `true`.
* `prune_tracks`: Boolean to set if tracks with a number of associated clusters > `max_associated_clusters` or with a track chi^2 > `max_track_chi2ndof` should be excluded from use in the alignment. The number of discarded tracks is outputted on terminal. Default is `false`.
* `max_associated_clusters`: Maximum number of associated clusters per track allowed when `prune_tracks = true` for the track to be used in the alignment. Default value is `1`.
* `max_track_chi2ndof`: Maximum track chi^2 value allowed when `prune_tracks = true` for the track to be used in the alignment. Default value is `10.0`.

### Plots produced
For each detector the following plots are produced:

* Translational shift along X-axis vs. iteration number
* Translational shift along Y-axis vs. iteration number
* Translational shift along Z-axis vs. iteration number
* Rotational shift around X-axis vs. iteration number
* Rotational shift around Y-axis vs. iteration number
* Rotational shift around Z-axis vs. iteration number

### Usage
```toml
[Corryvreckan]
# The global track limit can be used to restrict the alignment:
number_of_tracks = 200000

[AlignmentTrackChi2]
log_level = INFO
```
