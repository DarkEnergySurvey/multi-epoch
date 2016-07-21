#!/usr/bin/env python

#from despyastro import wcsutil
#import despydb
#import os,sys

from despymisc.miscutils import elapsed_time
from multiepoch.destiling import DEStiling
from despyastro import wcsutil

import os,sys
import time
import despydb

if __name__ == "__main__":

    #args   = cmdline()
    #kwargs = vars(args)

    
    # Inputs
    t0 = time.time()
    DBsection  = "db-destest"
    pixscale   = 0.263 
    NAXIS1     = 3000 
    NAXIS2     = 3000
    tablename  = 'felipe.coaddtile_new'
    ra_center  =  341.86159
    dec_center =  -44.965067

    t = DEStiling(pixscale, NAXIS1, NAXIS2, tileprefix='DES')
    t.ra_center   = ra_center
    t.dec_center  = dec_center

    # Prepare names
    t.get_tilename()
    #t.tilename = t.tilename+"_section"
    t.tilename = "DES2246-4457_section"
    
    # Make header
    t.create_header()

    # Create borders/edges
    nx = t.header['NAXIS1']
    ny = t.header['NAXIS2']
    wcs = wcsutil.WCS(t.header)
    RAC1,DECC1 = wcs.image2sky(1 , 1 )
    RAC2,DECC2 = wcs.image2sky(nx, 1 )
    RAC3,DECC3 = wcs.image2sky(nx, ny)
    RAC4,DECC4 = wcs.image2sky(1 , ny)
    t.ural  = min(RAC1,RAC2,RAC3,RAC4)
    t.urau  = max(RAC1,RAC2,RAC3,RAC4)
    t.udecl = min(DECC1,DECC2,DECC3,DECC4)
    t.udecu = max(DECC1,DECC2,DECC3,DECC4)
    t.crossRAzero = 'N'

    # Corners
    t.computeCornersTilename()

    # DB handle
    t.dbh = despydb.desdbi.DesDbi(section=DBsection)
    t.insertCOADDTILE(table=tablename)
    t.dbh.commit()
    print "\n"
    
