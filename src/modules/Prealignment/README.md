# Prealignment
**Maintainer**: Morag Williams (<morag.williams@cern.ch>)   
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functional   

### Description
This module performs translational telescope plane alignment.

This initial alignment along the X and Y axes is designed to be performed before the `Alignment` module, which carries out translational and rotational alignment of the planes. To not include the DUT in this transaltional alignment, it will need to be masked in the configuration file.

The required translational shifts in X and Y are calculated for each detector as the mean of the 1D correlation histogram along the axis.

### Parameters
* `damping_factor`: A factor to change the percentage of the calculated shift applied to each detector. Default value is `1`.
* `max_correlation_rms`: The maximum RMS of the 1D correlation histograms allowed for the shifts to be applied. This factor should be tuned for each run, and is combat the effect of flat distributions. Default value is `6mm`.
* `time_cut_rel`: Number of standard deviations the `time_resolution` of the detector plane will be multiplied by. This value is then used as the maximum time difference between a cluster on the current detector and a cluster on the reference plane to be considered in the prealignment. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference between a cluster on the current detector and a cluster on the reference plane to be considered in the prealignment. Absolute and relative time cuts are mutually exclusive. No default value.

### Plots Created
For each detector the following plots are produced:

* 1D correlation plot for X
* 1D correlation plot for Y
* 2D correlation plot for X in local coordinates (comparing to reference plane)
* 2D correlation plot for Y in local coordinates
* 2D correlation plot for X in global coordinates
* 2D correlation plot for Y in global coordinates

### Usage
```toml
[Prealignment]
log_level = INFO
max_correlation_rms = 6.0
damping_factor = 1.0
```
