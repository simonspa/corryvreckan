#!/bin/bash
CORRY=/home/telescope/corryvreckan
RUN=${1}
ALIGNMENT=28300

# Set up the arguments to be passed to Corryvreckan
CONDFILE=${CORRY}/cond/Alignment${ALIGNMENT}_aligning.conf
HISTOGRAMFILE=${CORRY}/macros/histograms.root
INPUTDIR_TPX=/data/tbApril2018/data/Run${RUN}
INPUTDIR_CPX=/data/tbApril2018/clicpix2/Run${RUN}

${CORRY}/bin/corry -c telescope.conf \
                   -o histogramFile = "histograms-${1}.root" \
                   -o detectors_file=${CONDFILE} \
		   -o histogramFile=${HISTOGRAMFILE} \
                   -o Timepix3EventLoader.inputDirectory=${INPUTDIR_TPX} \
                   -o Clicpix2EventLoader.inputDirectory=${INPUTDIR_CPX} \
                   ###-o OnlineMonitor.canvasTitle="\"CLICdp Online Monitor - Run ${1}\""


