#!/bin/bash

ROOT=/home/dhynds/tbcode/monitoring
RUN=$1

hadd -f ${ROOT}/histograms${RUN}.root ${ROOT}/histograms${RUN}_a.root ${ROOT}/histograms${RUN}_b.root ${ROOT}/histograms${RUN}_c.root ${ROOT}/histograms${RUN}_d.root
