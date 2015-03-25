'''
A mojo configuration to run the entire multiepoch pipeline.


How to Run
```````````
Assuming you have a proper installation (through eups) of the multiepoch
package with all its dependencies and you executed

$ setup multiepoch

then you can run the entire pipeline from anywhere by executing

$ mojo run_config multiepoch.config.me_full_config

Any of the parameters specified in this config file can be overwritten from
the command line call using the --ARGUMENTNAME=ARGUMENTVALUE syntax.

You can also copy this configuration file into any python package on the python
path, edit it and run your pipeline specification by executing

$ mojo run_config my_python_package.my_pipeline_config 
'''

# TOP LEVEL CONFIGURATION
# -----------------------------------------------------------------------------

tilename = 'DES2246-4457' 

EXECUTION_MODE = 'dryrun' # alternatively : 'tofile', 'execute'

jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.set_tile_directory',
        'multiepoch.tasks.find_ccds_in_tile',
        # alternatively to running the 3 tasks you can also run the single job
        # below
        #'multiepoch.tasks.query_database',
        'multiepoch.tasks.call_SWarp_michael_dev',
        'multiepoch.tasks.plot_ccd_corners_destile',
        'multiepoch.tasks.get_fitsfiles',
        'multiepoch.tasks.make_SWarp_weights',
        'multiepoch.tasks.call_SWarp_CustomWeights',
        'multiepoch.tasks.call_Stiff',
        'multiepoch.tasks.call_SExpsf',
        'multiepoch.tasks.call_psfex',
        'multiepoch.tasks.call_SExDual',
        ]


# JOB SPECIFIC CONFIGURATION
# -----------------------------------------------------------------------------

# query_tileinfo >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
coaddtile_table = 'felipe.coaddtile_new'
db_section = 'db-destest'

# set_tile_directory >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
outputpath = os.environ['HOME']+"/TILEBUILDER_TEST"

# find_ccds_in_tile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
tagname = 'Y2T_FIRSTCUT'
exec_name = 'immask'

# plot_ccd_corners_destile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# no extra config

# get_fitsfiles >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
local_archive = os.path.join(os.environ['HOME'],'LOCAL_DESAR')
http_section = 'http-desarchive'

# make_SWarp_weights >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
clobber_weights = False
MP_weight = 4

# call_SWarp_CustomWeights >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
swarp_parameters = {
    "NTHREADS"     :8,
    "COMBINE_TYPE" : "AVERAGE",    
    "PIXEL_SCALE"  : 0.263,
    }
DETEC_COMBINE_TYPE = "CHI-MEAN"
swarp_execution_mode = EXECUTION_MODE

# call_Stiff >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
stiff_params = {
    "NTHREADS"  :8,
    "COPYRIGHT" : "NCSA/DESDM",
    "WRITE_XML" : "N",
    }
stiff_execution_mode = EXECUTION_MODE

# call_Stiff >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SExpsf_execution_mode = EXECUTION_MODE

# call_psfex >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
psfex_parameters = {
    "NTHREADS"  :8,
    }
psfex_execution_mode = EXECUTION_MODE

# call_SExDual >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SExDual_parameters = {
    "MAG_ZEROPOINT":30,
    }
SExDual_execution_mode = EXECUTION_MODE
MP_SEx=8


# LOGGING
# -----------------------------------------------------------------------------
stdoutloglevel = 'INFO'
fileloglevel = 'INFO'
logfile = 'multiepoch.log'
