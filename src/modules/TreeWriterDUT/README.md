# TreeWriterDUT
**Maintainer**: Morag Williams (<morag.williams@cern.ch>)   
**Module Type**: *DUT*  
**Detector Type**: *all*  
**Status**: Functional  

### Description
This module writes out data from a Timepix3 DUT for timing analysis. The output ROOT tree contains data in branches. This is intended for analysis of the timing capabilities of Timepix3 devices of different thicknesses.

For each track associated DUT `cluster` object the following information is written out:

* Event ID
* Size in X
* Size in Y
* Number of pixels in the cluster

For each `pixel` object in an associated `cluster` the following information is written out:

* X position
* Y position
* ToT
* ToA

For each `track` with associated DUT `clusters` the following information is written out:

* Intercept with the DUT (3D position vector)

### Parameters
* `file_name`: Name of the data file to create, relative to the output directory of the framework. The file extension `.root` will be appended if not present. Default value is `outputTuples.root`.
* `tree_name`: Name of the tree inside the output ROOT file. Default value is `tree`.

### Usage
```toml
[TreeWriterDUT]
file_name = "myOutputFile.root"
tree_name = "myTree"
```
