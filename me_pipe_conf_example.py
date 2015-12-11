import os

# The tilename
tilename = 'DES2359+0001'

# tofile/dryrun/execut
EXECUTION_MODE = 'tofile'

# This is the list of tasks we want to executs
jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.find_ccds_in_tile',
        'multiepoch.tasks.plot_ccd_corners_destile',
        #'multiepoch.tasks.get_fitsfiles',
        #'multiepoch.tasks.me_prepare',
        #'multiepoch.tasks.call_SWarp',
        #'multiepoch.tasks.call_Stiff',
        #'multiepoch.tasks.call_SExpsf',
        #'multiepoch.tasks.call_psfex',
        #'multiepoch.tasks.call_SExDual',
        ]

# SETTING UP THE PATHS
MULTIEPOCH_ROOT   = os.path.join(os.environ['HOME'],'MULTIEPOCH_ROOT')
local_archive     = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE')
local_archive_me  = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE_ME')
outputpath        = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 
tiledir           = os.path.join(outputpath, tilename)

# GENERIC COMPUTATIONAL SETTINGS 
NTHREADS = 8
NCPU     = 8

# LOGGING
stdoutloglevel = 'DEBUG'
fileloglevel   = 'DEBUG'
logfile = os.path.join(tiledir, tilename+'_full_pipeline.log')

# JOB SPECIFIC CONFIGURATION

# query_tileinfo >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#coaddtile_table = 'felipe.coaddtile_new'
db_section = 'db-destest'
#assoc_file = 'DES2246-4457_ccdinfo.assoc'
#tile_geom_input_file = 'DES2246-4457_section_tileinfo.json'

# find_ccds_in_tile >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
tagname = 'Y2T4_FINALCUT'
exec_name = 'immask'

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
