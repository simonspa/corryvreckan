# Scripts

## addModule.sh

Short script to prepare the source code and CMake files for a new allpix<sup>2</sup> module.
The tool will ask for all necessary information, such as the module name ("Module" will be appended automatically) and the type (global, detector,dut).
Both unique and detector-specific modules are supported.

A README.md file is automatically created, containing the author and contact information of the creator. The information is retrieved using the `git config user.name` and `git config user.email` commands if available, or `whoami` and `hostname` as fallback.
Please make sure, the information is correct.

Usage:

```
./etc/addModule.sh
```

Example Output:
```
$ ./etc/addModule.sh 

Preparing code basis for a new module:

Name of the module? jensTest
Type of the module?

1) global
2) detector
3) dut
#? 1
Creating directory and files...

Name:   MyNewModule
Author: John Doe (john.doe@cern.ch)
Path:   /path/to/corryvreckan/src/modules/MyNewModule

Re-run CMake in order to build your new module.
```

## setup_lxplus.sh

Script to facilitate the compilation of Corryvreckan on the CERN LXPLUS Linux cluster. Sourcing the script via

```
source etc/setup_lxplus.sh
```

will setup all required build dependencies.
