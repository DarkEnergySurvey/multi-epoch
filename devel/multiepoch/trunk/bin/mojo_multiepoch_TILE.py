#!/usr/bin/env python

"""
Make sure we do:
 setup -v -r ~/DESDM-Code/devel/multiepoch/trunk
"""

from mojo import job_operator
import os,sys
import time
from despymisc.miscutils import elapsed_time

def cmdline():

    import argparse
    parser = argparse.ArgumentParser(description="Runs the DESDM multi-epoch pipeline")
    # The positional arguments
    parser.add_argument("tilename", action="store",default=None,
                        help="Name of the TILENAME")
    parser.add_argument("--db_section", action="store", default='db-destest',choices=['db-desoper','db-destest'],
                        help="DB Section to query")
    parser.add_argument("--tagname", action="store", default='Y2T_FIRSTCUT',
                        help="database TAG (i.e. Y2T_FIRSTCUT)")
    parser.add_argument("--exec_name", action="store", default='immask',
                        help="firstcut last exec_name (i.e. immask)")
    parser.add_argument("--runmode", action="store", default='dryrun',choices=['tofile','dryrun','execute'],
                        help="Mode to run the pipeline")
    parser.add_argument("--nthreads", action="store", default=6,
                        help="Number of threads to use in stiff/psfex/swarp")
    parser.add_argument("--ncpu", action="store", default=6,
                        help="Number of cpu to use in muti-process mode")
    parser.add_argument("--coaddtile_table", action="store",default='felipe.coaddtile_new',
                        help="Name of the table with coaddtile geometry")
    parser.add_argument("--local_desar", action="store",default=os.path.join(os.environ['HOME'],'LOCAL_DESAR'),
                        help="Name of LOCAL_DESAR repository (i.e. $HOME/LOCAL_DESAR)")
    parser.add_argument("--outputpath", action="store",default=os.path.join(os.environ['HOME'],'TILEBUILDER_DESDM'),
                        help="Path where we will write the outputs (i.e. $HOME/DESDM_TILEBUILDER)")
    args = parser.parse_args()
    return args
    
if __name__ == '__main__':

    args = cmdline()

    # Take time
    t0 = time.time()
    # 0. Initialize Job Operator
    jo  = job_operator.JobOperator(mojo_execution_mode='python')
    # 1.  Get the tile information from the table
    jo.run_job('multiepoch.tasks.query_tileinfo', tilename=args.tilename, coaddtile_table=args.coaddtile_table,db_section=args.db_section)
    # 2. Set up the output directory
    jo.run_job('multiepoch.tasks.set_tile_directory', outputpath=os.environ['HOME']+"/TILEBUILDER_TEST")
    # 3. Get the CCDs inside the tile
    # ---------------------------------------------------------------------
    # These are default extras for the full depth sample for SVA1
    # SELECT_EXTRAS = "felipe.extraZEROPOINT.MAG_ZERO,"
    # FROM_EXTRAS   = "felipe.extraZEROPOINT"
    # AND_EXTRAS    = "felipe.extraZEROPOINT.FILENAME = image.FILENAME"
    #
    # To select a subset of random CCDS based in on EXP (exposure) or CCD 
    SELECT_EXTRAS = "felipe.extraZEROPOINT.MAG_ZERO,"
    FROM_EXTRAS   = "felipe.extraZEROPOINT, felipe.TAGS"
    AND_EXTRAS    = """felipe.extraZEROPOINT.FILENAME = image.FILENAME and
    felipe.TAGS.FILENAME = image.FILENAME and
    felipe.TAGS.TAG = '%s_RAN_EXP'""" % args.tilename
    # -----------------------------------------------------------------
    jo.run_job('multiepoch.tasks.find_ccds_in_tile',
               tagname=args.tagname,
               exec_name=args.exec_name,
               and_extras=AND_EXTRAS,
               from_extras=FROM_EXTRAS)
    # 4a. Plot the corners -- all  bands (default)
    jo.run_job('multiepoch.tasks.plot_ccd_corners_destile')
    # 6. Retrieve the files -- if remotely
    jo.run_job('multiepoch.tasks.get_fitsfiles',local_archive=args.local_desar, http_section='http-desarchive')
    # 7 Create custom weights for SWarp
    jo.run_job('multiepoch.tasks.make_SWarp_weights',clobber_weights=False, MP_weight=args.ncpu, weights_execution_mode=args.runmode)
    # 8. The Custom call with custom weights 
    # Prepare call to SWarp
    swarp_params={
        "NTHREADS"     : args.nthreads,
        "COMBINE_TYPE" : "AVERAGE",    
        "PIXEL_SCALE"  : 0.263}
    jo.run_job('multiepoch.tasks.call_SWarp_CustomWeights',swarp_parameters=swarp_params, DETEC_COMBINE_TYPE="CHI-MEAN",swarp_execution_mode=args.runmode)
    # 9. Create the color images using stiff
    stiff_params={
        "NTHREADS"  : args.nthreads,
        "COPYRIGHT" : "NCSA/DESDM",
        "WRITE_XML" : "N"}
    jo.run_job('multiepoch.tasks.call_Stiff',stiff_parameters=stiff_params, stiff_execution_mode=args.runmode)
    # 10. make the SEx psf Call
    jo.run_job('multiepoch.tasks.call_SExpsf',SExpsf_execution_mode=args.runmode,MP_SEx=args.ncpu)
    # 11. Run  psfex
    jo.run_job('multiepoch.tasks.call_psfex',psfex_parameters={"NTHREADS": args.nthreads,},psfex_execution_mode=args.runmode)
    # 12. Run SExtractor un dual mode
    jo.run_job('multiepoch.tasks.call_SExDual',SExDual_parameters={"MAG_ZEROPOINT":30,}, SExDual_execution_mode=args.runmode,MP_SEx=args.ncpu)
    # 13. Create the MEF fits files in the formar we like
    if args.runmode == 'execute':
        jo.run_job('multiepoch.tasks.make_MEFs',clobber_MEF=False)
    print "# Grand Total time: %s" % elapsed_time(t0)
