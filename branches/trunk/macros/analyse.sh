#!/bin/bash

TBCODE=..
RUN=25

DATADIR=${TBCODE}/example/Run${RUN}
CONDFILE=${TBCODE}/cond/cond.dat
HISTOFILE=${TBCODE}/histos/histogramsRun${RUN}.root
NEVENTS=20

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -g
