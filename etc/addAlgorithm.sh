#!/bin/bash

# Ask for the name of the new algorithm
read -p "Name of the new algorithm: " NEWALGORITHM

# Copy the GenericAlgorithm and replace the name with the new algorithm name
echo "Creating folder src/algorithms/${NEWALGORITHM}"
mkdir ../src/algorithms/${NEWALGORITHM}
echo "Creating ${NEWALGORITHM}.cpp"
cp ../src/algorithms/GenericAlgorithm/GenericAlgorithm.cpp ../src/algorithms/${NEWALGORITHM}/${NEWALGORITHM}.cpp
echo "Creating ${NEWALGORITHM}.h"
cp ../src/algorithms/GenericAlgorithm/GenericAlgorithm.h ../src/algorithms/${NEWALGORITHM}/${NEWALGORITHM}.h
echo "Creating supplimentory files"
cp ../src/algorithms/GenericAlgorithm/CMakeLists.txt ../src/algorithms/${NEWALGORITHM}/CMakeLists.txt
cp ../src/algorithms/README_template.md ../src/algorithms/${NEWALGORITHM}/README.md

# Check mac or linux platform
platform=`uname`
if [ "$platform" == "Darwin" ]
then
  sed -i "" s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}/${NEWALGORITHM}.cpp
  sed -i "" s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}/${NEWALGORITHM}.h
  sed -i "" s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}/CMakeLists.txt
else
  sed -i s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}/${NEWALGORITHM}.cpp
  sed -i s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}/${NEWALGORITHM}.h
  sed -i s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../src/algorithms/${NEWALGORITHM}/CMakeLists.txt
fi
