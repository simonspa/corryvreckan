#!/bin/bash

TBCODE=..
RUN=$1
 
DATADIR=../example/Run${RUN}
#CONDFILE=alignmentOutput.dat
CONDFILE=../cond/alignmentRun${RUN}.dat
HISTOFILE=${TBCODE}/histos/histogramsRun${RUN}.root
NEVENTS=-1
EVENTTIME=0.00005
STARTTIME=1.2

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -t ${EVENTTIME} -o ${STARTTIME} -a 1
 
