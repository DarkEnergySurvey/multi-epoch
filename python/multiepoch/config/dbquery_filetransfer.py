'''
A mojo configuration to run the database queries and the filetransfer if
necessary.


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
then you can run the entire pipeline from anywhere by executing

$ mojo run_config multiepoch.config.dbquery_filetransfer

Any of the parameters specified in this config file can be overwritten from
the command line call using the --ARGUMENTNAME=ARGUMENTVALUE syntax.

You can also copy this configuration file into any python package on the python
path, edit it and run your pipeline specification by executing

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
        'multiepoch.tasks.set_tile_directory',
        'multiepoch.tasks.find_ccds_in_tile',
        # alternatively to running the 3 tasks you can also run the single job
        # below
        #'multiepoch.tasks.query_database',
        'multiepoch.tasks.plot_ccd_corners_destile',
        #'multiepoch.tasks.get_fitsfiles',
        ]

DATA_PATH = os.path.join(os.environ['HOME'], 'desdm', 'MULTIEPOCH_DATA') 

json_dump_file = os.path.join(DATA_PATH, 'CTXDUMP',
        tilename+'_tilename_tileinfo.json')
dump_var_list = ['tilename', 'tileinfo',]




# JOB SPECIFIC CONFIGURATION
# -----------------------------------------------------------------------------

# query_tileinfo >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
coaddtile_table = 'felipe.coaddtile_new'
db_section = 'db-destest'

# set_tile_directory >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
outputpath = os.path.joint(DATA_PATH, "TILEBUILDER")

# find_ccds_in_tile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
tagname = 'Y2T_FIRSTCUT'
exec_name = 'immask'

# plot_ccd_corners_destile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# no extra config

# get_fitsfiles >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
local_archive = os.path.join(DATA_PATH, 'LOCAL_DESAR')
http_section = 'http-desarchive'

# LOGGING
# -----------------------------------------------------------------------------
stdoutloglevel = 'DEBUG'
fileloglevel = 'DEBUG'
logfile = os.path.join(DATA_PATH, 'LOG', tilename+'.log')
