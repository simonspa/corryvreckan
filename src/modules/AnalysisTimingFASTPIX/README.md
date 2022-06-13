# AnalysisTimingFASTPIX
**Maintainer**: Eric Buschmann (eric.buschmann@cern.ch)
**Module Type**: *DUT*
**Status**: Immature

### Description
This module performs a timing analysis of FASTPIX data using waveforms from a fast timing reference.
A basic timing analysis without timewalk correction is performed and detailed timing information is written to a ROOT tree which can be used to extract and apply the timewalk correction.

### Parameters
* `chi2ndof_cut`: Acceptance criterion for the maximum telescope tracks chi2/ndf, defaults to a value of `3`.

### Plots produced

For each detector the following plots are produced:

* 1D histogram of time residuals for pixels inside the matrix and on the edge
* 2D histogram of FASTPIX timing vs reference

### Usage
```toml
[AnalysisTimingFASTPIX]

```
