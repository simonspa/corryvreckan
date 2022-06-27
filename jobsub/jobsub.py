#!/usr/bin/env python3
"""
jobsub: a tool for Corryvreckan job submission

Steering files are generated based on steering templates by substituting
job and run specific variables. The variable information can be
provided to jobsub by command line argument, by a config file or by a
text file with run parameters (in comma-separated value/csv format).

Run
python jobsub.py --help
to see the list of command line options.

"""
import sys
import logging

def parseIntegerString(inputstr=""):
    """
    return a list of selected values when a string in the form:
    1-4,6
    would return:
    1,2,3,4,6
    as expected...
    (from http://thoughtsbyclayg.blogspot.de/2008/10/parsing-list-of-numbers-in-python.html)

    Modified such that it returns a list of strings
    if the conversion to integer fails, e.g.
    "10ns, 20ns"
    would return:
    "10ns", "20ns"
    """
    selection = list()
    # tokens are comma separated values
    tokens = [substring.strip() for substring in inputstr.split(',')]
    for i in tokens:
        try:
            # typically tokens are plain old integers
            selection.append(int(i))
        except ValueError:
            try:
                # if not, then it might be a range
                token = [int(k.strip()) for k in i.split('-')]
                if len(token) > 1:
                    token.sort()
                    # we have items separated by a dash
                    # try to build a valid range
                    first = token[0]
                    last = token[len(token)-1]
                    for value in range(first, last+1):
                        selection.append(value)
            except ValueError:
                # if not treat as string, not integer
                selection.append(i)
    return selection # end parseIntegerString

def ireplace(old, new, text):
    """
    case insensitive search and replace function searching through string and returning the filtered string
    (based on http://stackoverflow.com/a/4773614)

    """
    idx = 0
    occur = 0
    while idx < len(text):
        index_l = text.lower().find(old.lower(), idx)
        if index_l == -1:
            if occur == 0:
                raise EOFError("Could not find string "+old)
            return text
        text = text[:index_l] + new + text[index_l + len(old):]
        idx = index_l + len(new)
        occur = occur+1
    if occur == 0:
        raise EOFError("Could not find string "+old)
    return text


def loadparamsfromcsv(csvfilename, runs):
    """ Load and parse the csv file for the given set of runs and
    return nested dictionary: a collection of dictionaries, one for
    each csv row matching a run number.

    """
    import csv
    import os.path
    from sys import exit # use sys.exit instead of built-in exit (latter raises exception)

    class CommentedFile:
        """ Decorator for text files: filters out comments (i.e. first char of line #)
        Based on http://www.mfasold.net/blog/2010/02/python-recipe-read-csvtsv-textfiles-and-ignore-comment-lines/

        """
        def __init__(self, f, commentstring="#"):
            self.f = f
            self.commentstring = commentstring
            self.linecount = 0
        def rewind(self):
            self.f.seek(0)
            self.linecount = 0
        def __next__(self):
            line = self.f.__next__()
            self.linecount += 1
            while line.startswith(self.commentstring) or not line.strip(): # test if line commented or empty
                line = self.f.__next__()
                self.linecount += 1
            return str(line)
        def __iter__(self):
            return self

    log = logging.getLogger('jobsub')
    parameters_csv = {} # store all information needed from the csv file
    if csvfilename is None:
        return parameters_csv # if no file name given, return empty collection here
    if not os.path.isfile(csvfilename): # check if file exists
        log.error("Could not find the specified csv file '"+csvfilename+"'!")
        exit(1)
    try:
        log.debug("Opening csv file '"+csvfilename+"'.")
        csvfile = open(csvfilename, 'rt')
        filteredfile = CommentedFile(csvfile)

        try:
            # construct a sample for the csv format sniffer:
            sample = ""
            try:
                while (len(sample)<1024):
                    sample += filteredfile.__next__()
            except StopIteration:
                log.debug("End of csv file reached, sample limited to " + str(len(sample))+ " bytes")
            dialect = csv.Sniffer().sniff(sample) # test csv file format details
            dialect.escapechar = "\\"
            log.debug("Determined the CSV dialect as follows: delimiter=%s, doublequote=%s, escapechar=%s, lineterminator=%s, quotechar=%s , quoting=%s, skipinitialspace=%s", dialect.delimiter, dialect.doublequote, dialect.escapechar, list(ord(c) for c in dialect.lineterminator), dialect.quotechar, dialect.quoting, dialect.skipinitialspace)
            filteredfile.rewind() # back to beginning of file
            reader = csv.DictReader(filteredfile, dialect=dialect) # now process CSV file contents here and load them into memory
            reader.__next__() # python requires an actual read access before filling 'DictReader.fieldnames'

            log.debug("CSV file contains the header info: %s", reader.fieldnames)
            try:
                reader.fieldnames = [field.lower() for field in reader.fieldnames] # convert to lower case keys to avoid confusion
                reader.fieldnames = [field.strip() for field in reader.fieldnames] # remove leading and trailing white space
            except TypeError:
                log.error("Could not process the CSV file header information. csv.DictReader returned fieldnames: %s", reader.fieldnames)
                exit(1)
            if not "runnumber" in reader.fieldnames: # verify that we have a column "runnumber"
                log.error("Could not find a column with header label 'RunNumber' in file '"+csvfilename+"'!")
                exit(1)
            if "" in reader.fieldnames:
                log.warning("Column without header label encountered in csv file '"+csvfilename+"'!")
            log.info("Successfully loaded csv file'"+csvfilename+"'.")
            # first: search through csv file to find corresponding runnumber entry line for every run
            filteredfile.rewind() # back to beginning of file
            reader.__next__()   # .. and skip the header line
            missingRuns = list(runs) # list of runs to look for in csv file

            cnt = 0
            for row in reader: # loop over all rows once
                try:
                    cnt += 1
                    for run in missingRuns: # check all runs if runnumber matches
                        if int(row["runnumber"]) == run:
                            log.debug("Found entry in csv file for run "+str(run)+" on line "+ str(filteredfile.linecount))
                            missingRuns.remove(run)
                    parameters_csv[cnt-1] = {} # start counting at 0
                    parameters_csv[cnt-1].update(row) # start counting at 0
                except ValueError: # int conversion error
                    log.warning("Could not interpret run number on line "+str(filteredfile.linecount)+" in file '"+csvfilename+"'.")
                    continue
            if len(missingRuns)==0:
                log.debug("Found at least one line for each run we were searching for.")

            log.debug("Searched over "+str(filteredfile.linecount)+" lines in file '"+csvfilename+"'.")
            if not len(missingRuns)==0:
                log.error("Could not find an entry for the following run numbers in '"+csvfilename+"': "+', '.join(map(str, missingRuns)))
        finally:
            csvfile.close()
    except csv.Error as e:
        log.error("Problem loading the csv file '"+csvfilename+"': %s"%e)
        exit(1)
    return parameters_csv

def checkSteer(sstring):
    """ Check string for any occurrence of @.*@ and return boolean. """
    log = logging.getLogger('jobsub')
    import re
    hits = re.findall("@.*@", sstring)
    if hits:
        log.error ("Missing configuration parameters: "+', '.join(map(str, hits)))
        return False
    else:
        return True

def check_program(name):
    """ Searches PATH environment variable for executable given by parameter """
    import os
    for dir in os.environ['PATH'].split(os.pathsep):
        prog = os.path.join(dir, name)
        if os.path.exists(prog): return prog

def runCorryvreckan(filenamebase, jobtask, silent):
    """ Runs Corryvreckan and stores log of output """
    from sys import exit # use sys.exit instead of built-in exit (latter raises exception)
    log = logging.getLogger('jobsub.' + jobtask)

    # check for Corryvreckan executable
    cmd = check_program("corry")
    if cmd:
        log.debug("Found Corryvreckan executable: " + cmd)
    else:
        log.error("Corryvreckan executable not found in PATH!")
        log.error(os.getcwd())
        exit(1)

    # search for stdbuf command: adjust stdout buffering
    stdbuf = check_program("stdbuf")
    if stdbuf:
        log.debug("Found stdbuf, will use line buffered output.")
        # -oL: adjust standard output stream buffering to line buffered
        cmd = stdbuf + " -oL " + cmd

    # need some additional libraries for process interaction
    import asyncio
    import os
    import sys
    from asyncio.subprocess import PIPE

    import datetime
    import shlex

    # Based on the solution proposed here:
    # https://gitlab.cern.ch/corryvreckan/corryvreckan/-/issues/146#note_4465941
    async def read_stream_and_display(stream, display):
        """Read from stream line by line until EOF, display, and capture the lines."""
        while True:
            line = await stream.readline()
            if not line:
                break

            if not silent:
                if b'WARNING' in line.strip():
                    log.warning(line.strip())
                elif b'ERROR' in line.strip():
                    log.error(line.strip())
                elif b'FATAL' in line.strip():
                    log.critical(line.strip())
                else:
                    log.info(line.strip())
            log_file.write(str(line,'utf-8'))

    async def read_and_display(cmd):
        """Capture cmd's stdout, stderr while displaying them as they arrive (line by line)."""
        # start process
        log.info("Starting process %s", cmd)
        process = await asyncio.create_subprocess_exec(*shlex.split(cmd), stdout=PIPE, stderr=PIPE)

        # read child's stdout/stderr concurrently (capture and display)
        try:
            # stdout, stderr = await asyncio.gather(
            await asyncio.gather(
                read_stream_and_display(process.stdout, sys.stdout.buffer.write),
                read_stream_and_display(process.stderr, sys.stderr.buffer.write))
        except Exception:
            process.kill()
            raise
        finally:
            # wait for the process to exit
            rc = await process.wait()
        return rc

    cmd = cmd+" -c "+filenamebase+".conf"
    rcode = None # the return code that will be set by a later subprocess method
    try:
        # open log file
        log_file = open(filenamebase+".log", "w")
        # print timestamp to log file
        log_file.write("---=== Analysis started on " + datetime.datetime.now().strftime("%A, %d. %B %Y %I:%M%p") + " ===---\n\n")

        # run process
        loop = asyncio.get_event_loop()
        rcode = loop.run_until_complete(read_and_display(cmd))

        # close log file
        loop.close()
        log_file.close()

    except OSError as e:
        log.critical("Problem with Corryvreckan execution: Command '%s' resulted in error %s", cmd, e)
        exit(1)
    return rcode

def submitCondor(filenamebase, subfile, runnr):
    """ Submits the Corryvreckan job to HTCondor """
    import os
    from sys import exit # use sys.exit instead of built-in exit (latter raises exception)
    log = logging.getLogger('jobsub.' + runnr)
    # We are running on HTCondor.

    # check for qsub executable
    cmd = check_program("condor_submit")
    if cmd:
        log.debug("Found condor_submit executable: " + cmd)
    else:
        log.error("condor_submit executable not found in PATH!")
        exit(1)

    # Add condor_submit parameters:
    cmd = cmd+" -batch-name \"Corry"+runnr+"\" "

    # check for Corryvreckan executable
    corry = check_program("corry")
    if corry:
        log.debug("Found Corryvreckan executable: " + corry)
        cmd = cmd+" executable="+corry
    else:
        log.error("Corryvreckan executable not found in PATH!")
        exit(1)

    cmd = cmd+" arguments=\"-c "+os.path.abspath(filenamebase+".conf")+"\""

    # Add Condor submission configuration file:
    cmd = cmd+" "+subfile

    rcode = None # the return code that will be set by a later subprocess method
    try:
        # run process
        log.info ("Now submitting Corryvreckan job: "+filenamebase+".conf to HTCondor")
        log.debug ("Executing: "+cmd)
        os.popen(cmd)
    except OSError as e:
        log.critical("Problem with HTCondor submission: Command '%s' resulted in error %s", cmd, e)
        exit(1)
    return 0

def zipLogs(path, filename):
    """  stores output from Corryvreckan in zip file; enables compression if necessary module is available """
    import zipfile
    import os.path
    log = logging.getLogger('jobsub')
    try:     # compression module might not be available, therefore try import here
        import zlib
        compression = zipfile.ZIP_DEFLATED
        log.debug("Creating *compressed* log archive")
    except ImportError: # no compression module available, use flat files
        compression = zipfile.ZIP_STORED
        log.debug("Creating flat log archive")
    try:
        zf = zipfile.ZipFile(os.path.join(path, filename)+".zip", mode='w') # create new zip file
        try:
            zf.write(os.path.join("./", filename)+".conf", compress_type=compression) # store in zip file
            zf.write(os.path.join("./", filename)+".log", compress_type=compression) # store in zip file
            os.remove(os.path.join("./", filename)+".conf") # delete file
            os.remove(os.path.join("./", filename)+".log") # delete file
            log.info("Logs written to "+os.path.join(path, filename)+".zip")
        finally:
            log.debug("Closing log archive file")
            zf.close()
    except IOError: # could not create zip file - path non-existent?!
        log.error("Input/Output error: Could not create log and steering file archive ("+os.path.join(path, filename)+".zip"+")!")


def main(argv=None):
    """  main routine of jobsub: a tool for Corryvreckan job submission """
    log = logging.getLogger('jobsub') # set up logging
    formatter = logging.Formatter('%(name)s(%(levelname)s): %(message)s',"%H:%M:%S")
    handler_stream = logging.StreamHandler()
    handler_stream.setFormatter(formatter)
    log.addHandler(handler_stream)
    # using this decorator, we can count the number of error messages
    class callcounted(object):
        """Decorator to determine number of calls for a method"""
        def __init__(self,method):
            self.method=method
            self.counter=0
        def __call__(self,*args,**kwargs):
            self.counter+=1
            return self.method(*args,**kwargs)
    log.error=callcounted(log.error)

    import os.path
    import configparser
    try:
        import argparse
    except ImportError:
        log.debug("No locally installed argparse module found; trying the package provided with jobsub.")
        # argparse is not installed; use (old) version provided with jobsub
        # determine path to subdirectory
        libdir = os.path.join(os.path.dirname(os.path.abspath(os.path.realpath(__file__))),"pymodules","argparse")
        if libdir not in sys.path:
            sys.path.append(libdir)
        # try again loading the module
        try:
            import argparse
        except ImportError:
            # nothing we can do now
            log.critical("Could not load argparse module. For python versions prior to 2.7, please install it from http://code.google.com/p/argparse")
            return 1

    if argv is None:
        argv = sys.argv
        progName = os.path.basename(argv.pop(0))

    # command line argument parsing
    parser = argparse.ArgumentParser(prog=progName, description="A tool for the convenient run-specific modification of Corryvreckan configuration files and their execution through the corry executable")
    parser.add_argument('--version', action='version', version='Revision: $Revision$, $LastChangedDate$')
    parser.add_argument("-c", "--conf-file", "--config", help="Configuration file with all Corryvreckan algorithms defined", metavar="FILE")
    parser.add_argument('--option', '-o', action='append', metavar="NAME=VALUE", help="Specify further options such as 'beamenergy=5.3'. This switch be specified several times for multiple options or can parse a comma-separated list of options. This switch overrides any config file options and also overwrites hard-coded settings on the Corryvreckan configuration file.")
    parser.add_argument("-htc", "--htcondor-file", "--batch", help="Specify condor_submit parameter file for HTCondor submission. Run HTCondor submission via condor_submit instead of calling Corryvreckan directly", metavar="FILE")
    parser.add_argument("-csv", "--csv-file", help="Load additional run-specific variables from table (text file in csv format)", metavar="FILE")
    parser.add_argument("--log-file", help="Save submission log to specified file", metavar="FILE")
    parser.add_argument("-v", "--verbosity", default="info", help="Sets the verbosity of log messages during job submission where LEVEL is either debug, info, warning or error", metavar="LEVEL")
    parser.add_argument("-s", "--silent", action="store_true", default=False, help="Suppress non-error (stdout) Corryvreckan output to console")
    parser.add_argument("--dry-run", action="store_true", default=False, help="Write configuration files but skip actual Corryvreckan execution")
    parser.add_argument("--subdir", action="store_true", default=False, help="Execute every job in its own subdirectory instead of all in the base path")
    parser.add_argument("--plain", action="store_true", default=False, help="Output written to stdout/stderr and log file in prefix-less format i.e. without time stamping")
    parser.add_argument("--zfill", metavar='N', type=int, help="Fill run number with zeros up to the defined number of digits")
    parser.add_argument("runs", help="The runs to be analyzed; can be a list of single runs and/or a range, e.g. 1056-1060.", nargs='*')
    args = parser.parse_args(argv)

    # Try to import the colorer module
    try:
        import Colorer
    except ImportError:
        pass

    # set the logging level
    numeric_level = getattr(logging, "INFO", None) # default: INFO messages and above
    if args.verbosity:
        # Convert log level to upper case to allow the user to specify --log=DEBUG or --log=debug
        numeric_level = getattr(logging, args.verbosity.upper(), None)
        if not isinstance(numeric_level, int):
            log.error('Invalid log level: %s' % args.verbosity)
            return 2
    handler_stream.setLevel(numeric_level)
    log.setLevel(numeric_level)

    if args.plain:
        formatter = logging.Formatter('%(message)s')
        handler_stream.setFormatter(formatter)

    # set up submission log file if requested on command line
    if args.log_file:
        handler_file = logging.FileHandler([args.log_file])
        handler_file.setFormatter(formatter)
        handler_file.setLevel(numeric_level)
        log.addHandler(handler_file)

    log.debug( "Command line arguments used: %s ", args )

    runs = list()
    for runnum in args.runs:
        try:
            log.debug("Parsing run-range argument: '%s'", runnum)
            runs = runs + parseIntegerString(runnum)
        except ValueError:
            log.error("The list of runs contains non-integer and non-range values: '%s'", runnum)
            return 2

    if not runs:
        log.error("No run numbers were specified. Please see '"+progName+" --help' for details.")
        return 2

    if len(runs) > len(set(runs)): # sets items are unique
        log.error("At least one run is specified multiple times!")
        return 2

    # dictionary keeping our parameters
    # here you can set some minimal default config values that will (possibly) be overwritten by the config file
    parameters = {"conf_file":"analysis.conf", "logpath":"."}

    # Parse option part of the  argument here -> overwriting config options
    if args.option is None:
        log.debug("Nothing to parse: No additional config options specified through command line arguments. ")
    else:
        try:
            # now parse any options given through the -o cmd line switch
            cmdoptions = dict(opt.strip().split('=', 1) for optlist in args.option for opt in optlist.split(',')) # args.option is a list of lists of strings we need to split at every '='
        except ValueError:
            log.error( "Command line error: cannot parse --option argument(s). Please use a '--option name=value' format. ")
            return 2
        for key in cmdoptions: # and overwrite our current config settings
            log.debug( "Parsing cmd line: Setting "+key+" to value '"+cmdoptions[key]+"', possibly overwriting corresponding config file option")
            parameters[key.lower()] = cmdoptions[key]

    log.debug( "Our final config:")
    for key, value in parameters.items():
        log.debug ( "     "+key+" = "+value)

    if not os.path.isfile(args.conf_file):
        log.critical("Configuration template '"+args.conf_file+"' not found!")
        return 1

    log.debug( "Opening configuration template "+args.conf_file)
    steeringStringBase = open(args.conf_file, "r").read()

    #Query replace steering template with our parameter set
    log.debug ("Generating base configuration file")
    for key in parameters.keys():
        # check if we actually find all parameters from the config in the steering file
        try:
            steeringStringBase = ireplace("@" + key + "@", parameters[key], steeringStringBase)
        except EOFError:
            if (not key == "conf_file" and not key == "logpath"): # do not warn about default content of config
                log.warning("Parameter '" + key + "' was not found in configuration template "+args.conf_file)

    # CSV table
    log.debug ("Loading csv file (if requested)")
    parameters_csv = loadparamsfromcsv(args.csv_file, runs) # store all information needed from the csv file

    # setup mechanism to deal with user pressing ctrl-c in a safe way while we execute Corryvreckan later
    import signal
    keepRunning = {'Sigint':'no'}
    def signal_handler(signal, frame):
        """ log if SIGINT detected, set variable to indicate status """
        log.critical ('You pressed Ctrl+C!')
        keepRunning['Sigint'] = 'seen'
    prevINTHandler = signal.signal(signal.SIGINT, signal_handler)

    log.info("Will now start processing the following runs: "+', '.join(map(str, runs)))
    # now loop over all runs
    for run in runs:
        n_repeat = 0 # counts how many times one run occurs in the config file with different configurations
        i_repeat = 0 # repeat until i_repeat = n_repeat
        while True: # break when not repeating the same run again
            if keepRunning['Sigint'] == 'seen':
                log.critical("Stopping to process remaining runs now")
                break  # if we received ctrl-c (SIGINT) we stop processing here

            if args.zfill:
                runnr = str(run).zfill(args.zfill)
            else:
                runnr = str(run)
            log.info ("Now generating configuration file for run number "+runnr+"..")

            # When  running in subdirectories for every job, create it:
            if args.subdir:
                basedirectory = "run_"+runnr
                if not os.path.exists(basedirectory):
                    os.makedirs(basedirectory)

                # Descend into subdirectory:
                savedPath = os.getcwd()
                os.chdir(basedirectory)

            if parameters_csv:
                for line in parameters_csv: # go through line by line
                    # make a copy of the preprocessed steering file content
                    steeringString = steeringStringBase
                    # if we have a csv file we can parse, we will check for the runnumber and replace any
                    # variables identified by the csv header by the run specific value

                    try:
                        if parameters_csv[line]["runnumber"] != runnr:
                            continue
                        appendix = '' # empty string if one run is analysed only once
                        for field in parameters_csv[line].keys():

                            # prepare empty list in case of a set or range of parameters like {10,12}
                            current_parameter = list()

                            log.debug("Next parameter: %s", parameters_csv[line][field])
                            log.debug("parameters_csv[line][field][0] = %s", parameters_csv[line][field][0])
                            # remove all whitespaces from beginning and end of string (not in the middle)
                            parameters_csv[line][field] = parameters_csv[line][field].strip()
                            if parameters_csv[line][field][0] == '{':
                                log.debug("Found open bracket, look for matching close bracket.")
                                if parameters_csv[line][field][-1] == '}':
                                    log.debug("Found matching close bracket, Interpret as range or set of parameters.")
                                    # remove curly brackets:
                                    parameter_field = parameters_csv[line][field].strip("{}")
                                    # Check if csv field contains "," or "-", i.e. a set or range of values
                                    # If not, no conversion is required (or even possible in case of file paths etc.)
                                    # If yes, call parseIntegerString() and create multiple configuration files.
                                    if any(delimiter in parameter_field for delimiter in [',','-']):
                                        current_parameter = parseIntegerString(parameter_field)
                                        n_repeat = len(current_parameter)
                                        log.debug("Found delimiter for '%s'", field)
                                    else:
                                        # current_parameter needs to be a list to get len(list) = 1
                                        current_parameter.append(parameters_csv[line][field])
                                        log.debug("No delimiter found for '%s'", field)
                                else:
                                    log.error("No matching close bracket found. Please update CSV file.")
                                    exit(1)

                            else:
                                log.debug("No bracket found, interpret as one string.")
                                current_parameter.append(parameters_csv[line][field])

                            log.debug("current_parameter has length %d", len(current_parameter))
                            # check if we actually find all parameters from the csv file in the steering file - warn if not
                            log.debug("Parsing steering file for csv field name '%s'", field)
                            try:
                                # check that the field name is not empty and do not yet replace the runnumber
                                if not field == "":
                                    if len(current_parameter) == 1:
                                        steeringString = ireplace("@" + field + "@", parameters_csv[line][field], steeringString)
                                    else:
                                        log.debug("list index, n_repeat = '%d', i_repeat = '%d'", n_repeat, i_repeat)
                                        steeringString = ireplace("@" + field + "@", str(current_parameter[i_repeat]), steeringString)
                                        appendix = appendix + '_' +field + str(current_parameter[i_repeat])
                                        i_repeat += 1
                                        log.debug("appendix is now '%s'", appendix)
                            except EOFError:
                                log.warning("Parameter '" + field + "' from the csv file was not found in the template file (already overwritten by config file parameters?)")
                    except KeyError:
                        log.warning("Run #" + runnr + " was not found in the specified CSV file - will skip this run! ")
                        continue

                    if not checkSteer(steeringString):
                        return 1

                    if args.htcondor_file:
                        args.htcondor_file = os.path.abspath(args.htcondor_file)
                        if not os.path.isfile(args.htcondor_file):
                            log.critical("HTCondor submission parameters file '"+args.htcondor_file+"' not found!")
                            return 1

                    # update this line too
                    log.debug ("Writing steering file for run %i", run)

                    # Get "jobtask" as basename of the configuration file:
                    jobtask = os.path.splitext(os.path.basename(args.conf_file))[0]
                    # Write the steering file:
                    basefilename = jobtask+"_run"+runnr+appendix
                    log.info("basefilename = " + basefilename)
                    steeringFile = open(basefilename+".conf", "w")

                    try:
                        steeringFile.write(steeringString)
                    finally:
                        steeringFile.close()

                    # bail out if running a dry run
                    if args.dry_run:
                        log.info("Dry run: skipping Corryvreckan execution. Steering file written to "+basefilename+'.conf')
                    elif args.htcondor_file:
                        rcode = submitCondor(basefilename, args.htcondor_file, basefilename) # start HTCondor submission
                        if rcode == 0:
                            log.info("HTCondor job submitted")
                        else:
                            log.error("HTCondor submission returned with error code "+str(rcode))
                    else:
                        rcode = runCorryvreckan(basefilename, basefilename, args.silent) # start Corryvreckan execution
                        if rcode == 0:
                            log.info("Corryvreckan execution done")
                        else:
                            log.error("Corryvreckan returned with error code "+str(rcode))
                        zipLogs(parameters["logpath"], basefilename)

                    # Return to old directory:
                    if args.subdir:
                        os.chdir(savedPath)

            if (i_repeat == n_repeat): # break the while loop
                log.debug("Finished scanning run %d'.", run)
                break
            # end while true

        # return to the previous signal handler
        signal.signal(signal.SIGINT, prevINTHandler)
        if log.error.counter>0:
            log.warning("There were "+str(log.error.counter)+" error messages reported")

    return 0

if __name__ == "__main__":
    sys.exit(main())
