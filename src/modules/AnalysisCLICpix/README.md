# AnalysisCLICpix
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>)   
**Module Type**: *DUT*  
**Detector Type**: *CLICpix*  
**Status**: Functional  

### Description
This module associates CLICpix2 DUT clusters to tracks using a spatial cut (device type not checked). A significant number of analysis plots are produced.

### Parameters
* `associationCut`: Maximum distance between a track and cluster for them to be associated. Units of mm. Default value is `0.05` (50um).
* `proximityCut`: Maximum distance apart two tracks are for them to be 'close' to each other. If at the CLICpix plane there are two tracks close to each other, the DUT cluster is not associated with either track. Units of mm. Default value is `0.0005` (0.5um).
* `timepix3Telescope`: Boolean to set whether the Timepix3 telescope is being used. Default value is `false`.
* `DUT`: Name of the DUT plane. The CLICpix device is assumed to be the DUT.

### Plots produced
For the DUT the following plots are produced:

* 2D hitmap
* Column hits histogram
* Row hits histogram

* Cluster size histogram
* CLuster ToT histogram
* Clusters per event histogram
* Clusters vs event number
* Cluster width histogram (rows, Y-axis)
* Cluster width histogram (columns, X-axis)

* Global track difference
* Global track difference in Y
* Global residuals in X
* Global residuals in Y
* Absolute residuals
* Global residuals in X vs X
* Global residuals in X vs Y
* Global residuals in Y vs X
* Global residuals in Y vs Y
* Global residuals in X vs column width
* Global residuals in X vs row width
* Global residuals in Y vs column width
* Global residuals in Y vs row width

* Track intercept in rows
* Track intercept in columns
* Absolute residual map histogram
* X residual vs Y residual
* Associated clusters per event histogram
* Associated clusters vs event number
* Associated clusters vs trigger number
* Associated cluster row
* Associated cluster column
* Frame efficiency histogram
* Frame tracks
* Associated frame tracks
* Associated cluster size
* Associated cluster width (row)
* Associated cluster width (column)
* Associated 1-pixel cluster ToT
* Associated 2-pixel cluster ToT
* Associated 3-pixel cluster ToT
* Associated 4-pixel cluster ToT
* Pixel response in X
* Pixel response in X in global coordinates
* Pixel response in X for odd columns
* Pixel response in X for even columns
* Pixel response in Y
* Pixel response in Y in global coordinates
* Pixel response in Y for odd columns
* Pixel response in Y for even columns
* 2D eta distribution in X
* 2D eta distribution in Y

* Local residual for rows for 2-pixel clusters
* Local residual for columns for 2-pixel clusters
* Cluster ToT for rows for 2-pixel clusters
* Cluster ToT for columns for 2-pixel clusters
* Pixel ToT for rows for 2-pixel clusters
* Pixel ToT for columns for 2-pixel clusters
* Cluster ToT ratio for rows for 2-pixel clusters
* Cluster ToT ratio for rows for 2-pixel clusters
* Local residuals for rows for 2-pixel clusters

* 2D track intercepts
* 2D associatedd track intercepts
* 2D cluster positions in global coordinates
* 2D associated cluster positions in global coordinates
* 2D track-pixel intercepts
* 2D track-associated pixel intercepts
* 2D track-chip intercepts
* 2D track-associated chip intercepts
* 2D track-unassociated chip intercepts
* 2D track-chip intercepts lost
* 2D pixel efficiency
* 2D chip efficiency
* 2D global efficiency
* 2D intercepts for cluster size of 1-pixel
* 2D intercepts for cluster size of 2-pixel
* 2D intercepts for cluster size of 3-pixel
* 2D intercepts for cluster size of 4-pixel
* 2D Associated cluster size map


### Usage
```toml
[CLICpixAnalysis]
associationCut = 0.005
proximityCut = 0.005
timepix3Telescope = true
DUT = "W0003_H05"
```
