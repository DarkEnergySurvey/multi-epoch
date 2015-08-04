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
you can then run this pipeline from anywhere by executing

$ mojo run_config multiepoch.config.full_pipeline

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

EXECUTION_MODE = 'tofile'

jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.find_ccds_in_tile',

        'multiepoch.tasks.plot_ccd_corners_destile',

        'multiepoch.tasks.get_fitsfiles',

        'multiepoch.tasks.make_SWarp_weights',
        #'multiepoch.tasks.call_SWarp_michael_dev',
        'multiepoch.tasks.call_SWarp_CustomWeights',
        'multiepoch.tasks.call_Stiff',
        'multiepoch.tasks.call_SExpsf',
        'multiepoch.tasks.call_psfex',
        'multiepoch.tasks.call_SExDual',
        'multiepoch.tasks.make_MEFs',
        ]


# SETTING UP THE PATHS
# -----------------------------------------------------------------------------

# Only the REQUIRED PIPELINE PARAMETERS local_archive, weights_archive and
# tiledir have to be set to run the pipeline. They CAN BE SET INDEPENDENTLY.
# MULTIEPOCH_ROOT and outputpath are simply supportive, non-required
# organisational variables.

# MULTIEPOCH_ROOT :: not required organizational support variable
# For default standalone applications we define one writeable root directory.
# In docker containers you will want to have this directory to be a mounted
# volume.
# Required permissions for the executing user: writing
#MULTIEPOCH_ROOT = os.path.abspath('/MULTIEPOCH_ROOT')
MULTIEPOCH_ROOT = os.path.join(os.environ['HOME'],'MULTIEPOCH_ROOT')

# local_archive :: REQUIRED PIPELINE PARAMETER !!
# The path where your input archive images are located: Either they are already
# present here or will be transferred to here using the get_fitsfiles task.
# Required permissions for the executing user: only reading required
local_archive = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_DESAR')

# weights_archive :: REQUIRED PIPELINE PARAMETER !!
# The weights archive is a mirrored directory structure of the local_archive
# and can be chosen to be the same as local_archive in case you would like your
# weights files to end up in the 'archive'.
# Required permissions for the executing user: writing
local_weight = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_WEIGHTS')

# outputpath :: not required organizational support variable
# Required permissions for the executing user: writing
outputpath = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 

# tiledir :: REQUIRED PIPELINE PARAMETER !!
# The directory where ultimately all output will end up for the tile being
# produced here.
# Required permissions for the executing user: writing
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
desservicesfile = '/MULTIEPOCH_ROOT/.desservices.ini'


# find_ccds_in_tile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
tagname = 'Y2T_FIRSTCUT'
exec_name = 'immask'


# plot_ccd_corners_destile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# -


# get_fitsfiles >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
http_section = 'http-desarchive'


# make_SWarp_weights >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
clobber_weights = False
MP_weight = NCPU
weights_execution_mode = 'execute'


# call_SWarp >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
swarp_parameters = {
    "NTHREADS"     : NTHREADS,
    "COMBINE_TYPE" : "AVERAGE",    
    "PIXEL_SCALE"  : 0.263,
    }
DETEC_COMBINE_TYPE = "CHI-MEAN"
swarp_execution_mode = EXECUTION_MODE
