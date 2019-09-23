ABSOLUTE_PATH="$( cd "$( dirname "${BASH_SOURCE}" )" && pwd )"

# First argument is the data set to be used, ask for the download:
python download_data.py $1

# Second argument is the full test command to be executed:
exec $2
