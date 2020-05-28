# OnlineMonitor
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *GLOBAL*  
**Status**: Functional

### Description
This module opens a GUI to monitor the progress of the reconstruction.
Since Linux allows concurrent (reading) file access, this can already be started while a run is recorded to disk and thus serves as an online monitoring tool during data taking.
A set of canvases is available to display a variety of information ranging from hitmaps and basic correlation plots to more advances results such as tracking quality and track angles.
The plots on each of the canvases contain real time data, automatically updated every `update` events.

The displayed plots and their source can be configured via the framework configuration file.
For plots from clustering and tracking modules, the module name can be specified to display standard plots of the corresponding module.
In addition, overruling the standard plot configuration, each canvas can be configured via a matrix containing the path of the plot and its plotting options in each row, e.g.

```toml
dut_plots =  [["EventLoaderEUDAQ2/%DUT%/hitmap", "colz"],
             ["EventLoaderEUDAQ2/%DUT%/hPixelRawValues", "log"]]
```


The allowed plotting options comprise all drawing options offered by ROOT.
In addition, the `log` keyword is supported, which switches the Y axis of the respective plot to a logarithmic scale.

Several keywords can be used in the plot path, which are parsed and interpreted by the OnlineMonitor module:

* `%DETECTOR%`: If this keyword is found, the plot is added for each of the available detectors by replacing the keyword with the respective detector name.
* `%DUT%`: This keyword is replaced by the value of the `DUT` configuration key of the framework.
* `%REFERENCE%`: This keyword is replaced by the vale of the `reference` configuration key of the framework.

The "corryvreckan" namespace is not required to be added to the plot path.

### Parameters

#### General parameters
* `update`: Number of events after which to update, defaults to `500`.
* `canvas_title`: Title of the GUI window to be shown, defaults to `Corryvreckan Testbeam Monitor`. This parameter can be used to e.g. display the current run number in the window title.
* `ignore_aux`: With this boolean variable set, detectors with `auxiliary` roles are ignored and none of their histograms are added to the UI. Defaults to `true`.
* `clustering_module`: Module for which the standard clustering plots are selected. Defaults to `Clustering4D`.
* `tracking_module`: Module for which the standard tracking plots are selected. Defaults to `Tracking4D`.

#### Canvas parameters
* `overview`: List of plots to be placed on the "Overview" canvas of the online monitor. The list of plots created in the default configuration is listed below.
* `dut_plots`: List of plots to be placed on the "DUTs" canvas of the online monitor. By default, this canvas contains plots collected from the `EventLoaderEUDAQ2` as well as the `AnalysisDUT` modules for the each configured DUT. This canvas should be customized for the respective DUT.
* `hitmaps`: List of plots to be placed on the "Hitmaps" canvas of the online monitor. By default, this canvas displays `Correlations/%DETECTOR%/hitmap` for all detectors.
* `tracking`: List of plots to be placed on the "Tracking" canvas of the online monitor. The list of plots created in the default configuration is listed below.
* `residuals`: List of plots to be placed on the "Residuals" canvas of the online monitor. By default, this canvas displays `Tracking4D/%DETECTOR%/residualsX` for all detectors.
* `correlation_x`: List of plots to be placed on the "CorrelationX" canvas of the online monitor.  By default, this canvas displays `Correlations/%DETECTOR%/correlationX` for all detectors.
* `correlation_y`: List of plots to be placed on the "CorrelationY" canvas of the online monitor.  By default, this canvas displays `Correlations/%DETECTOR%/correlationY` for all detectors.
* `correlation_x2d`: List of plots to be placed on the "CorrelationX2D" canvas of the online monitor. By default, this canvas displays `Correlations/%DETECTOR%/correlationX_2Dlocal` for all detectors.
* `correlation_y2d`: List of plots to be placed on the "CorrelationY2D" canvas of the online monitor. By default, this canvas displays `Correlations/%DETECTOR%/correlationY_2Dlocal` for all detectors.
* `charge_distributions`: List of plots to be placed on the "ChargeDistributions" canvas of the online monitor. By default, this canvas displays `Clustering4D/%DETECTOR%/clusterCharge` for all detectors.
* `event_times`: List of plots to be placed on the "EventTimes" canvas of the online monitor. By default, this canvas displays `Correlations/%DETECTOR%/eventTimes` for all detectors.

### Plots produced

The following plots are displayed by default (can be configured):

* Overview canvas:
   * Histogram of the cluster ToT of reference plane
   * 2D hit map of reference plane
   * Histogram of the residual in X of reference plane
* Tracking canvas:
   * Histogram of the track $\chi^2$
   * Histogram of the track angle in X

### Usage
```toml
[OnlineMonitor]
update = 200
clustering_module = "ClusteringSpatial"
dut_plots = [["EventLoaderEUDAQ2/%DUT%/hitmap", "colz"],
             ["EventLoaderEUDAQ2/%DUT%/hPixelTimes"],
             ["EventLoaderEUDAQ2/%DUT%/hPixelRawValues"],
             ["EventLoaderEUDAQ2/%DUT%/pixelMultiplicity", "log"],
             ["AnalysisDUT/clusterChargeAssociated"],
             ["AnalysisDUT/associatedTracksVersusTime"]]
```
