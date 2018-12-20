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
* `damping_factor`: A factor to change the percentage of the calcuated shift applied to each detector. Default value is `1`.
* `max_correlation_rms`: The maximum RMS of the 1D correlation histograms allowed for the shifts to be applied. This factor should be tuned for each run, and is combat the effect of flat distributions. Default value is `6mm`.
* `timing_cut`: maximum time difference between clusters to be taken into account. Defaults to `100ns`.

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
