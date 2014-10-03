#!/usr/bin/env python

import os,sys
import multiepoch

"""
A test script to coadd a tilename contained in the SVA1 re-run
from Margaret
""" 

if __name__ == "__main__":

    try:
        TILENAME = sys.argv[1]
    except:
        TILENAME = 'DES2246-4457'

    # 0. Initialize class
    #outpath = os.path.join(os.getcwd(),'timing-tests')
    outpath = os.path.join(os.environ['HOME'],'tileTesting')
    c = multiepoch.DEScoadd(verbose=True,outpath=outpath)

    # 1. Grab the information of that TILENAME and put in an object
    c.getDESTILENAMEinfo(tilename=TILENAME)


    # 2. Find the CCDs that will fit inside TILENAME footprint
    # Extras to get only the ones from Margaret's test run
    and_extras = """image.run in ('20140421093250_20121207','20140421090850_20121124')
                    and image.IMAGETYPE='red'"""

    ######################################################################
    # Extras to match ZEROPOINT TABLE -- full, but slower
    # sel_extras =  """ distinct zeropoint.IMAGEID, zeropoint.MAG_ZERO """
    # from_extras = """ ZEROPOINT, IMAGE imagezp, RUNTAG"""
    # and_extras = and_extras + """ and imagezp.ID = zeropoint.IMAGEID and 
    #                 imagezp.IMAGENAME = image.IMAGENAME and 
    #                 imagezp.RUN       = runtag.RUN and 
    #                 runtag.tag        = 'SVA1_FINALCUT'"""
    #######################################################################
    ###################################
    # Faster using ZEROP in my table
    sel_extras =  " felipe.extraZEROPOINT.IMAGENAME, felipe.extraZEROPOINT.MAG_ZERO "
    from_extras = " felipe.extraZEROPOINT "
    and_extras = and_extras + " and felipe.extraZEROPOINT.IMAGENAME = image.IMAGENAME "
    ######################################
    c.findCCDinDESTILENAME(tilename=TILENAME,
                           and_constraints=and_extras,
                           from_constraints=from_extras,
                           select_constraints=sel_extras)

    # 3. (Optional)  Now we plot the tile + CCD inside - plot them all using subplot
    c.plot_CCDcornersDESTILEsubplot(fignumber=4)

    # 4. Collect the files for SWarp calls and others
    c.collectFILESforSWarp(archive_root='/home/felipe/work/NCSA_posters_images',prefix='imskTR_')

    # 5 . Prepare the SWarp call.
    c.makeSWarpCall(NTHREADS=8,COMBINE_TYPE="SUM",DETEC_COMBINE_TYPE="CHI-MEAN",PIXEL_SCALE=0.263)

    # 6. Create the color image call to stiff
    c.makeStiffCall(NTHREADS=8)

    # 7. Prepare the SEx for psfex call
    c.setCatNames()
    c.makeSExpsfCall(DETECT_MINAREA=10)

    # 8.
    c.makepsfexCall(NTHREADS=8)

    # 9.
    c.makeSExDualCall(MAG_ZEROPOINT=30)
