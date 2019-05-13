# AnalysisTimingATLASpix
**Maintainer**: Jens Kroeger (jens.kroeger@cern.ch)  
**Module Type**: *DUT*  
**Detector Type**: *ATLASpix*  
**Status**: work in progress

### Description
This module contains everything that's necessary for an in-depth timing analysis of the ATLASpix, including timewalk and row corrections.

### Parameters
* `clusterTotCut`: Parameter to discard clusters with a ToT too large, default is 500ns (inifitely large).
* `clusterSizeCut`: Parameter to discard clusters with a size too large, only for debugging purposes, default is 100 (inifitely large).
* `chi2ndofCut`: Acceptance criterion for telescope tracks, defaults to a value of `3`.

### Plots produced
* to be updated...

### Usage
```toml
[AnalysisTiming]

```
