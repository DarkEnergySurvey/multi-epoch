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
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.find_ccds_in_tile',

#       'multiepoch.tasks.plot_ccd_corners_destile',

        'multiepoch.tasks.get_fitsfiles',

        'multiepoch.tasks.make_SWarp_weights',
        'multiepoch.tasks.call_SWarp_michael_dev',
#       'multiepoch.tasks.call_SWarp_CustomWeights',
#       'multiepoch.tasks.call_Stiff',
#       'multiepoch.tasks.call_SExpsf',
#       'multiepoch.tasks.call_psfex',
#       'multiepoch.tasks.call_SExDual',
        ]


# SETTING UP THE PATHS
# -----------------------------------------------------------------------------

# MULTIEPOCH_ROOT ::
# for default standalone applications we define one writeable root directory
# in docker containers you will want to have this directory be a mounted volume
MULTIEPOCH_ROOT = os.path.abspath('/MULTIEPOCH_ROOT')

# local_archive ::
# the path where your archive input images are located
# either they are already present here or will be transferred to here
# !! if left empty we assume you're in the cosmology cluster and set
# local_archive accordingly (done in find_ccds_in_tile.py task)
local_archive = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_DESAR')

# weights_archive ::
# the weights archive is a mirrored directory structure of the local_archive
# and can be chosen to be the same as local_archive in case you would like your
# weights files to end up in the 'archive'.
# please be sure though that you have writing permissions therein!
weights_archive = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_WEIGHTS')

# TODO : change local_archive to archive_path and weights_archive to
# weights_path

# outputpath ::
outputpath = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 

# tiledir ::
# the directory where ultimately all output will end up for the tile being
# produced here.
tiledir = os.path.join(outputpath, tilename)

# at the end of a run of this task we persist some of the ctx to be able to
# reuse the generated information in later executable runs
#json_dump_file = os.path.join(tiledir, tilename+'_tilename_tileinfo.json')
#dump_var_list = ['tilename', 'tileinfo', 'assoc',]



# GENERIC COMPUTATIONAL SETTINGS 
# -----------------------------------------------------------------------------
NTHREADS = 8
NCPU = 8


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
# FIXME !! what's filepath_local good for???
filepath_local = local_archive 
http_section = 'http-desarchive'

# make_SWarp_weights >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
clobber_weights = False
MP_weight = NCPU
weights_execution_mode = 'execute'


# LOGGING
# -----------------------------------------------------------------------------
stdoutloglevel = 'DEBUG'
fileloglevel = 'DEBUG'
logfile = os.path.join(tiledir, tilename+'_full_pipeline.log')
