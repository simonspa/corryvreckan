#!/bin/bash

ROOT=/home/dhynds/tbcode
RUN=$1
ALIRUN=$2
NEVENTS=30000

if [ $# -ne 2 ]
then
	ALIRUN=${RUN}
        echo - No alignment run specified, will use alignment for run ${ALIRUN}
	if [ ! -e ${ROOT}/cond/Alignment${RUN}.dat ]
	then
		echo - This run has not been aligned, will create defaut alignment file
		cp ${ROOT}/cond/Alignment.dat ${ROOT}/cond/Alignment${RUN}.dat  
	fi
fi
 
${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${ALIRUN}.dat -h ${ROOT}/monitoring/histograms${RUN}_a.root -M ${ROOT}/masks/maskedPixels10894.dat -u 0.2 -x 0.1 -y 0.1 -W ${ROOT}/masks/trackingMask10891.dat -n ${NEVENTS} -s $((0*$NEVENTS)) &> ${ROOT}/monitoring/log${RUN}_a.txt &

${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${ALIRUN}.dat -h ${ROOT}/monitoring/histograms${RUN}_b.root -M ${ROOT}/masks/maskedPixels10894.dat -u 0.2 -x 0.1 -y 0.1 -W ${ROOT}/masks/trackingMask10891.dat -n ${NEVENTS} -s $((1*$NEVENTS)) &> ${ROOT}/monitoring/log${RUN}_b.txt &

${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${ALIRUN}.dat -h ${ROOT}/monitoring/histograms${RUN}_c.root -M ${ROOT}/masks/maskedPixels10894.dat -u 0.2 -x 0.1 -y 0.1 -W ${ROOT}/masks/trackingMask10891.dat -n ${NEVENTS} -s $((2*$NEVENTS)) &> ${ROOT}/monitoring/log${RUN}_c.txt &

${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${ALIRUN}.dat -h ${ROOT}/monitoring/histograms${RUN}_d.root -M ${ROOT}/masks/maskedPixels10894.dat -u 0.2 -x 0.1 -y 0.1 -W ${ROOT}/masks/trackingMask10891.dat -n ${NEVENTS} -s $((3*$NEVENTS)) &> ${ROOT}/monitoring/log${RUN}_d.txt &

