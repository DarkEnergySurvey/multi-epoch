#!/usr/bin/env python

"""
Make sure we do:
 setup -v -r ~/DESDM-Code/devel/multiepoch/trunk
"""

from mojo import job_operator
import os,sys
import time
from despymisc.miscutils import elapsed_time
import multiepoch.utils as utils
import multiepoch.tasks.find_ccds_in_tile 

SELECT_EXTRAS = multiepoch.tasks.find_ccds_in_tile.SELECT_EXTRAS
FROM_EXTRAS   = multiepoch.tasks.find_ccds_in_tile.FROM_EXTRAS
AND_EXTRAS    = multiepoch.tasks.find_ccds_in_tile.AND_EXTRAS
CLOBBER_ME = False
XBLOCK = 10
ADD_NOISE = True

def cmdline():
    
    # SETTING UP THE PATHS
    # -----------------------------------------------------------------------------
    # The only REQUIRED PIPELINE PARAMETERS are:
    #  local_archive
    #  local_archive_me 
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

    outputpath           = os.path.join(MULTIEPOCH_ROOT, 'TILEBUILDER') 
    if utils.inDESARcluster():
        local_archive        = "/work/prodbeta/archive"
    else:
        local_archive        = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE')
    local_archive_me = os.path.join(MULTIEPOCH_ROOT, 'LOCAL_ARCHIVE_ME')

    import argparse
    parser = argparse.ArgumentParser(description="Runs the DESDM multi-epoch pipeline")
    # The positional arguments
    parser.add_argument("tilename", action="store",default=None,
                        help="Name of the TILENAME")
    # Path-related arguments
    parser.add_argument("--local_archive", action="store", default=local_archive,
                        help="Name of local archive repository [default: $MULTIEPOCH_ROOT/LOCAL_ARCHIVE]")
    parser.add_argument("--local_archive_me", action="store", default=local_archive_me,
                        help="Name of local archive repository [default: $MULTIEPOCH_ROOT/LOCAL_WEIGHTS]")
    parser.add_argument("--outputpath", action="store",default=outputpath,
                        help="Path where we will write the outputs [default: $MULTIEPOCH_ROOT/TILEBUILDER]")
    parser.add_argument("--tiledir", action="store",default=None,
                        help="Path where we will write the outputs, overides --outputpath (i.e. $MULTIEPOCH_ROOT/TILEBUILDER/tilename)")
    # Run options
    parser.add_argument("--runmode", action="store", default='dryrun',choices=['tofile','dryrun','execute'],
                        help="Mode to run the pipeline")
    parser.add_argument("--nthreads", action="store", default=6,
                        help="Number of threads to use in stiff/psfex/swarp")
    parser.add_argument("--ncpu", action="store", default=6,
                        help="Number of cpu to use in muti-process mode")
    parser.add_argument("--cleanup", action="store_true",default=False,
                        help="Clean up SWarp and psfcat fits files?")
    parser.add_argument("--keep", action="store_true",default=False,
                        help="Keep SWarp and psfcat fits files?")
    parser.add_argument("--doBANDS", action="store",default=['all'], nargs='+',
                        help="BANDS to processs (default=all)")
    # Query options
    parser.add_argument("--db_section", action="store", default='db-destest',choices=['db-desoper','db-destest'],
                        help="DB Section to query")
    parser.add_argument("--http_section", action="store", default='http-desarchive',
                        help="Name of section on desservices file to use for file transfer")
    parser.add_argument("--tagname", action="store", default='Y2T_FIRSTCUT',
                        help="database TAG (i.e. Y2T_FIRSTCUT)")
    parser.add_argument("--exec_name", action="store", default='immask', # REVIEW ASK FILETYPES HAVE CHANGED
                        help="firstcut last exec_name (i.e. immask)")
    parser.add_argument("--coaddtile_table", action="store",default='felipe.coaddtile_new',  # NEED TO BE REVISED
                        help="Name of the table with coaddtile geometry")
    # and/select/from extras
    parser.add_argument("--select_extras", action="store",default=SELECT_EXTRAS,
                        help="string with extra SELECT for query")
    parser.add_argument("--and_extras", action="store",default=AND_EXTRAS,
                        help="string with extra AND for query")
    parser.add_argument("--from_extras", action="store",default=FROM_EXTRAS,
                        help="string with extra FROM for query")
    # me-prepare options
    parser.add_argument("--clobber_me", action="store_true",default=CLOBBER_ME,
                        help="Clobber existing me-prepared files?")
    # SWarp options
    parser.add_argument("--COMBINE_TYPE", action="store",default='WEIGHTED',
                        help="SWarp COMBINE_TYPE for band coadds")
    parser.add_argument("--COMBINE_TYPE_detec", action="store",default='CHI-MEAN',
                        help="SWarp COMBINE_TYPE for detection coadds")
    parser.add_argument("--weight_for_mask", action="store_true",default=False,
                        help="Create extra coadded weight for mask creation")

    # More optional args to bypass queries for tileinfo and geometry
    parser.add_argument("--tile_geom_input_file", action="store",default='',
                        help="The json file with the tile information (default='')")
    parser.add_argument("--assoc_file", action="store",default='',
                        help="Input association file with CCDs information (default=''")

    args = parser.parse_args()

    # Sanity checks
    if (args.cleanup is True) and (args.keep is True):
        sys.exit("ERROR: clean=yes and keep=yes are in conflict")

    # Setup args.tiledir if not setup by command-line
    if not args.tiledir:
        args.tiledir = os.path.join(args.outputpath, args.tilename)

    # Update execution modes for individual tasks 
    # me_prepare
    args.MP_me = args.ncpu
    args.execution_mode_me = args.runmode
    # SWarp
    args.execution_mode_swarp = args.runmode
    return args
    
if __name__ == '__main__':

    args   = cmdline()
    kwargs = vars(args)

    # Take time
    t0 = time.time()

    # 0. Initialize Job Operator
    jo  = job_operator.JobOperator(mojo_execution_mode='python',
                                   stdoutloglevel = 'DEBUG',
                                   fileloglevel = 'DEBUG',
                                   logfile = os.path.join(args.tiledir, args.tilename+'_full_pipeline.log'),
                                   **kwargs) 

    # 1.  Get the tile information from the table -- unless provided
    if args.tile_geom_input_file == '':
        jo.run_job('multiepoch.tasks.query_tileinfo', **kwargs) #tilename=args.tilename, coaddtile_table=args.coaddtile_table,db_section=args.db_section)
    else:
        jo.logger.info("Skipping tasks.query_tileinfo, will loaad assoc file:%s" % args.tile_geom_input_file)
        
    # 2. Get the CCDs inside the tile -- unless provided
    if args.assoc_file == '':
        jo.run_job('multiepoch.tasks.find_ccds_in_tile',**kwargs)
        # 2b. Plot the corners -- all  bands (default)
        jo.run_job('multiepoch.tasks.plot_ccd_corners_destile',**kwargs)
    else:
        jo.logger.info("Skipping task tasks.find_ccds_in_tile, will loaad assoc file:%s" % args.assoc_file)

    # 3. Retrieve the files -- if not running on cosmology cluster
    jo.run_job('multiepoch.tasks.get_fitsfiles',**kwargs)

    # 4. Prepare single-epoch input for coadd (performs interpolation and null_weights)
    jo.run_job('multiepoch.tasks.me_prepare',**kwargs)

    # 5. The SWarp call 
    swarp_pars={"PIXEL_SCALE": 0.263}
    jo.run_job('multiepoch.tasks.call_SWarp',swarp_parameters=swarp_pars,**kwargs)
               
    # Now we need to combine the 3 planes SCI/WGT/MSK into a single image
    jo.run_job('multiepoch.tasks.call_coadd_merge',clobber_MEF=True,MEF_execution_mode=args.runmode,cleanupSWarp=args.cleanup,add_noise=ADD_NOISE,xblock=XBLOCK)

    # 6. Create the color images using stiff
    stiff_params={"NTHREADS"  : args.nthreads,}
    jo.run_job('multiepoch.tasks.call_Stiff',tilename=args.tilename, stiff_parameters=stiff_params, stiff_execution_mode=args.runmode)
    
    # 7. make the SEx psf Call
    jo.run_job('multiepoch.tasks.call_SExpsf',tilename=args.tilename, SExpsf_execution_mode=args.runmode,MP_SEx=args.ncpu)

    # 8. Run  psfex
    jo.run_job('multiepoch.tasks.call_psfex',psfex_parameters={"NTHREADS": args.nthreads,},psfex_execution_mode=args.runmode)

    # 9. Run SExtractor un dual mode
    jo.run_job('multiepoch.tasks.call_SExDual',SExDual_parameters={"MAG_ZEROPOINT":30,}, SExDual_execution_mode=args.runmode,MP_SEx=args.ncpu,cleanupPSFcats=args.cleanup)
    
    print "# Grand Total time: %s" % elapsed_time(t0)
