![](doc/logo_small.png)

# Corryvreckan
### The Maelstrom for Your Test Beam Data

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
* Daniel Hynds, CERN, @dhynds
* Simon Spannagel, CERN, @simonspa

The following authors, in alphabetical order, have contributed to Corryvreckan:

* Matthew Daniel Buckland, University of Liverpool, @mbucklan
* Adrian Fiergolski, CERN, @afiergol
* Andreas Matthias Nürnberg, CERN, @nurnberg
* Florian Pitters, CERN, @fpipper

## Contributing
All types of contributions, being it minor and major, are very welcome. Please refer to our [contribution guidelines](CONTRIBUTING.md) for a description on how to get started.

Before adding changes it is very much recommended to carefully read through the documentation in the User Manual first.

## Licenses
This software is distributed under the terms of the MIT license. A copy of this license can be found in [LICENSE.md](LICENSE.md).

The documentation is distributed under the terms of the CC-BY-4.0 license. This license can be found in [doc/COPYING.md](doc/COPYING.md).

The LaTeX and Pandoc CMake modules used by Corryvreckan are licensed under the BSD 3-Clause License.
