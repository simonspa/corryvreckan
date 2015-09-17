#!/bin/bash

ROOT=/home/dhynds/tbcode
RUN=$1

${ROOT}/bin/tpanal -r ${ROOT}/rawDataLocal/Run${RUN} -z ${ROOT}/rootDataLocal/Run${RUN}.root -q #&> /dev/null

if [ -e ${ROOT}/rootDataLocal/Run${RUN}.root ]
then
  rm -rf ${ROOT}/rawDataLocal/Run${RUN}
fi
