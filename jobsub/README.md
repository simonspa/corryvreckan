## Jobsub

### Overview

`jobsub` is a tool for the convenient run-specific modification of Corryvreckan configuration files and their execution through the `corry` executable. It is derived from the original `jobsub` written for EUTelescope by Hanno Perrey, Lund University.

### Usage

The following help text is printed when invoking `jobsub` with the `-h` argument:

```result
usage: jobsub.py [-h] [--option NAME=VALUE] [-c FILE] [-csv FILE]
                 [--log-file FILE] [-l LEVEL] [-s] [--dry-run]
		 [--batch FILE] [--subdir]
                 jobtask [runs [runs ...]]

A tool for the convenient run-specific modification of Marlin steering files
and their execution through the Marlin processor

positional arguments:
  runs                  The runs to be analyzed; can be a list of single runs
                        and/or a range, e.g. 1056-1060.

optional arguments:
  -h, --help            show this help message and exit
  --version             show program's version number and exit
  -c FILE, --conf-file FILE, --config FILE
                        Configuration file with all Corryvreckan algorithms
                        defined
  --option NAME=VALUE, -o NAME=VALUE
                        Specify further options such as 'beamenergy=5.3'. This
                        switch be specified several times for multiple options
                        or can parse a comma-separated list of options. This
                        switch overrides any config file options and also
                        overwrites hard-coded settings on the Corryvreckan
                        configration file.
  -htc FILE, --htcondor-file FILE, --batch FILE
                        Specify condor_submit parameter file for HTCondor submission. Run
                        HTCondor submission via condor_submit instead of calling
                        Corryvreckan directly
  -csv FILE, --csv-file FILE
                        Load additional run-specific variables from table
                        (text file in csv format)
  --log-file FILE       Save submission log to specified file
  -v LEVEL, --verbosity LEVEL
                        Sets the verbosity of log messages during job
                        submission where LEVEL is either debug, info, warning
                        or error
  -s, --silent          Suppress non-error (stdout) Corryvreckan output to
                        console
  --dry-run             Write configuration files but skip actual Corryvreckan
                        execution
  --subdir              Execute every job in its own subdirectory instead of
                        all in the base path
  --plain               Output written to stdout/stderr and log file in
                        prefix-less format i.e. without time stamping
  --zfill N             Fill run number with zeros up to the defined number of
                        digits
```

The environment variables need to be set using ```source etc/setup_lxplus.sh```.
When using a submission file, `getenv = True` should be used (see example.sub).

### Preparation of Configuration File Templates

Configuration file templates are valid Corryvreckan configuration files in TOML format, where single values are replaced by variables in the form `@SomeVariable@`.
A more detailed description of the configuration file format can be found elsewhere in the user manual.
The section of a configuration file template with variable geometry file and DUT name could e.g. look like

```toml
[Corryvreckan]
detectors_file = "@telescopeGeometry@"
histogram_file = "histograms_run@RunNumber@.root"

number_of_events = 5000000

log_level = WARNING
```

When `jobsub` is executed, these placeholders are replaced with user-defined values that can be specified through command-line arguments or a table with a row for each run number processed, and a final configuration file is produced for each run separately, e.g.

```toml
[Corryvreckan]
detectors_file = "my_telescope_Nov2017_1.conf"
histogram_file = "histograms_run999.root"

number_of_events = 5000000

log_level = WARNING
```

There is only one predefined placeholder, `@RunNumber@`, which will be substituted with the current run number. Run numbers are not padded with leading zeros unless the `--zfill` option is provided.

### Using Configuration Variables

As described in the previous paragraph, variables in the configuration file template are replaced with values at run time.
Two sources of values are currently supported, and are described in the following.

#### Command Line
   Variable substitutions can be specified using the `--option` or `-o` command line switches, e.g.

   ```bash
   jobsub.py --option beamenergy=5.3 -c alignment.conf 1234
   ```

   This switch be specified several times for multiple options or can parse a comma-separated list of options. This switch overrides any config file options.

#### Table (comma-separated text file)
   - format: e.g.
     - export from Open/LibreOffice with default settings (UTF-8,comma-separated, text-field delimiter: ")
     - emacs org-mode table (see http://orgmode.org/manual/Tables.html)
     - use Atom's *tablr* extension
   - commented lines (starting with #) are ignored
   - first row (after comments) has to provide column headers which identify the variables in the steering template to replace (case-insensitive)
   - requires one column labeled "RunNumber"
   - only considers placeholders left in the steering template after processing command-line arguments and config file options
   -
##### Example
    The CSV file could have the following form:

    ```csv
    RunNumber, BeamEnergy, telescopeGeometry
          4115,          1, telescope_june2017_1.conf
          4116,          2, telescope_june2017_1.conf
          4117,          3, telescope_june2017_1.conf
          4118,          4, telescope_june2017_1.conf
          4119,          5, telescope_june2017_1.conf
    ```

    Using this table, the variables `@BeamEnergy@` and `@telescopeGeometry@` in the templates would be replaced by the values corresponding to the current run number.
