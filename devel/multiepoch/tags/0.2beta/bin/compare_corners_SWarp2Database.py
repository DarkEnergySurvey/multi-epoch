#!/usr/bin/env python

"""
Simple script to compare the corners from a SWarped file withe entries in the DB
"""

import os,sys
from pyfits import getheader
from despyastro import wcsutil

def get_corners_filename(fileName):

    # Get the header of the fitsfile
    header = getheader(fileName)
    nx = header['NAXIS1']
    ny = header['NAXIS2']

    wcs = wcsutil.WCS(header)
    RAC1,DECC1 = wcs.image2sky(1 , 1 )
    RAC2,DECC2 = wcs.image2sky(nx, 1 )
    RAC3,DECC3 = wcs.image2sky(nx, ny)
    RAC4,DECC4 = wcs.image2sky(1 , ny)

    print "---------------------------------------------------"
    print "Corners in file: %s" % fileName
    print "RAC1: %16.10f -- DECC1: %16.10f" % (RAC1,DECC1)
    print "RAC2: %16.10f -- DECC2: %16.10f" % (RAC2,DECC2)
    print "RAC3: %16.10f -- DECC3: %16.10f" % (RAC3,DECC3)
    print "RAC4: %16.10f -- DECC4: %16.10f" % (RAC4,DECC4)
    print "---------------------------------------------------"
    return RAC1,DECC1, RAC2,DECC2, RAC3,DECC3, RAC4,DECC4


def get_corners_DB(tileName, tableName = 'felipe.coaddtile_new'):
    
    from multiepoch.destiling import DEStiling
    
    pixscale = 0.263 # Do not change unless you know 
    NAXIS1   = 10000 # Do not change unless you know 
    NAXIS2   = 10000 # Do not change unless you know 
    tiles = DEStiling(pixscale, NAXIS1, NAXIS2, tileprefix='DES')
    tiles.connectDB(section='db-desoper')
    cur = tiles.dbh.cursor()

    query = """select RAC1,DECC1, RAC2,DECC2, RAC3,DECC3, RAC4,DECC4
               from %s where TILENAME='%s'""" % (tableName,tileName)
    cur.execute(query)
    (RAC1,DECC1, RAC2,DECC2, RAC3,DECC3, RAC4,DECC4) = cur.fetchone()

    print "---------------------------------------------------"
    print "Corners in table: %s" % tableName
    print "RAC1: %16.10f -- DECC1: %16.10f" % (RAC1,DECC1)
    print "RAC2: %16.10f -- DECC2: %16.10f" % (RAC2,DECC2)
    print "RAC3: %16.10f -- DECC3: %16.10f" % (RAC3,DECC3)
    print "RAC4: %16.10f -- DECC4: %16.10f" % (RAC4,DECC4)
    print "---------------------------------------------------"
    return RAC1,DECC1, RAC2,DECC2, RAC3,DECC3, RAC4,DECC4

if __name__ == "__main__":

    try:
        tileName = sys.argv[1]
        fileName = sys.argv[2]
    except:

        prog = os.path.basename(sys.argv[0])
        USAGE="""
        Simple script to compare the corners from a SWarped file withe entries in the DB
        USAGE: %s <filename> <tilename>\n""" % prog
        sys.exit(USAGE)

    # Get the corners via two methods
    corner1 = get_corners_filename(fileName)
    corner2 = get_corners_DB(tileName)

    # Comparing
    for k in range(4):
        dRA  = corner1[k]  - corner2[k]
        dDEC = corner1[k+1]- corner2[k+1]
        print " Corner%d: dRA=%18s, dDEC=%18s" % (k+1, dRA, dDEC)
        
        

    

