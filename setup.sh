#!/bin/bash

export LCGEXTERNAL=/afs/cern.ch/sw/lcg/external
export LCGRELEASES=/afs/cern.ch/sw/lcg/app/releases

#--------------------------------------------------------------------------------
# CMAKE
#--------------------------------------------------------------------------------
export CMAKE_HOME=$LCGEXTERNAL/CMake/2.8.8/x86_64-slc6-gcc46-opt
export PATH=${CMAKE_HOME}/bin:$PATH


#--------------------------------------------------------------------------------
#     GCC
#--------------------------------------------------------------------------------

gcc_config_version=4.8.1
mpfr_config_version=3.1.2
gmp_config_version=5.1.1
LCGPLAT=x86_64-slc6-gcc48-opt
LCG_lib_name=lib64
LCG_arch=x86_64

if [ -z $1 ] 
then LCG_contdir=/afs/cern.ch/sw/lcg/contrib
else LCG_contdir=$1 
fi
LCG_gcc_home=${LCG_contdir}/gcc/${gcc_config_version}/${LCGPLAT}
LCG_mpfr_home=${LCG_contdir}/mpfr/${mpfr_config_version}/${LCGPLAT}
LCG_gmp_home=${LCG_contdir}/gmp/${gmp_config_version}/${LCGPLAT}

export PATH=${LCG_gcc_home}/bin:${PATH}
export COMPILER_PATH=${LCG_gcc_home}/lib/gcc/${LCG_arch}-unknown-linux-gnu/${gcc_config_version}

if [ ${LD_LIBRARY_PATH} ] 
then
export LD_LIBRARY_PATH=${LCG_gcc_home}/${LCG_lib_name}:${LCG_mpfr_home}/lib:${LCG_gmp_home}/lib:${LD_LIBRARY_PATH}
else
export LD_LIBRARY_PATH=${LCG_gcc_home}/${LCG_lib_name}:${LCG_mpfr_home}/lib:${LCG_gmp_home}/lib
fi



### ROOT
source $LCGRELEASES/ROOT/5.34.10/x86_64-slc6-gcc48-dbg/root/bin/thisroot.sh

#export BOOSTROOT=/home/dhynds/
