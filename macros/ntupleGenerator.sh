#!/bin/bash

ROOT=/afs/cern.ch/eng/clic/work/clicpix
EOSROOT=eos/clicdp/data/VertexTB/RawData/CERN_TB_DATA_May2015

RUN=${1}
RAWDATA=run0${RUN}.raw

# Get the data from EOS
mkdir -p Run${RUN}
xrdcp root://eospublic//${EOSROOT}/${RAWDATA} ${RAWDATA}

# Convert the data from the eudaq raw format
${ROOT}/../eudaq/bin/MonitorEventsSG.exe ${RAWDATA} -e 1000000 -g 10 -p Run${RUN} &> /dev/null
rm ${RAWDATA}

# Produce the ntuple of pixels 
${ROOT}/tbAnalysis/branches/trunk/bin/tbAnalysis -d Run${RUN} -c ../cond/AlignmentAngle0.dat -f pixelsRun${RUN}.root
rm -rf Run${RUN}
