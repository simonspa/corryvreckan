#!/bin/bash

ROOT=/home/dhynds/tbcode
RUN=$1
ALIRUN=$2

if [ $# -ne 2 ]
then
  ALIRUN=${RUN}
fi

${ROOT}/bin/tpmon -z ${ROOT}/rootDataLocal/Run${RUN}.root -h ${ROOT}/monitoring/histograms${RUN}.root -c ${ROOT}/cond/Alignment${ALIRUN}.dat

