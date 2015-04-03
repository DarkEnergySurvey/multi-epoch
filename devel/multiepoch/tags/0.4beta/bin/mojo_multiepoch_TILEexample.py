#!/usr/bin/env python

"""
Make sure we do:
 setup -v -r ~/DESDM-Code/devel/multiepoch/trunk
"""

from mojo import job_operator
import os,sys
import time
from despymisc.miscutils import elapsed_time

# We want to make command-line out of these
RUNMODE  = 'execute' # dryrun/tofile/execute
TILENAME = 'DES2246-4457'

# Take time
t0 = time.time()

# 0. Initialize Job Operator
jo  = job_operator.JobOperator(mojo_execution_mode='python')

# 1.  Get the tile information from the table
jo.run_job('multiepoch.tasks.query_tileinfo', tilename=TILENAME, coaddtile_table='felipe.coaddtile_new',db_section='db-destest')

# 2. Set up the output directory
jo.run_job('multiepoch.tasks.set_tile_directory', outputpath=os.environ['HOME']+"/TILEBUILDER_TEST")

# 3. Get the CCDs inside the tile
# -------------------------------------------------
# The default extras for the full sample
SELECT_EXTRAS = "felipe.extraZEROPOINT.MAG_ZERO,"
FROM_EXTRAS   = "felipe.extraZEROPOINT"
AND_EXTRAS    = "felipe.extraZEROPOINT.FILENAME = image.FILENAME"

# To select a subset of random CCDS based in on EXP (exposure) or CCD 
SELECT_EXTRAS = "felipe.extraZEROPOINT.MAG_ZERO,"
FROM_EXTRAS   = "felipe.extraZEROPOINT, felipe.TAGS"
AND_EXTRAS    = """felipe.extraZEROPOINT.FILENAME = image.FILENAME and
                   felipe.TAGS.FILENAME = image.FILENAME and
                   felipe.TAGS.TAG = '%s_RAN_EXP'""" % TILENAME

# -------------------------------------------------
jo.run_job('multiepoch.tasks.find_ccds_in_tile',
           tagname='Y2T_FIRSTCUT',
           exec_name='immask',
           and_extras=AND_EXTRAS,
           from_extras=FROM_EXTRAS)

# 4a. Plot the corners -- all  bands (default)
jo.run_job('multiepoch.tasks.plot_ccd_corners_destile')

# 4b. Plot the corners -- single band
#jo.run_job('multiepoch.tasks.plot_ccd_corners_destile', band='r')
#jo.run_job('multiepoch.tasks.plot_ccd_corners_destile', band='i')

# 6. Retrieve the files -- if remotely
LOCAL_DESAR = os.path.join(os.environ['HOME'],'LOCAL_DESAR')
jo.run_job('multiepoch.tasks.get_fitsfiles',local_archive=LOCAL_DESAR, http_section='http-desarchive')

# 7 Create custom weights for SWarp
jo.run_job('multiepoch.tasks.make_SWarp_weights',clobber_weights=False, MP_weight=4, weight_execution_mode=RUNMODE)

# Prepare call to SWarp
swarp_params={
    "NTHREADS"     :8,
    "COMBINE_TYPE" : "AVERAGE",    
    "PIXEL_SCALE"  : 0.263}

# 8a. The simple call, no custom weights (deprecated?)
#jo.run_job('multiepoch.tasks.call_SWarp',swarp_parameters=swarp_params, DETEC_COMBINE_TYPE="CHI-MEAN",swarp_execution_mode='execute')
#jo.run_job('multiepoch.tasks.call_SWarp',swarp_parameters=swarp_params, DETEC_COMBINE_TYPE="CHI-MEAN",swarp_execution_mode='dryrun')

# 8b. The Custom call with custom weights 
jo.run_job('multiepoch.tasks.call_SWarp_CustomWeights',swarp_parameters=swarp_params, DETEC_COMBINE_TYPE="CHI-MEAN",swarp_execution_mode=RUNMODE)

# 9. Create the color images using stiff
stiff_params={
    "NTHREADS"  :8,
    "COPYRIGHT" : "NCSA/DESDM",
    "WRITE_XML" : "N"}
jo.run_job('multiepoch.tasks.call_Stiff',stiff_parameters=stiff_params, stiff_execution_mode=RUNMODE)


# 10. make the SEx psf Call
jo.run_job('multiepoch.tasks.call_SExpsf',SExpsf_execution_mode=RUNMODE,MP_SEx=8)

# 11. Run  psfex
jo.run_job('multiepoch.tasks.call_psfex',psfex_parameters={"NTHREADS"  :8,},psfex_execution_mode=RUNMODE)

# 12. Run SExtractor un dual mode
jo.run_job('multiepoch.tasks.call_SExDual',SExDual_parameters={"MAG_ZEROPOINT":30,}, SExDual_execution_mode=RUNMODE,MP_SEx=8)

# 13. Create the MEF fits files in the formar we like
if RUNMODE == 'execute':
    jo.run_job('multiepoch.tasks.make_MEFs',clobber_MEF=False)

print "# Grand Total time: %s" % elapsed_time(t0)
