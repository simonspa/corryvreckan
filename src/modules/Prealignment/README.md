# Prealignment
**Maintainer**: Morag Williams (<morag.williams@cern.ch>)   
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Functional   

### Description
This module performs translational telescope plane alignment. The rotational alignment is not changed.

This initial alignment along the X and Y axes is designed to be performed before the `Alignment` module, which carries out translational and rotational alignment of the planes. To not include the DUT in this translational alignment, it will need to be masked in the configuration file.

The way in which the required translational shifts in X and Y are calculated is specified by the parameter `method`.
Either the mean of the 1D correlation histogram, its maximum or the mean of a Gaussian fit can be used to determine the translational shifts. 
As described in the alignment chapter of the user manual, the spatial correlations in X and Y should not be forced to be centered around zero for the final alignment as they correspond to the *physical displacement* of the detector plane in X and Y with respect to the reference plane.
However, for the prealignment this is a an acceptable estimation which works without any tracking.

### Parameters
* `damping_factor`: A factor to change the percentage of the calculated shift applied to each detector. Default value is `1`.
* `max_correlation_rms`: The maximum RMS of the 1D correlation histograms allowed for the shifts to be applied. This factor should be tuned for each run, and is combat the effect of flat distributions. Default value is `6mm`.
* `time_cut_rel`: Number of standard deviations the `time_resolution` of the detector plane will be multiplied by. This value is then used as the maximum time difference between a cluster on the current detector and a cluster on the reference plane to be considered in the prealignment. Absolute and relative time cuts are mutually exclusive. Defaults to `3.0`.
* `time_cut_abs`: Specifies an absolute value for the maximum time difference between a cluster on the current detector and a cluster on the reference plane to be considered in the prealignment. Absolute and relative time cuts are mutually exclusive. No default value.
* `method`: Specifies which method should be used to compute the translational shifts. With the option `mean` the mean of the 1D correlation histogram is used. The option `maximum` uses the maximum value of the histogram. With `method` set to `gauss_fit` a Gaussian is fitted and the mean of the fit is used for the translational shift. Default is to `mean`. 
* `fit_range_rel`: Parameter to set the fit range of the Gaussian fit if `method` is set to `gauss_fit`. The absolute fit range is given by `fit_range_rel` times the spatial resolution of the corresponding detector around the maximum of the 1D correlation histogram. The default of the relative fit range is `fit_range_rel` = 500.
* `range_abs`: Parameter to allow setting up the range in which residuals get plotted (mm, +- around 0). Default is `10mm`. This needs to be increased for large sensors, where the alignment might be out by well more than 10mm and thus residual plots might be empty in the range +- 10.

### Plots Created

For each detector the following plots are produced:

* 1D histograms of the correlations in X/Y (comparing to the reference plane)
* 2D histograms of the correlation plot for X/Y in local/global coordinates (comparing to the reference

### Usage
```toml
[Prealignment]
log_level = INFO
max_correlation_rms = 6.0
damping_factor = 1.0
```
