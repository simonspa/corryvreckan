# EtaCorrection
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Module Type**: *DETECTOR*  
**Detector Type**: *all*  
**Status**: Work in progress

### Description
This module applies previously determined $`\eta`$-corrections to cluster positions of any detector in order to correct for non-linear charge sharing. Corrections can be applied to any cluster read from the clipboard. The correction function as well as the parameters for each of the detectors can be given separately for X and Y via the configuration file.

This module does not calculate the $`\eta`$ distribution.

### Parameters
* `eta_formula_x` / `eta_formula_y`: The formula for the $`\eta`$ correction to be applied for the X an Y coordinate, respectively. It defaults to a polynomial of fifth ordern, i.e. `[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5`.
* `eta_constants_x_<detector>` / `eta_constants_y_<detector>`: Vector of correction factors, representing the parameters of the above correction function, in X and Y coordinates, respectively. Defaults to an empty vector, i.e. by default no correction is applied. The `<detector>` part of the variable has to be replaced with the respective unique name of the detector given in the setup file.

### Plots produced
Currently, no plots are produced. The result of the $`\eta`$-correction is best observed in residual distributions of the respective detector.

### Usage
```toml
[EtaCorrection]
eta_formula_x = [0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5
eta_constants_x_dut = 0.025 0.038  6.71 -323.58  5950.3 -34437.5
```
