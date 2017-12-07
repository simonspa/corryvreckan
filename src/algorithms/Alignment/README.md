## Alignment
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)   
**Status**: Functional   

#### Description
This algorithm performs translational and rotational telescope plane alignment.

The alignment is performed with respect to the reference plane set in the configuration file.

This algorithm uses the tracks produced by the `BasicTracking` algorithm to align the telescope planes. There are two methods available for alignment:

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

#### Plots produced
For each detector the following plots are produced when using `alignmentMethod = 0`:
* Translational shift along X-axis vs. iteration number
* Translational shift along Y-axis vs. iteration number
* Translational shift along Z-axis vs. iteration number
* Rotational shift around X-axis vs. iteration number
* Rotational shift around Y-axis vs. iteration number
* Rotational shift around Z-axis vs. iteration number

For the `detectorToAlign` the following plots are produced when using `alignmentMethod = 1`:
* Residual in X
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
Parameters to be used in multiple algorithms can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
