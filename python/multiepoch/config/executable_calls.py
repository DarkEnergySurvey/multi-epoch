'''
A mojo configuration for the

MULTIEPOCH PIPELINE


How to Run
```````````
Assuming you have a proper installation (through eups) of the multiepoch
package with all its dependencies and you executed

```
$ setup multiepoch
```
or 
```
$ setup -r -v .
```

from the multiepoch trunk directory after svn checkout
then you can run this pipeline from anywhere by executing

$ mojo run_config multiepoch.config.executable_calls

Any of the parameters specified in this config file can be overwritten from
the command line call using the --ARGUMENTNAME ARGUMENTVALUE syntax.

You can also copy or import this configuration file into any python package on
the python path, edit it and run your pipeline specification by executing

$ mojo run_config my_python_package.my_pipeline_config 

'''

import os

# TOP LEVEL CONFIGURATION
# -----------------------------------------------------------------------------

tilename = 'DES2246-4457'

# WARNING !! ------------------------------------------------------------------
# we need to load tileinfo and tilename from a ctx dump
# have a look at the dbquery_filetransfer.py config to see how to produce it!
MULTIEPOCH_ROOT = os.path.join(os.environ['HOME'], 'MULTIEPOCH_ROOT')
outputpath = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 
tiledir = os.path.join(outputpath, tilename)
json_load_file = os.path.join(tiledir, 'ctx',
        tilename+'_dbquery_filetransfer.json')
assoc_json = os.path.join(tiledir, 'ctx',
        tilename+'_assoc.json')


EXECUTION_MODE = 'execute' # alternatively : 'tofile', 'execute', 'dryrun'

jobs = [
        'multiepoch.tasks.make_SWarp_weights',
        'multiepoch.tasks.call_SWarp',
        'multiepoch.tasks.call_Stiff',
        'multiepoch.tasks.call_SExpsf',
        'multiepoch.tasks.call_psfex',
        'multiepoch.tasks.call_SExDual',
        'multiepoch.tasks.make_MEFs',
        ]


NTHREADS = 8
NCPU = 8


# IMPORTS 
# -----------------------------------------------------------------------------
#from .dbquery_filetransfer import local_archive, outputpath


# LOGGING
# -----------------------------------------------------------------------------
stdoutloglevel = 'DEBUG'
fileloglevel = 'DEBUG'
logfile = os.path.join(tiledir, tilename+'_executable_calls.log')


# JOB SPECIFIC CONFIGURATION
# -----------------------------------------------------------------------------

# make_SWarp_weights >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
clobber_weights = False
MP_weight = NCPU
weights_execution_mode = EXECUTION_MODE


# call_SWarp >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
swarp_parameters = {
    "NTHREADS"     : NTHREADS,
    "COMBINE_TYPE" : "AVERAGE",    
    "PIXEL_SCALE"  : 0.263,
    }
DETEC_COMBINE_TYPE = "CHI-MEAN"
swarp_execution_mode = EXECUTION_MODE


# call_Stiff >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
stiff_params = {
    "NTHREADS"  : NTHREADS, 
    }
stiff_execution_mode = EXECUTION_MODE


# call_SExpsf >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SExpsf_execution_mode = EXECUTION_MODE
MP_SEx = NCPU


# call_psfex >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
psfex_execution_mode = EXECUTION_MODE
cleanupPSFcats = False


# call_SExDual >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SExDual_parameters = {
    "MAG_ZEROPOINT":30,
    }
SExDual_execution_mode = EXECUTION_MODE
MP_SEx = NCPU


# make_MEFs >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
clobber_MEF = False
MEF_execution_mode = EXECUTION_MODE
cleanupSWarp = False
