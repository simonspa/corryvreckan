if [ ! -d "/cvmfs/clicdp.cern.ch" ]; then
    echo "CVMFS not available"
    return
fi

# Get our directory and load the CI init
ABSOLUTE_PATH=`dirname $(readlink -f ${BASH_SOURCE[0]})`

# Load default configuration
source $ABSOLUTE_PATH/../.gitlab-ci.d/init_x86_64.sh
