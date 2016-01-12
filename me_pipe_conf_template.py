import os

# Generic template to run a TILENAME using mojo, to run:
#    mojo run_config ./me_pipe_conf_template.py --tilename DES2359+0001 --tiledir ~/TILEBUILDER/DES2359+0001
# 
# Most options can be overide from the command-line

# The tilename
#tilename = 'DES2359+0001'

# tofile/dryrun/execute
#EXECUTION_MODE = 'dryrun'
#EXECUTION_MODE = 'tofile'
EXECUTION_MODE = 'execute'

# Clean up files True/False
cleanupPSFcats = True
cleanupSWarp   = True

# This is the list of tasks we want to execute -- comment as required
jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.find_me_inputs_in_tile',
        'multiepoch.tasks.plot_ccd_corners_destile',
        'multiepoch.tasks.get_fitsfiles',
        'multiepoch.tasks.prepare_red_files',
        'multiepoch.tasks.prepare_scamp_files',
        'multiepoch.tasks.call_scamp',
        'multiepoch.tasks.prepare_head_files',
        'multiepoch.tasks.call_SWarp',
        'multiepoch.tasks.call_coadd_MEF',
        'multiepoch.tasks.call_Stiff',
        'multiepoch.tasks.call_SExpsf',
        'multiepoch.tasks.call_psfex',
        'multiepoch.tasks.call_SExDual',
        ]

# SETTING UP THE OUTPUT, only tiledir and local_archive need to be defined
MULTIEPOCH_ROOT = os.path.join(os.environ['HOME'],'MULTIEPOCH_ROOT')
local_archive   = os.path.join(MULTIEPOCH_ROOT,'LOCAL_ARCHIVE')
# uncomment tiledir if you don't want to use the --tiledir option
#tiledir         = os.path.join(MULTIEPOCH_ROOT,'TILEBUILDER',tilename + "Y2T7_test1")

# GENERIC COMPUTATIONAL SETTINGS 
NTHREADS = 8
NCPU     = 6
nthreads = NTHREADS

# LOGGING
stdoutloglevel = 'DEBUG'
fileloglevel   = 'DEBUG'
#logfile = os.path.join(tiledir, tilename+'_full_pipeline.log')

# TASK SPECIFIC CONFIGURATION

# 1. query_tileinfo --  Get the tile information from the table -- unless provided
coaddtile_table = 'felipe.coaddtile_new'
db_section      = 'db-destest'

# 2. find_me_inputs_in_tile -- Get the CCDs and exposure catalogs inside the tile -- unless provided
tagname   = 'Y2T7_FINALCUT'
super_align = True

# In case we want to skip step 1) and 2) we can define the inputs here to be loaded here
#assoc_file           = 'DES2246-4457_ccdinfo.assoc'
#tile_geom_input_file = 'DES2246-4457_section_tileinfo.json'
#cats_file            = 'DES2359+0001_cats.list'

# 3. get_fitsfiles -- Retrieve the files, if not running on cosmology cluster (most cases)
http_section = 'http-desarchive'

# 4. prepare_scamp_files -- Prepare the inputs to run scamp
MP_cats = NCPU
execution_mode_scamp_prep = EXECUTION_MODE

# 5. call_scamp -- the call to scamp
execution_mode_scamp = EXECUTION_MODE
#scamp_conf = 
    
# 6. Prepare the head files
execution_mode_head = EXECUTION_MODE
MP_head = 1

# 7. prepare_red_files -- Prepare single-epoch input for coadd (performs interpolation and null_weights)
extension_me      = "me"
weight_for_mask   = True
clobber_me        = False
MP_me             = NCPU
execution_mode_red = EXECUTION_MODE

# 8. call_SWarp -- The SWarp call 
swarp_parameters = {
    "NTHREADS"     : NTHREADS,
    "COMBINE_TYPE" : "AVERAGE",    
    "PIXEL_SCALE"  : 0.263,
    }
magbase              = 30.0
detname              = 'det'
doBANDS              = ['all']
DETEC_COMBINE_TYPE   = "CHI-MEAN"
execution_mode_swarp = EXECUTION_MODE
#swarp_conf = /path/to/file

# 9. call_coadd_MEF -- Combine the 3 planes SCI/WGT/MSK into a single image, interpolate the SCI and create MSK 
xblock    = 10
add_noise = False
execution_mode_MEF = EXECUTION_MODE

# 10. call_Stiff -- Create the color images using stiff
stiff_parameters = {
    "VERBOSE_TYPE": "FULL",
    "NTHREADS"    : NTHREADS,
    }
execution_mode_stiff = EXECUTION_MODE
#stiff_conf = /path/to/file

# 11. call_SExpsf -- make the SEx psf Call
MP_SEx = NCPU
execution_mode_SExpsf = EXECUTION_MODE
#SExpsf_conf = /path/to/file

# 12. call_psfex -- Run psfex
execution_mode_psfex = EXECUTION_MODE
cleanupPSFcats = False
# psfex_conf = /path/to/file

# 13. call_SExDual -- Run SExtractor un dual mode 
SExDual_parameters = {
    "MAG_ZEROPOINT":30,
    }
MP_SEx                 = NCPU
execution_mode_SExDual = EXECUTION_MODE
# SExDual_conf = /path/to/file


# END ----
