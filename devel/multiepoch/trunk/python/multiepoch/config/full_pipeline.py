'''
A mojo configuration to run the entire multiepoch pipeline.


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

$ mojo run_config multiepoch.config.me_full_config

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



EXECUTION_MODE = 'dryrun' # alternatively : 'tofile', 'execute'


jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.set_tile_directory',
        'multiepoch.tasks.find_ccds_in_tile',
        # alternatively to running the 3 tasks you can also run the single job
        # below
        #'multiepoch.tasks.query_database',
#       'multiepoch.tasks.plot_ccd_corners_destile',
#       'multiepoch.tasks.get_fitsfiles',
#       'multiepoch.tasks.make_SWarp_weights',
#       'multiepoch.tasks.call_SWarp_CustomWeights',
#       'multiepoch.tasks.call_Stiff',
#       'multiepoch.tasks.call_SExpsf',
#       'multiepoch.tasks.call_psfex',
#       'multiepoch.tasks.call_SExDual',
        ]


NTHREADS = 8
NCPU = 8

DATA_PATH = os.path.join(os.environ['HOME'], 'DESDM', 'MULTIEPOCH_DATA') 




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
stdoutloglevel = 'INFO'
fileloglevel = 'INFO'
logfile = os.path.join(os.environ['HOME'], 'multiepoch.log')
