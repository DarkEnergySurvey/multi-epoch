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

$ mojo run_config multiepoch.config.dbquery_filetransfer

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

'''

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

jobs = [
        #'multiepoch.tasks.setup_paths',
        'multiepoch.tasks.query_tileinfo',
        #'multiepoch.tasks.set_tile_directory',
        'multiepoch.tasks.find_ccds_in_tile',
        # alternatively to running the 3 tasks you can also run the single job
        # below
#       'multiepoch.tasks.query_database',
        'multiepoch.tasks.plot_ccd_corners_destile',
        'multiepoch.tasks.get_fitsfiles',
        ]


# SETTING UP THE PATHS
# -----------------------------------------------------------------------------

# for default standalone applications we define one writeable root directory
# this directory can be set to be the users home dir, nothing will directly be
# written into here but into subdirectories herein only ..
# in docker containers you will want to have this directory be a mounted volume
MULTIEPOCH_ROOT = os.path.abspath('/MULTIEPOCH_ROOT')

LOCAL_ARCHIVE = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_DESAR')
outputpath = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 

# the directory where ultimately all output will end up
tiledir = os.path.join(outputpath, tilename)

# at the end of a run of this task we persist some of the ctx to be able to
# reuse the generated information in later executable runs
#json_dump_file = os.path.join(tiledir, tilename+'_tilename_tileinfo.json')
#dump_var_list = ['tilename', 'tileinfo', 'assoc',]



# JOB SPECIFIC CONFIGURATION
# -----------------------------------------------------------------------------

# query_tileinfo >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
coaddtile_table = 'felipe.coaddtile_new'
db_section = 'db-destest'
desservicesfile = '/MULTIEPOCH_ROOT/.desservices.ini'

# set_tile_directory >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# -

# find_ccds_in_tile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
tagname = 'Y2T_FIRSTCUT'
exec_name = 'immask'

# plot_ccd_corners_destile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# no extra config

# get_fitsfiles >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
local_archive = LOCAL_ARCHIVE 
# FIXME !!
filepath_local = LOCAL_ARCHIVE 
http_section = 'http-desarchive'


# LOGGING
# -----------------------------------------------------------------------------
stdoutloglevel = 'DEBUG'
fileloglevel = 'DEBUG'
logfile = os.path.join(tiledir, tilename+'_dbquery_filetransfer.log')
