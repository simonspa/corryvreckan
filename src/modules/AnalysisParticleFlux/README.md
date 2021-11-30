# AnalysisParticleFlux
**Maintainer**: Maximilian Felix Caspar (<maximilian.caspar@desy.de>)
**Module Type**: *GLOBAL*
**Status**: Experimental

### Description
`AnalysisParticleFlux` records the zenith and azimuth angles of tracks. The binning of the resulting histograms as well as the bounds are parameters to the module.

### Parameters
* Parameters of the azimuthal angle histogram:
    * `azimuthLow`: Lower bound of the azimuth histogram. Defaults to `0 deg`.
    *  `azimuthHigh`: Upper bound of the azimuth histogram. Defaults to `360 deg`.
    * `azimuthGranularity`: Number of bins in $\varphi$ which defaults to `36`.
* Parameters of the zenith angle histogram:
    * `zenithLow`: Lower bound of the zenith histogram. Defaults to `0 deg`.
    *  `zenithHigh`: Upper bound of the zenith histogram. Defaults to `90 deg`.
    * `zenithGranularity`: Number of bins in $\vartheta$ which defaults to `9`.
* `trackIntercept`: Value of $z$ where the track angles are calculated. Defaults to `0.0` and does not affect the analysis for `straightline` tracks.

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
[AnalysisDUT]
zenithHigh = 20 deg
zenithGranularity = 10
```