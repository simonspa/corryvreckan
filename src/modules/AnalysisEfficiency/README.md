# AnalysisEfficiency
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>), Jens Kroeger (<jens.kroeger@cern.ch>)  
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional

### Description
This module measures the efficiency of the DUT by comparing its cluster positions with the interpolated track position at the DUT.
It also comprises a range of histograms to investigate where inefficiencies might come from.

The efficiency is calculated as the fraction of tracks with associated clusters on the DUT over the the total number of tracks intersecting the DUT (or region-of-interest, if defined).
It is stored in a ROOT `TEfficiency` object (see below).
Its uncertainty is calculated using the default ROOT `TEfficiency` method which is applying a Clopper-Pearson confidence interval of one sigma.
Analogue to a Gaussian sigma, this corresponds to the central 68.3% of a binomial distribution for the given efficiency but taking into account a lower limit of 0 and an upper limit of 1.
This method is recommended by the Particle Data Group.
More information can be found in the ROOT `TEfficiency` class reference, section `ClopperPearson()` @root-tefficiency-class-ref.

### Parameters
* `time_cut_frameedge`: Parameter to discard telescope tracks at the frame edges (start and end of the current event window). Defaults to `20ns`.
* `chi2ndof_cut`: Acceptance criterion for telescope tracks, defaults to a value of `3`.
* `inpixel_bin_size`: Parameter to set the bin size of the in-pixel 2D efficiency histogram. This should be given in units of distance and the same value is used in both axes. Defaults to `1.0um`.
* `inpixel_cut_edge`: Parameter to exclude tracks going within a cut-distance to the pixel edge. Defaults to `5um`.
* `masked_pixel_distance_cut`: Distance (in pixels) to exclude tracks passing close to masked pixel. Defaults to `1`.
* `require_associated_cluster_on`: Names of detectors which are required to have an associated cluster to the telescope tracks. Detectors listed here must be marked as `role = DUT` in the detector configuration file. Only tracks satisfying this requirement are accepted for the efficiency measurement. If empty, no detector is required. Default is empty.

### Plots produced

For the DUT, the following plots are produced:

* 2D histograms:
  * 2D Map of in-pixel efficiency
  * 2D Maps of chip efficiency in local and global coordinates, filled at the position of the track intercept point or at the position of the associated cluster center
  * 2D Map of pixels efficiency, for the full matrix, filled at the pixel (of the associated cluster) for which the track goes through, constrained to an inpixel-ROI defined by `inpixel_cut_edge`.
  * 2D Maps of the position difference of a track with and without associated cluster to the previous track
  * 2D Map of the distance between track intersection and associated cluster

* 1D histograms:
  * Histogram of all single-pixel efficiencies
  * Histograms of time difference of the matched and non-matched track time to the previous track
  * Histograms of the row and column difference of the matched and non-matched track time to the previous track
  * Histograms of the time difference of a matched (non-matched) cluster to a previous hit (not matter if noise or track)
  * Distribution of cluster-track distances

* Other:
  * Value of total efficiency as `TEfficiency` including (asymmetric) error bars
  * Efficiency as function of column and row


### Usage
```toml
[AnalysisEfficiency]
chi2ndof_cut = 5
```
[@root-tefficiency-class-ref]: https://root.cern.ch/doc/master/classTEfficiency.html#ae80c3189bac22b7ad15f57a1476ef75b
