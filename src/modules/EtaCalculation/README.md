# EtaCalculation
**Maintainer**: Daniel Hynds (<daniel.hynds@cern.ch>), Simon Spannagel (<simon.spannagel@cern.ch>)  
**Status**: Under development  

### Description
This module calculates the $`\eta`$-distributions for two-pixel clusters of any detector in analysis by comparing the in-pixel track position and the calculated cluster centre position. Histograms for all available detectors are filled for both X and Y coordinate.
At the end of the run, fits to the recorded profiles are performed using the provided formulas. A printout of the resulting fit parameters is provided in the format read by the EtaCorrection module for convenience.

In order to measure the correct $`\eta`$-distribution, no additional $`\eta`$-correction should be applied during this calculation, i.e. by using the EtaCorrection module.

### Parameters
* `chi2ndofCut`: Track quality cut on its Chi2 over numbers of degrees of freedom. Default value is `100`.
* `EtaFormulaX` / `EtaFormulaY`: Formula to for to the recorded $`\eta`$-distributions, defaults to a polynomial of fifth order, i.e. `[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5`.

### Plots produced
For each detector the following plots are produced:

* 2D histogram of the calculated $`\eta`$-distribution, for X and Y respectively
* Profile plot of the calculated $`\eta`$-distribution, for X and Y respectively

### Usage
```toml
[EtaCalculation]
chi2ndofCut = 100
```
