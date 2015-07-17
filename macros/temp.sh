#!/bin/bash

RUN=$1
ALIRUN=$2

ROOT=/home/dhynds/tbcode

echo ""
echo ===== Starting monitoring of run ${RUN}
# Check if the telecope data needs to been converted to "raw" data
if [ ! -e ${ROOT}/rootDataLocal/Run${RUN}.root ]
then
  echo ===== Converting data
  mkdir -p ${ROOT}/rawDataLocal/Run${RUN}
  ${ROOT}/macros/convert.sh ${RUN}
fi

# Check if the ntuple has already been made
if [ ! -e ${ROOT}/rootDataLocal/Run${RUN}.root ]
then
  echo ===== Amalgamating data into ntuple
  . ${ROOT}/macros/amalgamate.sh ${RUN}
fi

# Analyse the run
echo ===== Running the reconstruction
. ${ROOT}/macros/analyse.sh ${RUN} ${ALIRUN}
# Launch the viewer
#echo ===== Starting the viewer
#. ${ROOT}/macros/viewdata.sh ${RUN} ${ALIRUN}
