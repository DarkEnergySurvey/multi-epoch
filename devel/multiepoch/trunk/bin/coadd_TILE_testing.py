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

    #outpath = os.path.join(os.getcwd(),'timing-tests')
    outpath = os.path.join(os.environ['HOME'],'tileTesting')
    
    c = multiepoch.DEScoadd(verbose=True,outpath=outpath)
    c.getDESTILENAMEinfo(tilename=TILENAME)
    
    # Extras to get only the ones from Margaret's test run
    and_extras = """(image.run='20140421093250_20121207' or image.run='20140421090850_20121124')
                 and image.IMAGETYPE='red'"""
    ######################################
    # Find the CCDs that fit inside
    c.findCCDinDESTILENAME(tilename=TILENAME,
                           and_constraints=and_extras)
    #####################################
    # Now we plot the tile + CCD inside
    # Plot them all using subplot
    c.plot_CCDcornersDESTILEsubplot(fignumber=4)
    c.collectFILESforSWarp(archive_root='/home/felipe/work/NCSA_posters_images',prefix='imskTR_')
    c.makeSWarpTileCall(NTHREADS=8,COMBINE_TYPE="SUM",PIXEL_SCALE=0.500)
    c.makeSWarpDetecCall()
    c.makeSExpsfCall(DETECT_MINAREA=10)
    c.makepsfexCall(PSF_SIZE="30,30")
    c.makeSExDualCall(MAG_ZEROPOINT=31)
