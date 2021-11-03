![](doc/logo_small.png)

# Corryvreckan
### The Maelstrom for Your Test Beam Data

For more details about the project please have a look at the website at https://cern.ch/corryvreckan.

[![build status](https://gitlab.cern.ch/simonspa/corryvreckan/badges/master/build.svg)](https://gitlab.cern.ch/simonspa/corryvreckan/commits/master)

## Dependencies
* [ROOT](https://root.cern.ch/building-root) (required, with the GenVector component)

## Installation
The CMake build system is used for compilation and installation. The install directory can be specified by adding `-DCMAKE_INSTALL_PREFIX=<prefix>` as argument to the CMake command below. Other configuration options are explained in the manual.

The dependencies need to be initialized for the build to succeed.

### Compilation
To compile and install a default installation of Corryvreckan, run the following commands

```
$ mkdir build && cd build/
$ cmake ..
$ make install
```

For more detailed installation instructions, please refer to the documentation below.

## Documentation
The PDF user manual is automatically compiled by the continuous integration and can be [downloaded here](https://gitlab.cern.ch/corryvreckan/corryvreckan/-/jobs/artifacts/master/raw/public/usermanual/corryvreckan-manual.pdf?job=cmp%3Ausermanual).

The latest PDF version of the User Manual can be created from source by executing
```
$ make pdf
```
After running the manual is available under `usermanual/corryvreckan-manual.pdf` in the build directory.

To build the HTML version of the latest Doxygen reference, run the following command
```
$ make reference
```
The main page of the reference can then be found at `reference/html/index.html` in the build folder.

## Development of Corryvreckan

Corryvreckan has been developed and is maintained by

* Morag Williams, University of Glasgow/CERN, @williamm
* Jens Kroeger, University of Heidelberg/CERN, @jekroege
* Lennart Huth, DESY, @lhuth
* Paul Schütze, DESY, @pschutze
* Simon Spannagel, DESY, @simonspa

The following authors, in alphabetical order, have contributed to Corryvreckan:

* Matthew Daniel Buckland, University of Liverpool, @mbucklan
* Carsten Daniel Burgard, DESY, @cburgard
* Manuel Colocci, CERN, @mcolocci
* Jens Dopke, STFC RAL, @jdopke
* Jordi Duarte-Campderros, IFCA, @duarte
* Nicolò Jacazio, CERN, @njacazio
* Chun Cheng, DESY, @chengc
* Dominik Dannheim, CERN, @dannheim
* Katharina Dort, University of Giessen/CERN, @kdort
* Alexander Ferk, CERN, @aferk
* Adrian Fiergolski, CERN, @afiergol
* Sejla Hadzic, MPP, @sehadzic
* Daniel Hynds, Nikhef, @dhynds
* Magnus Mager, CERN, @mmager
* Andreas Matthias Nürnberg, KIT, @nurnberg
* Ryunosuke O'Neil, University of Edinburgh, @roneil
* Klaas Padeken, Bonn, HISKP, @padeken
* Florian Pitters, HEPHY, @fpipper
* Tomas Vanat, CERN, @tvanat
* Jin Zhang, DESY, @jinz

## Citations
The reference paper of Corryvreckan describing the framework and providing an example for a test beam data reconstruction has been published in the *Journal of Instrumentation*.
The paper is published with open access and can be obtained from here:

https://doi.org/10.1088/1748-0221/16/03/P03008

Please cite this paper when publishing your work using Corryvreckan as:

> D. Dannheim et al., “Corryvreckan: a modular 4D track reconstruction and analysis software for test beam data”, J. Instr. 16 (2021) P03008, doi:10.1088/1748-0221/16/03/P03008, arXiv:2011.12730

A preprint version is available on [arxiv.org](https://arxiv.org/abs/2011.12730).

In addition, the software can be cited using the [versioned Zenodo record](https://doi.org/10.5281/zenodo.4384170) or the current version as:

>  M. Williams, J. Kröger, L. Huth, P. Schütze, S. Spannagel. (2020, December 22). Corryvreckan - A Modular 4D Track Reconstruction and Analysis Software for Test Beam Data
> (Version 2.0). Zenodo. http://doi.org/10.5281/zenodo.4384186

## Contributing
All types of contributions, being it minor and major, are very welcome. Please refer to our [contribution guidelines](CONTRIBUTING.md) for a description on how to get started.

Before adding changes it is very much recommended to carefully read through the documentation in the User Manual first.

## Licenses
This software is distributed under the terms of the MIT license. A copy of this license can be found in [LICENSE.md](LICENSE.md).

The documentation is distributed under the terms of the CC-BY-4.0 license. This license can be found in [doc/COPYING.md](doc/COPYING.md).

This project strongly profits from the developments done for the [Allpix Squared project](https://cern.ch/allpix-squared) which is released under the MIT license. Especially the configuration class, the module instantiation logic and the file reader and writer modules have profited heavily by their corresponding framework components in Allpix Squared.

The LaTeX and Pandoc CMake modules used by Corryvreckan are licensed under the BSD 3-Clause License.

The General Broken Lines library for track fitting is distributed under the terms of the GNU General Public License version 2. The license can be found [here](3rdparty/GeneralBrokenLines/COPYING.LIB), the original source code is available from [here](www.terascale.de/wiki/generalbrokenlines/).
