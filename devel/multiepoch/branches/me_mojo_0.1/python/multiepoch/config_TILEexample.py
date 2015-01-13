
import os


# JOBS TO BE RUN
jobs = [
        'multiepoch.jobs.query_tileinfo',
        'multiepoch.jobs.set_tile_directory',
        'multiepoch.jobs.find_ccds_in_tile',
        'multiepoch.jobs.plot_ccd_corners_destile',
        'multiepoch.jobs.find_fitsfiles_location', 
        'multiepoch.jobs.get_fitsfiles',
        'multiepoch.jobs.make_SWarp_weights',
        'multiepoch.jobs.call_SWarp_CustomWeights',
        'multiepoch.jobs.call_Stiff',
        'multiepoch.jobs.set_catNames',
        'multiepoch.jobs.call_SExpsf',
        'multiepoch.jobs.call_psfex',
        'multiepoch.jobs.call_SExDual',
        'multiepoch.jobs.make_MEFs',
        ]

# the database handler
try:
    from despydb import desdbi
    dbh = desdbi.DesDbi(section='db-destest')
except:
    print 'no connection to the database.'


# PARAMETERS
verbose = True

# query_tileinfo
#tilename = 'DES2246-4457'
tilename = 'DES0445-4623'
coaddtile_table = 'felipe.coaddtile_new'

# set_tile_directory
outputpath = os.environ['HOME']+"/TILEBUILDER_TEST"

# find_ccds_in_tile
tagname = 'Y2T1_FIRSTCUT'
exec_name = 'immask'
select_extras = "felipe.extraZEROPOINT.MAG_ZERO,"
from_extras = "felipe.extraZEROPOINT"
and_extras = "felipe.extraZEROPOINT.FILENAME = image.FILENAME" 

# find_fitsfiles_location
archive_name = 'desar2home'

# get_fitsfiles
local_archive = os.path.join(os.environ['HOME'],'LOCAL_DESAR')

# make_SWarp_weights
clobber_weights = False
MP_weight = 4

# call_SWarp_CustomWeights
swarp_params = {
    "NTHREADS"     :8,
    "COMBINE_TYPE" : "AVERAGE",    
    "PIXEL_SCALE"  : 0.263,
    }
DETEC_COMBINE_TYPE = "CHI-MEAN"
swarp_execution_mode = 'execute'

# call_Stiff
stiff_params = {
    "NTHREADS"  :8,
    "COPYRIGHT" : "NCSA/DESDM",
    "WRITE_XML" : "N",
    }
stiff_execution_mode='execute'

# call_SExpsf
SExpsf_execution_mode = 'execute'
MP_SEx = 8

# call_psfex
psfex_parameters = { 
    "NTHREADS"  :8,
    }
psfex_execution_mode = 'execute'

# call_SExDual
SExDual_parameters = { 
    "MAG_ZEROPOINT":30,
    }
SExDual_execution_mode = 'execute'
MP_SEx = 8

# make_MEFs
clobber_MEF=False
