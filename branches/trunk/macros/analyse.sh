#!/bin/bash

TBCODE=..
RUN=$1
 
DATADIR=/data/tpx3test/data/Run${RUN}
CONDFILE=alignmentSaved.dat
#CONDFILE=../cond/alignmentRun880.dat
HISTOFILE=${TBCODE}/histos/histogramsRun${RUN}.root
NEVENTS=-1
EVENTTIME=0.00005
STARTTIME=18.0

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -t ${EVENTTIME} -o ${STARTTIME}
 
