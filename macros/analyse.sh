#!/bin/bash
CORRY=/home/telescope/corryvreckan
RUN=${1}
ALIGNMENT=27834

# Set up the arguments to be passed to Corryvreckan
CONDFILE=${CORRY}/cond/Alignment${ALIGNMENT}.dat
HISTOGRAMFILE=${CORRY}/macros/histograms.root
INPUTDIR_TPX=/data/tbSeptember2017/data/Run${RUN}
INPUTDIR_CPX=/data/tbSeptember2017/clicpix2/Run${RUN}

${CORRY}/bin/corry -c telescope.conf \
                   -o detectors_file=${CONDFILE} \
		   -o histogramFile=${HISTOGRAMFILE} \
                   -o Timepix3EventLoader.inputDirectory=${INPUTDIR_TPX} \
                   -o Clicpix2EventLoader.inputDirectory=${INPUTDIR_CPX}

