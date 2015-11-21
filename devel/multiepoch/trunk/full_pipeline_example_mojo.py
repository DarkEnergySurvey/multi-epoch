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
you can then run this pipeline from anywhere by executing

$ mojo run_config multiepoch.config.full_pipeline

Any of the parameters specified in this config file can be overwritten from
the command line call using the --ARGUMENTNAME ARGUMENTVALUE syntax.

You can also copy or import this configuration file into any python package on
the python path, edit it and run your pipeline specification by executing

$ mojo run_config my_python_package.my_pipeline_config 

$ mojo run_config multiepoch.config.full_pipeline_felipe

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import os


# TOP LEVEL CONFIGURATION
# -----------------------------------------------------------------------------

tilename = 'DES2246-4457_section'
EXECUTION_MODE = 'tofile'

jobs = [
        'multiepoch.tasks.query_tileinfo',
        #'multiepoch.tasks.find_ccds_in_tile',
        #'multiepoch.tasks.plot_ccd_corners_destile',
        #'multiepoch.tasks.get_fitsfiles',
        #'multiepoch.tasks.me_prepare',
        #'multiepoch.tasks.call_SWarp',
        #'multiepoch.tasks.call_Stiff',
        #'multiepoch.tasks.call_SExpsf',
        #'multiepoch.tasks.call_psfex',
        #'multiepoch.tasks.call_SExDual',
        #'multiepoch.tasks.make_MEFs',
        ]


# SETTING UP THE PATHS
# -----------------------------------------------------------------------------
MULTIEPOCH_ROOT   = os.path.join(os.environ['HOME'],'MULTIEPOCH_ROOT')
local_archive     = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE')
local_archive_me  = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE_ME')
outputpath = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 
tiledir = os.path.join(outputpath, tilename)

# GENERIC COMPUTATIONAL SETTINGS 
# -----------------------------------------------------------------------------
NTHREADS = 8
NCPU = 8

# LOGGING
# -----------------------------------------------------------------------------
stdoutloglevel = 'DEBUG'
fileloglevel = 'DEBUG'
logfile = os.path.join(tiledir, tilename+'_full_pipeline.log')

# JOB SPECIFIC CONFIGURATION
# -----------------------------------------------------------------------------

# query_tileinfo >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
coaddtile_table = 'felipe.coaddtile_new'
db_section = 'db-destest'
#desservicesfile = os.path.join(MULTIEPOCH_ROOT, '.desservices.ini')

# find_ccds_in_tile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
tagname = 'Y2T3_FINALCUT'
exec_name = 'immask'

# plot_ccd_corners_destile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# -


# get_fitsfiles >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
http_section = 'http-desarchive'


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
