#!/bin/bash

TBCODE=..
 
DATADIR=${1}
CONDFILE=${TBCODE}/cond/alignmentLab.dat
HISTOFILE=${TBCODE}/histos/histogramsLabTest${2}.root
NEVENTS=-1
EVENTTIME=0.00005
STARTTIME=0

${TBCODE}/bin/tbAnalysis -d ${DATADIR} -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -t ${EVENTTIME} -o ${STARTTIME}

