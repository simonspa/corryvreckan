#!/bin/bash

# Check that only one argument was given
ARGS=1
if [ $# -ne "$ARGS" ]
then
  echo "Please enter the new algorithm name"
  exit
fi

# Copy the GenericAlgorithm and replace the name with the new algorithm name
NEWALGORITHM=$1
cp ../algorithms/GenericAlgorithm.C ../algorithms/${NEWALGORITHM}.C
cp ../algorithms/GenericAlgorithm.h ../algorithms/${NEWALGORITHM}.h
# If running on mac, use this sed command
sed -i "" s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../algorithms/${NEWALGORITHM}.C
sed -i "" s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../algorithms/${NEWALGORITHM}.h
# If running on linux, use this command
#sed -i s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../algorithms/${NEWALGORITHM}.C
#sed -i s/"GenericAlgorithm"/"${NEWALGORITHM}"/g ../algorithms/${NEWALGORITHM}.C
 
