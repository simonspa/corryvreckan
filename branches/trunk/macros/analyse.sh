#!/bin/bash

TBCODE=/Users/danielhynds/work/hvcmos/newTBcode
RUN=${1}

DATADIR=/Users/danielhynds/work/hvcmos/newTBcode/tpx3data/Run${RUN}
CONDFILE=${TBCODE}/cond/cond.dat
HISTOFILE=${TBCODE}/histos/newHistogramsRun${RUN}.root
NEVENTS=20

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -a
