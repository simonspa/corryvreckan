#!/bin/bash

ROOT=/home/dhynds/tbcode
RUN=$1
ALIRUN=$2

if [ $# -ne 2 ]
then
        ALIRUN=${RUN}
        if [ ! -e ${ROOT}/cond/Alignment${RUN}.dat ]
        then
                echo cp ${ROOT}/cond/Alignment.dat ${ROOT}/cond/Alignment${RUN}.dat  
        fi
fi

# make a temporary directory for the output
mkdir -p $PWD/cond
# align with progressively smaller windows and move the output alignment file

#${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${RUN}.dat -h ${ROOT}/monitoring/alignHistograms${RUN}.root -n 1000 -u 0.3 -a -l 0 -x 0.3 -y 0.3
#mv $PWD/cond/AlignmentOutput_un${RUN}.root.dat ${ROOT}/cond/Alignment${RUN}.dat
#${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${RUN}.dat -h ${ROOT}/monitoring/alignHistograms${RUN}.root -n 3000 -u 0.2 -a -l 0 -x 0.2 -y 0.2
#mv $PWD/cond/AlignmentOutput_un${RUN}.root.dat ${ROOT}/cond/Alignment${RUN}.dat
${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${ALIRUN}.dat -h ${ROOT}/monitoring/alignHistograms${RUN}.root -n 5000 -u 0.2 -a -l 0 -x 0.1 -y 0.1 -W ${ROOT}/masks/trackingMaskOpen.dat -M ${ROOT}/masks/maskedPixels11044.dat
mv $PWD/cond/AlignmentOutput_un${RUN}.root.dat ${ROOT}/cond/Alignment${RUN}.dat
${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${ALIRUN}.dat -h ${ROOT}/monitoring/alignHistograms${RUN}.root -n 5000 -u 0.2 -a -l 0 -x 0.05 -y 0.05 -W ${ROOT}/masks/trackingMaskOpen.dat -M ${ROOT}/masks/maskedPixels11044.dat
mv $PWD/cond/AlignmentOutput_un${RUN}.root.dat ${ROOT}/cond/Alignment${RUN}.dat
rmdir $PWD/cond
