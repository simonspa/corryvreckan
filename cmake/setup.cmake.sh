Corryvreckan_version="@CORRYVRECKAN_VERSION@"

# Dependencies
@SETUP_FILE_DEPS@

THIS_BASH=$BASH
if [[ $THIS_BASH == "" ]]; then
    THIS_HERE=$(dirname $0)
else
    THIS_HERE=$(dirname ${BASH_SOURCE[0]})
fi

CLIC_Corryvreckan_home=$THIS_HERE

# Export path for executable, library path and data path for detector models
export PATH=${CLIC_Corryvreckan_home}/bin:${PATH}
export LD_LIBRARY_PATH=${CLIC_Corryvreckan_home}/lib:${LD_LIBRARY_PATH}
export XDG_DATA_DIRS=${CLIC_Corryvreckan_home}/share:${XDG_DATA_DIRS}
