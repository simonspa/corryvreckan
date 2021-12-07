# AnalysisParticleFlux
**Maintainer**: Maximilian Felix Caspar (<maximilian.caspar@desy.de>)
**Module Type**: *GLOBAL*
**Status**: Experimental

### Description
`AnalysisParticleFlux` records the zenith and azimuth angles of tracks. The binning of the resulting histograms as well as the bounds are parameters to the module.

### Parameters
* Parameters of the azimuthal angle histogram:
    * `azimuth_low`: Lower bound of the azimuth histogram. Defaults to `0 deg`.
    *  `azimuth_high`: Upper bound of the azimuth histogram. Defaults to `360 deg`.
    * `azimuth_granularity`: Number of bins in $`\varphi`$ which defaults to `36`.
* Parameters of the zenith angle histogram:
    * `zenith_low`: Lower bound of the zenith histogram. Defaults to `0 deg`.
    *  `zenith_high`: Upper bound of the zenith histogram. Defaults to `90 deg`.
    * `zenith_granularity`: Number of bins in $`\vartheta`$ which defaults to `9`.
* `track_intercept`: Value of $`z`$ where the track angles are calculated. Defaults to `0.0` and does not affect the analysis for `straightline` tracks.

### Plots produced
The following plots are produced:

* 2D histograms:
    * Zenith angle vs. azimuthal angle of tracks
* 1D histograms:
    *  Histogram of the azimuth angle of tracks
    * Histogram of the zenith angle of tracks

### Usage
This module is intended to be used **after** the `Tracking4D` module:
```toml
[AnalysisParticleFlux]
zenithHigh = 20deg
zenithGranularity = 10
```