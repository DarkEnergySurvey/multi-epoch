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

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

available tiles for testing.
felipe, april 4, 2015

tiles_RXJ2248 = ['DES2251-4331',
                    'DES2251-4414',
                    'DES2254-4457',
                    'DES2247-4331',
                    'DES2247-4414',
                    'DES2246-4457',
                    'DES2250-4457']

tiles_ElGordo = ['DES0105-4831',
                    'DES0059-4957',
                    'DES0103-4957',
                    'DES0058-4914',
                    'DES0102-4914',
                    'DES0106-4914',
                    'DES0101-4831']

'''

import os


# TOP LEVEL CONFIGURATION
# -----------------------------------------------------------------------------

tilename = 'DES2246-4457'

jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.find_ccds_in_tile',

        'multiepoch.tasks.plot_ccd_corners_destile',

        'multiepoch.tasks.get_fitsfiles',
        ]


# SETTING UP THE PATHS
# -----------------------------------------------------------------------------

# Only the REQUIRED PIPELINE PARAMETERS local_archive, local_weights and
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

# local_weigths :: REQUIRED PIPELINE PARAMETER !!
# The weights archive is a mirrored directory structure of the local_archive
# and can be chosen to be the same as local_archive in case you would like your
# weights files to end up in the 'archive'.
# Required permissions for the executing user: writing
local_weights = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_WEIGHTS')

# outputpath :: not required organizational support variable
# Required permissions for the executing user: writing
outputpath = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 

# tiledir :: REQUIRED PIPELINE PARAMETER !!
# The directory where ultimately all output will end up for the tile being
# produced here.
# Required permissions for the executing user: writing
tiledir = os.path.join(outputpath, tilename)


# LOGGING
# -----------------------------------------------------------------------------
stdoutloglevel = 'DEBUG'
fileloglevel = 'DEBUG'
logfile = os.path.join(tiledir, tilename+'_dbquery_filetransfer.log')


# CTX DUMP 
# -----------------------------------------------------------------------------
json_dump_file = os.path.join(tiledir, 'ctx', tilename+'_dbquery_filetransfer.json')
dump_var_list = ['tilename', 'tileinfo', 'local_archive', 'local_weights', 
			'tiledir', ]



# JOB SPECIFIC CONFIGURATION
# -----------------------------------------------------------------------------

# query_tileinfo >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
coaddtile_table = 'felipe.coaddtile_new'
db_section = 'db-destest'
desservicesfile = os.path.join(MULTIEPOCH_ROOT, '.desservices.ini')


# find_ccds_in_tile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
tagname = 'Y2T_FIRSTCUT'
exec_name = 'immask'


# plot_ccd_corners_destile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# -


# get_fitsfiles >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
http_section = 'http-desarchive'
