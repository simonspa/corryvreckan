#!/bin/bash

ROOT=/home/dhynds/tbcode
RUN=$1
ALIRUN=$2

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
 
${ROOT}/bin/tpanal -z ${ROOT}/rootDataLocal/Run${RUN}.root -c ${ROOT}/cond/Alignment${ALIRUN}.dat -h ${ROOT}/monitoring/histograms${RUN}.root -M ${ROOT}/masks/maskedPixels11173.dat -u 0.2 -x 0.1 -y 0.1 -W ${ROOT}/masks/trackingMask11260.dat &> ${ROOT}/monitoring/log${RUN}.txt
