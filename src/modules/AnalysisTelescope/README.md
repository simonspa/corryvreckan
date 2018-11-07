# TelescopeAnalysis
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)
**Status**: Work in progress

### Description
This module produces reference histograms for the telescope performance used for tracking. It produces local and global biased residuals for each of the telescope planes, and if Monte Carlo information is available, calculates the residuals between track and Monte Carlo particle.

Furthermore, the telescope resolution at the position of the DUT detector is plotted of Monte Carlo information is available. The Monte Carlo particle position is compared with the track interception with the DUT.

### Parameters
* `chi2ndofCut`: Chi2 over number of degrees of freedom for the track to be taken into account. Tracks with a larger value are discarded. Default value is `3`.

### Plots produced
* Telescope resolution at position of DUT

For each detector participating in tracking, the following plots are produced:

* Biased local track residual, for X and Y;
* Biased global track residual, for X and Y;
* Local residuals with track and Monte Carlo particle, for X and Y;
* Global residuals with track and Monte Carlo particle, for X and Y;

### Usage
```toml
[NAME]
chi2ndofCut = 3
```
