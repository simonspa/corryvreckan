#!/bin/bash
CORRY=/home/telescope/corryvreckan
RUN=${1}

# Set up the arguments to be passed to Corryvreckan
GEOMETRY=${CORRY}/cond/Alignment_April2018_1.conf
HISTOGRAMFILE=${CORRY}/macros/histograms_${RUN}.root
INPUTDIR_TPX=/data/tbApril2018/data/Run${RUN}
INPUTDIR_CPX=/data/tbApril2018/clicpix2/Run${RUN}

${CORRY}/bin/corry -c telescope.conf \
                   -o detectors_file=${GEOMETRY} \
		   -o histogramFile=${HISTOGRAMFILE} \
                   -o Timepix3EventLoader.inputDirectory=${INPUTDIR_TPX} \
                   -o Clicpix2EventLoader.inputDirectory=${INPUTDIR_CPX} \
                   -o OnlineMonitor.canvasTitle="\"CLICdp Online Monitor - Run ${1}\""

