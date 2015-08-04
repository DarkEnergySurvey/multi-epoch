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
    
    # SETTING UP THE PATHS
    # -----------------------------------------------------------------------------
    # The only REQUIRED PIPELINE PARAMETERS are:
    #  local_archive
    #  weights_archive 
    #  tiledir
    # and they have to be set to run the pipeline. They CAN BE SET INDEPENDENTLY.
    #
    # MULTIEPOCH_ROOT and outputpath are simply supportive,
    # non-required organisational variables.
    # ------------------------------------------------------------------------
    if os.environ.get('MULTIEPOCH_ROOT'):
        MULTIEPOCH_ROOT = os.environ['MULTIEPOCH_ROOT']
    elif os.environ.get('HOME'):
        MULTIEPOCH_ROOT = os.path.join(os.environ['HOME'],'MULTIEPOCH_ROOT')
    else:
        print "# Warning $HOME is not defined, will use ./ instead"
        MULTIEPOCH_ROOT = os.path.abspath('/MULTIEPOCH_ROOT')

    outputpath      = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 
    local_archive   = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE')
    local_weights   = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_WEIGHTS')

    import argparse
    parser = argparse.ArgumentParser(description="Runs the DESDM multi-epoch pipeline")
    # The positional arguments
    parser.add_argument("tilename", action="store",default=None,
                        help="Name of the TILENAME")

    # Optional arguments
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
    parser.add_argument("--cleanup", action="store_true",default=False,
                        help="Clean up SWarp and psfcat fits files?")
    parser.add_argument("--keep", action="store_true",default=False,
                        help="Keep SWarp and psfcat fits files?")
    parser.add_argument("--custom_weights", action="store_true",default=False,
                        help="Use custom weights for SWarp coaddition")
    # Optional path-related arguments
    parser.add_argument("--local_archive", action="store", default=local_archive,
                        help="Name of local archive repository [default: $MULTIEPOCH_ROOT/LOCAL_ARCHIVE]")
    parser.add_argument("--local_weights", action="store", default=local_weights,
                        help="Name of local archive repository [default: $MULTIEPOCH_ROOT/LOCAL_WEIGHTS]")
    parser.add_argument("--outputpath", action="store",default=outputpath,
                        help="Path where we will write the outputs [default: $MULTIEPOCH_ROOT/TILEBUILDER]")
    parser.add_argument("--tiledir", action="store",default=None,
                        help="Path where we will write the outputs, overides --outputpath (i.e. $MULTIEPOCH_ROOT/TILEBUILDER/tilename)")

    args = parser.parse_args()

    # Sanity check
    if (args.cleanup is True) and (args.keep is True):
        sys.exit("ERROR: clean=yes and keep=yes are in conflict")

    # Setup args.tiledir if not setup by command-line
    if not args.tiledir:
        args.tiledir = os.path.join(args.outputpath, args.tilename)

    return args
    
if __name__ == '__main__':

    args = cmdline()

    # Take time
    t0 = time.time()
    # 0. Initialize Job Operator
    jo  = job_operator.JobOperator(mojo_execution_mode='python',
                                   stdoutloglevel = 'DEBUG',
                                   fileloglevel = 'DEBUG',
                                   #logfile = os.path.join(tiledir, tilename+'_full_pipeline.log')
                                   )
    # 1.  Get the tile information from the table
    jo.run_job('multiepoch.tasks.query_tileinfo', tilename=args.tilename, coaddtile_table=args.coaddtile_table,db_section=args.db_section)

    # 2. Get the CCDs inside the tile
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

    # 3. Plot the corners -- all  bands (default)
    jo.run_job('multiepoch.tasks.plot_ccd_corners_destile',tiledir=args.tiledir)

    # 4. Retrieve the files -- if remotely
    jo.run_job('multiepoch.tasks.get_fitsfiles',local_archive=args.local_archive, http_section='http-desarchive')

    # 5 Create custom weights for SWarp (optional)
    if args.custom_weights:
        jo.run_job('multiepoch.tasks.make_SWarp_weights',clobber_weights=False, MP_weight=args.ncpu, local_weights=args.local_weights,weights_execution_mode=args.runmode)

    # 6. The SWarp call 
    swarp_params={
        "NTHREADS"     : args.nthreads,
        "COMBINE_TYPE" : "AVERAGE",    
        "PIXEL_SCALE"  : 0.263}
    jo.run_job('multiepoch.tasks.call_SWarp',swarp_parameters=swarp_params, DETEC_COMBINE_TYPE="CHI-MEAN",
               swarp_execution_mode=args.runmode,
               custom_weights=args.custom_weights)

    # 7. Create the color images using stiff
    stiff_params={"NTHREADS"  : args.nthreads,}
    jo.run_job('multiepoch.tasks.call_Stiff',stiff_parameters=stiff_params, stiff_execution_mode=args.runmode)

    # 8. make the SEx psf Call
    jo.run_job('multiepoch.tasks.call_SExpsf',SExpsf_execution_mode=args.runmode,MP_SEx=args.ncpu)

    # 9. Run  psfex
    jo.run_job('multiepoch.tasks.call_psfex',psfex_parameters={"NTHREADS": args.nthreads,},psfex_execution_mode=args.runmode,cleanupPSFcats=args.cleanup)

    # 10. Run SExtractor un dual mode
    jo.run_job('multiepoch.tasks.call_SExDual',SExDual_parameters={"MAG_ZEROPOINT":30,}, SExDual_execution_mode=args.runmode,MP_SEx=args.ncpu)
    
    # 11. Create the MEF fits files in the formar we like
    jo.run_job('multiepoch.tasks.make_MEFs',clobber_MEF=False,MEF_execution_mode=args.runmode,cleanupSWarp=args.cleanup)
    print "# Grand Total time: %s" % elapsed_time(t0)
