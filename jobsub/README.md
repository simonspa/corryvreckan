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

For usage on `lxplus`, the environment variables need to be set by running ```source etc/setup_lxplus.sh```.
When using a submission file, `getenv = True` should be used (as in `example.sub`).

### Preparation of Configuration File Templates

Configuration file templates are valid Corryvreckan configuration files in TOML format, where single values are replaced by variables in the form `@SomeVariable@`.
A more detailed description of the configuration file format can be found elsewhere in the user manual.
The section of a configuration file template with variable geometry file and DUT name could e.g. look like

```toml
[Corryvreckan]
detectors_file = "@telescopeGeometry@"
histogram_file = "histograms_@RunNumber@.root"

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

This switch can be specified several times for multiple options or can parse a comma-separated list of options. This switch overrides any config file options.

#### Table (comma-separated text file)
Tables in the form of CSV files can be used to replace placeholders with the `-csv` option.
For the correct format, the following tools can be used:
- export from Open/LibreOffice with default settings (UTF-8,comma-separated, text-field delimiter: ")
- emacs org-mode table (see http://orgmode.org/manual/Tables.html)
- use Atom's *tablr* extension
or the CSV file can be edited in a text editor of choice.

The following rules apply:
- Commented lines (starting with `#`) are ignored.
- The first row (after comments) has to provide column headers which identify the variables in the steering template to replace (case-insensitive)
- One column labeled "RunNumber" is required.
- Only placeholders left in the steering template __after__ processing command-line arguments and config file options are filled with values from the CSV file.

Strings can be passed by the user of double-quotes `" "` which also avoid the separation by commas.
A double-quote can be used as part of a string when using the escape character backslash `\` in front of the double-quote.

It is also possible to specify multiple different settings for the same run number by making use of the following syntax to specify a set or range of parameters.
Curly brackets in double-quotes `"{ }"` can be used to indicate a set (indicated by a comma `,`) or range (indicated by a dash `-`) of parameters which will be split up and processed one after the other.
If a set or a range is detected, the parameter plus its value are attached to the name of the configuration file.
Ranges can only be used for integer values (without units).
However, a set or range can oly be used for one parameter, i.e. multi-dimensional parameters scans are not supported and have to be separated into individual CSV files.

* `"{10,12-14}"` translates to `10`, `12`, `13`, `14` in consecutive jobs for the same run number
* `"{10ns, 20ns}"` tranlates to `10ns`, `20ns` in consecutive jobs for the same run number
* `"string,with,comma"` translates to `string,with,comma` in one job
* `"{string,with,comma}"` which translates to `string`, `with`, `comma` in consecutive jobs for the same run number
* `""\string in quotes\""` translates to `"string in quotes"`

If a range or set of parameters is detector, the naming scheme of the auto-generated configuration files is extended from `MyAnalysis_run@RunNUmber@.conf` to `MyAnalysis_run@RunNUmber@_OtherParameter@OtherParameter@.conf`

It must be insured by the user that the output ROOT file is not simply called `histograms_@RunNumber@.root` but rather `histograms_@RunNumber@_OtherParameter@OtherParameter@.root` to prevent overwriting the output file.

##### Example
The CSV file could have the following form:

```csv
# AnalysisExample.csv
# This is an example.
RunNumber,  ExampleParameter,   AnotherParameter
100,        "{3-5}",            10ns
101,        3,                  "{10ns, 20ns}"
```
Using this table, the placeholders `@RunNUmber@`, `@ExampleParameter@`, and `@AnotherParameter@` in the template file `AnalysisExample.conf` would be replaced by the values corresponding to the current run number and the following configuration files would be generated:
```
AnalysisExample_run100_exampleparameter3.conf
AnalysisExample_run100_exampleparameter4.conf
AnalysisExample_run100_exampleparameter5.conf
AnalysisExample_run101_anotherparameter10ns.conf
AnalysisExample_run101_anotherparameter20ns.conf
```
### Example Usage with a Batch File:

Example command line usage:
```bash
./jobsub.py -c /path/to/example.conf -v DEBUG --batch /path/to/example.sub --subdir <run_number>
```

An example batch file is provided in the repository as `htcondor.sub`.
Complicated and error-prone `transfer_output_files` commands can be avoided. It is much simpler to set an absolute path like
```
output_directory = "/eos/user/y/yourname/whateveryouwant/run@RunNumber@"
```
directly in the Corryvreckan config file.
