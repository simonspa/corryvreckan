#!/bin/bash

if [ "$(uname)" == "Darwin" ]; then
    if [ $(sw_vers -productVersion | awk -F '.' '{print $1 "." $2}') == "10.12" ]; then
        OS=mac1012
        COMPILER_TYPE=llvm
        COMPILER_VERSION=clang80
    else
        echo "Bootstrap only works on macOS Sierra (10.12)"
        exit 1
    fi
else
    echo "This script is only meant for Mac"
    exit 1
fi

# Determine is you have CVMFS installed
if [ ! -d "/cvmfs" ]; then
    echo "No CVMFS detected, please install it."
    exit 1
fi

if [ ! -d "/cvmfs/clicdp.cern.ch" ]; then
    echo "No clicdp CVMFS repository detected, please add it."
    exit 1
fi

# Choose build type
if [ -z ${BUILD_TYPE} ]; then
    BUILD_TYPE=opt
fi


# General variables
CLICREPO=/cvmfs/clicdp.cern.ch
BUILD_FLAVOUR=x86_64-${OS}-${COMPILER_VERSION}-${BUILD_TYPE}

#--------------------------------------------------------------------------------
#     CMake
#--------------------------------------------------------------------------------

export CMAKE_HOME=${CLICREPO}/software/CMake/3.6.2/${BUILD_FLAVOUR}
export PATH=${CMAKE_HOME}/bin:$PATH

#--------------------------------------------------------------------------------
#     ROOT
#--------------------------------------------------------------------------------

export ROOTSYS=${CLICREPO}/software/ROOT/6.08.00/${BUILD_FLAVOUR}
export PYTHONPATH="$ROOTSYS/lib:$PYTHONPATH"
export PATH="$ROOTSYS/bin:$PATH"
export DYLD_LIBRARY_PATH="$ROOTSYS/lib:$DYLD_LIBRARY_PATH"

#--------------------------------------------------------------------------------
#     Ninja
#--------------------------------------------------------------------------------

export Ninja_HOME=${CLICREPO}/software/Ninja/1.7.1/${BUILD_FLAVOUR}
export PATH="$Ninja_HOME:$PATH"
