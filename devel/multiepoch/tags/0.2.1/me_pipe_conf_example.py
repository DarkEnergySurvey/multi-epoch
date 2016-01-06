import os

# The tilename
tilename = 'DES2359+0001'

# tofile/dryrun/execute
#EXECUTION_MODE = 'tofile'
EXECUTION_MODE = 'execute'

# This is the list of tasks we want to executs
jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.find_ccds_in_tile',
        'multiepoch.tasks.plot_ccd_corners_destile',
        'multiepoch.tasks.get_fitsfiles',
        'multiepoch.tasks.me_prepare',
        'multiepoch.tasks.call_SWarp',
        'multiepoch.tasks.call_coadd_MEF',
        'multiepoch.tasks.call_Stiff',
        'multiepoch.tasks.call_SExpsf',
        'multiepoch.tasks.call_psfex',
        'multiepoch.tasks.call_SExDual',
        ]

# SETTING UP THE PATHS
MULTIEPOCH_ROOT   = os.path.join(os.environ['HOME'],'MULTIEPOCH_ROOT')
local_archive     = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE')
local_archive_me  = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE_ME')
outputpath        = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 
tiledir           = os.path.join(outputpath, tilename)

# GENERIC COMPUTATIONAL SETTINGS 
NTHREADS = 8
NCPU     = 6

# LOGGING
stdoutloglevel = 'DEBUG'
fileloglevel   = 'DEBUG'
logfile = os.path.join(tiledir, tilename+'_full_pipeline.log')

# TASK SPECIFIC CONFIGURATION

# 1. query_tileinfo --  Get the tile information from the table -- unless provided
coaddtile_table = 'felipe.coaddtile_new'
db_section      = 'db-destest'

# 2. find_ccds_in_tile -- Get the CCDs inside the tile -- unless provided
tagname   = 'Y2T4_FINALCUT'

# In case we want to skip step 1) and 2) we can define the inputs here
#assoc_file           = 'DES2246-4457_ccdinfo.assoc'
#tile_geom_input_file = 'DES2246-4457_section_tileinfo.json'

# 3. get_fitsfiles -- Retrieve the files -- if not running on cosmology cluster
http_section = 'http-desarchive'

# 4. me_prepare -- Prepare single-epoch input for coadd (performs interpolation and null_weights)
extension_me      = "_me"
weight_for_mask   = True
clobber_me        = True
MP_me             = NCPU
execution_mode_me = EXECUTION_MODE

# 5. call_SWarp -- The SWarp call 
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

# 6. call_coadd_MEF -- Combine the 3 planes SCI/WGT/MSK into a single image, interpolate the SCI and create MSK 
xblock    = 10
add_noise = False
execution_mode_MEF = EXECUTION_MODE

# 7. call_Stiff -- Create the color images using stiff
stiff_parameters = {
    "NTHREADS"    : NTHREADS,
    "DESCRIPTION" : "'Pseudo Color of coadded image for DES tile %s'" % tilename,
    }
execution_mode_stiff = EXECUTION_MODE

# 8. call_SExpsf -- make the SEx psf Call
MP_SEx = NCPU
execution_mode_SExpsf = EXECUTION_MODE

# 9. call_psfex -- Run psfex
execution_mode_psfex = EXECUTION_MODE
cleanupPSFcats = False

# 10. call_SExDual -- Run SExtractor un dual mode 
SExDual_parameters = {
    "MAG_ZEROPOINT":30,
    }
MP_SEx                 = NCPU
execution_mode_SExDual = EXECUTION_MODE

# Clean up files 
cleanupPSFcats = True
cleanupSWarp   = True
