# Millepede
**Maintainer**: Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Work in progress  

### Description
This implementation of the Millepede module has been taken from the [Kepler framework](https://gitlab.cern.ch/lhcb/Kepler) used for test beam data analysis within the LHCb collaboration. It has been written by Christoph Hombach and has seen contributions from Tim Evans and Heinrich Schindler. This version is only slightly adapted to the Corryvreckan framework.

The Millepede algorthm allows a simultaneous fit of both the tracks and the alignment constants.

The modules stops if the convergence, i.e. the absolute sum of all corrections over the total number of parameters, is smaller than the configured value.

### Parameters
* `number_of_tracks`: Number of tracks used in the alignment method chosen. Default value is `20000`.
* `iterations`: Number of times the chosen alignment method is to be iterated. Default value is `3`.
* `dofs`: Degrees of freedom to be aligned. This parameter should be given as vector of six boolean values for the parameters "Translation X", "Translation Y", "Translation Z", "Rotation X", "Rotation Y" and "Rotation Z". The default setting is an alignment of all parameters except for "Translation Z", i.e. `dofs = true, true, false, true, true, true`.
* `residual_cut`: Residual cut to reject a track as an outlier. Default value is `0.05mm`;
* `residual_cut_init`: Initial residual cut for outlier rejection in the first iteration. This value is applied for the first iteration and replaced by `residual_cut` thereafter. Default value is `0.6mm`.
* `nstddev`: Cut to reject track candidates based on their Chi2/ndof value. Default value is `0`, i.e. the feature is disabled.
* `sigmas`: Uncertainties for each of the alignment parameters. Defaults to `0.05, 0.05, 0.5, 0.005, 0.005, 0.005`.
* `convergence`: Convergence value at which the module stops iterating. Default value is `10e-5`.

### Usage
```toml
[Millepede]
iterations = 10
dofs = true, true, false, true, true, true
```
Parameters to be used in multiple modules can also be defined globally at the top of the configuration file. This is highly encouraged for parameters such as `DUT` and `reference`.
