#!/bin/bash

VERTEX=/home/vertextb
ROOT=/home/dhynds/tbcode
RUN=$1

if [ -e ${VERTEX}/eudaq/data/run0${RUN}.raw ]
then
  echo - Found input data file run0${RUN}.raw
  echo - Writing output
fi

mkdir -p ${ROOT}/rawDataLocal/Run${RUN}
${VERTEX}/eudaqSG/bin/MonitorEventsSG.exe ${VERTEX}/eudaq/data/run0${RUN}.raw -e 100000 -g 10 -p ${ROOT}/rawDataLocal/Run${RUN} &> /dev/null

