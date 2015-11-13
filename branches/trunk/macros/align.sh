#!/bin/bash

TBCODE=..
RUN=1100
 
DATADIR=${TBCODE}/example/Run${RUN}
CONDFILE=${TBCODE}/cond/alignmentRun${RUN}.dat
HISTOFILE=${TBCODE}/histos/histogramsRun${RUN}.root
NEVENTS=-1
EVENTTIME=0.00005
STARTTIME=0

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -t ${EVENTTIME} -o ${STARTTIME} -a 0
mv alignmentOutput.dat ${CONDFILE}

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -t ${EVENTTIME} -o ${STARTTIME} -a 0
mv alignmentOutput.dat ${CONDFILE}

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -t ${EVENTTIME} -o ${STARTTIME} -a 1
mv alignmentOutput.dat ${CONDFILE}

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -t ${EVENTTIME} -o ${STARTTIME} -a 1
mv alignmentOutput.dat ${CONDFILE}
