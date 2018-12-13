ABSOLUTE_PATH="$( cd "$( dirname "${BASH_SOURCE}" )" && pwd )"

# Load dependencies if run by the CI
# FIXME: This is needed because of broken RPATH on Mac
if [ -n "${CI}" ]; then
    if [ "$(uname)" == "Darwin" ]; then
        source $ABSOLUTE_PATH/../.gitlab-ci.d/init_mac.sh
    else
        source $ABSOLUTE_PATH/../.gitlab-ci.d/init_x86_64.sh
    fi
    source $ABSOLUTE_PATH/../.gitlab-ci.d/load_deps.sh
fi

# First argument is the data set to be used, ask for the download:
python download_data.py $1
# Second argument is the full test command to be executed:
exec $2
