#!/usr/bin/env python

import os,sys
import multiepoch

tiles_RXJ2248 = ['DES2251-4331',
                 'DES2251-4414',
                 'DES2254-4457',
                 'DES2247-4331',
                 'DES2247-4414',
                 'DES2246-4457',
                 'DES2250-4457']

tiles_ElGordo = ['DES0105-4831',
                 'DES0059-4957',
                 'DES0103-4957',
                 'DES0058-4914',
                 'DES0102-4914',
                 'DES0106-4914',
                 'DES0101-4831']

def doTILES(tiles,outpath):

    for TILENAME in tiles:

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
        #c.collectFILESforSWarp(archive_root='/home/felipe/work/NCSA_posters_images',prefix='imsk_')
        c.collectFILESforSWarp(archive_root='/home/felipe/work/NCSA_posters_images',prefix='imskTR_')
        c.makeSWarpTilecall()

    print "End of loop of tiles"
    return

if __name__ == "__main__":
    
    outpath = os.path.join(os.getcwd(),'TESTING_RXJ2248_TR')
    doTILES(tiles_RXJ2248,outpath)

    sys.exit()
    
    outpath = os.path.join(os.getcwd(),'TESTING_ElGordo_TR')
    doTILES(tiles_RXJ2248,outpath)


    # SVA1 query
    # Check to make sure we match the SVA1 tag
    SVA1_and_extras = """image.RUN=runtag.RUN and
                         image.IMAGETYPE='red' and
                         runtag.TAG='SVA1_FINALCUT'"""
    SVA1_from_extras = 'RUNTAG'
    c.findCCDinDESTILENAME(tilename='DES0102-4914',
                        and_constraints =SVA1_and_extras,
                        from_constraints=SVA1_from_extras)


    # Plotting tests
    #TILENAME = 'DES0102-4914_core'
    #TILENAME = 'DES0102-4914'
    ##############################
    # Plot one a a time
    #c.plot_CCDcornersDESTILEsingle(BAND='g',fignumber=1)
    #c.plot_CCDcornersDESTILEsingle(BAND='r',fignumber=2)
    #c.plot_CCDcornersDESTILEsingle(BAND='i',fignumber=3)
    # plt.show()
    
