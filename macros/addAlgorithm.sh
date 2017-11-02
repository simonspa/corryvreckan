#!/bin/bash

# Ask for the name of the new algorithm
read -p "Name of the new algorithm: " NEWALGORITHM

# Copy the GenericAlgorithm and replace the name with the new algorithm name
echo "Creating ${NEWALGORITHM}.cpp"
cp ../src/algorithms/GenericAlgorithm.cpp ../src/algorithms/${NEWALGORITHM}.cpp
echo "Creating ${NEWALGORITHM}.h"
cp ../src/algorithms/GenericAlgorithm.h ../src/algorithms/${NEWALGORITHM}.h

# Check mac or linux platform
platform=`uname`
if [ "$platform" == "Darwin" ]
then 
  sed -i "" s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}.cpp
  sed -i "" s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}.h
else
  sed -i s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}.cpp
  sed -i s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}.h
fi
