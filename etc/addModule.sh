#!/bin/bash

echo -e "\nPreparing code basis for a new module:\n"

# Ask for module name:
read -p "Name of the module? " MODNAME

echo "Creating directory and files..."

echo
# Try to find the modules directory:
BASEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd -P )"
DIRECTORIES[0]="${BASEDIR}/../../src/modules"
DIRECTORIES[1]="../src/modules"
DIRECTORIES[2]="src/modules"

MODDIR=""
for DIR in "${DIRECTORIES[@]}"; do
    if [ -d "$DIR" ]; then
	MODDIR="$DIR"
	break
    fi
done

# Create directory
mkdir "$MODDIR/$MODNAME"

# Copy over CMake file and sources from Dummy:
sed -e "s/Dummy/$MODNAME/g" $MODDIR/Dummy/CMakeLists.txt > $MODDIR/$MODNAME/CMakeLists.txt

# Copy over the README, setting current git username/email as author
# If this fails, use system username and hostname
MYNAME=$(git config user.name)
MYMAIL=$(git config user.email)
if [ -z "$MYNAME" ]; then
    MYNAME=$(whoami)
fi
if [ -z "$MYMAIL" ]; then
    MYMAIL=$(hostname)
fi
sed -e "s/Dummy/$MODNAME/g" \
    -e "s/\*NAME\*/$MYNAME/g" \
    -e "s/\*EMAIL\*/$MYMAIL/g" \
    -e "s/Functional/Immature/g" \
    "$MODDIR/Dummy/README.md" > "$MODDIR/$MODNAME/README.md"

# Copy over source code skeleton:
sed -e "s/Dummy/$MODNAME/g" "$MODDIR/Dummy/Dummy.h" > "$MODDIR/$MODNAME/${MODNAME}.h"
sed -e "s/Dummy/$MODNAME/g" "$MODDIR/Dummy/Dummy.cpp" > "$MODDIR/$MODNAME/${MODNAME}.cpp"

# Options for sed vary slightly between mac and linux
opt=-i
platform=`uname`
if [ "$platform" == "Darwin" ]; then opt="-i \"\""; fi

# Print a summary of the module created:
FINALPATH=`realpath $MODDIR/$MODNAME`
echo "Name:   $MODNAME"
echo "Author: $MYNAME ($MYMAIL)"
echo "Path:   $FINALPATH"
echo
echo "Re-run CMake in order to build your new module."
