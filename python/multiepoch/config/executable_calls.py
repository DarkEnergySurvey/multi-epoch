'''
A mojo configuration for the

MULTIEPOCH PIPELINE

to run the database queries and the filetransfer if necessary.


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

# WARNING !!
# we need to load tileinfo and tilename from a ctx dump
# have a look at the dbquery_filetransfer.py config to see how to produce it!
DATA_PATH = os.path.join(os.environ['HOME'], 'DESDM', 'MULTIEPOCH_DATA') 
json_load_file = os.path.join(DATA_PATH, 'CTXDUMP',
        tilename+'_tilename_tileinfo.json')

EXECUTION_MODE = 'execute' # alternatively : 'tofile', 'execute', 'dryrun'

jobs = [
        'multiepoch.tasks.make_SWarp_weights',
#       'multiepoch.tasks.call_SWarp_CustomWeights',
#       'multiepoch.tasks.call_Stiff',
#       'multiepoch.tasks.call_SExpsf',
#       'multiepoch.tasks.call_psfex',
#       'multiepoch.tasks.call_SExDual',
        ]


NTHREADS = 8
NCPU = 8


# IMPORTS 
# -----------------------------------------------------------------------------
from .dbquery_filetransfer import local_archive, outputpath



# JOB SPECIFIC CONFIGURATION
# -----------------------------------------------------------------------------

# make_SWarp_weights >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
clobber_weights = False
MP_weight = NCPU

# call_SWarp_CustomWeights >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
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
    "COPYRIGHT" : "NCSA/DESDM",
    "WRITE_XML" : "N",
    }
stiff_execution_mode = EXECUTION_MODE

# call_Stiff >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SExpsf_execution_mode = EXECUTION_MODE

# call_psfex >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
psfex_parameters = {
    "NTHREADS"  : NTHREADS,
    }
psfex_execution_mode = EXECUTION_MODE

# call_SExDual >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SExDual_parameters = {
    "MAG_ZEROPOINT":30,
    }
SExDual_execution_mode = EXECUTION_MODE
MP_SEx = NCPU


# LOGGING
# -----------------------------------------------------------------------------
stdoutloglevel = 'DEBUG'
fileloglevel = 'DEBUG'
logfile = os.path.join(DATA_PATH, 'LOG', tilename+'.log')
