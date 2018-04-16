#!/bin/bash

# Ask for the name of the new module
read -p "Name of the new module: " NEWMODULE

# Copy the GenericAlgorithm and replace the name with the new module name
echo "Creating folder src/modules/${NEWMODULE}"
mkdir ../src/modules/${NEWMODULE}
echo "Creating ${NEWMODULE}.cpp"
cp ../src/modules/GenericAlgorithm/GenericAlgorithm.cpp ../src/modules/${NEWMODULE}/${NEWMODULE}.cpp
echo "Creating ${NEWMODULE}.h"
cp ../src/modules/GenericAlgorithm/GenericAlgorithm.h ../src/modules/${NEWMODULE}/${NEWMODULE}.h
echo "Creating supplimentory files"
cp ../src/modules/GenericAlgorithm/CMakeLists.txt ../src/modules/${NEWMODULE}/CMakeLists.txt
cp ../src/modules/README_template.md ../src/modules/${NEWMODULE}/README.md

# Check mac or linux platform
platform=`uname`
if [ "$platform" == "Darwin" ]
then
  sed -i "" s/"GenericAlgorithm"/"${NEWMODULE}"/g ../src/modules/${NEWMODULE}/${NEWMODULE}.cpp
  sed -i "" s/"GenericAlgorithm"/"${NEWMODULE}"/g ../src/modules/${NEWMODULE}/${NEWMODULE}.h
  sed -i "" s/"GenericAlgorithm"/"${NEWMODULE}"/g ../src/modules/${NEWMODULE}/CMakeLists.txt
else
  sed -i s/"GenericAlgorithm"/"${NEWMODULE}"/g ../src/modules/${NEWMODULE}/${NEWMODULE}.cpp
  sed -i s/"GenericAlgorithm"/"${NEWMODULE}"/g ../src/modules/${NEWMODULE}/${NEWMODULE}.h
  sed -i s/"GenericAlgorithm"/"${NEWMODULE}"/g ../src/modules/${NEWMODULE}/CMakeLists.txt
fi
