#!/bin/bash

# Where the code is and which run to analyse
TBCODE=..
RUN=${1}
 
# Set up the arguments to be passed to the testbeam code
CONDFILE=${TBCODE}/cond/Alignment${RUN}.dat
HISTOFILE=${TBCODE}/histos/histogramsRun${RUN}test.root
INPUTFILE=${TBCODE}/pixels/pixelsRun${RUN}.root
INPUTFOLDER=${TBCODE}/example/Run${RUN}
EVENTTIME=0.005
NEVENTS=-1

# If the alignment file for this run does not exist, get it from the DB
if [ ! -f ${CONDFILE} ] 
then
  while read run align dut thl angle
  do
    if [ "$run" == "$RUN" ]
    then
      CONDFILE=${TBCODE}/cond/Alignment${align}.dat
      echo New cond file is ${CONDFILE}
    fi
  done <<< "$(grep ${RUN} runListAngledHVCMOS.dat)"
fi

# Launch the testbeam analysis and replace the original alignment file with the new one
${TBCODE}/bin/tbAnalysis -c ${CONDFILE} -n ${NEVENTS} -h ${HISTOFILE} -t ${INPUTFILE} -s ${TBCODE}/masks/maskedPixels11044.dat
#${TBCODE}/bin/tbAnalysis -c ${CONDFILE} -h ${HISTOFILE} -d ${INPUTFOLDER} -p ${EVENTTIME}
