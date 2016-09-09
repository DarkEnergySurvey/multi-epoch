#!/usr/bin/env python

from despyastro import wcsutil
import despydb
import os,sys


section_desoper = "db-desoper"
dbh = despydb.desdbi.DesDbi(None,section_desoper)

pixelscale   = 0.263
ra_center   = 16.1620977018
dec_center  = -49.2416666667
table='felipe.coaddtile_new'
NAXIS1     = 5000
NAXIS2     = 5000
#NAXIS1     = 2500
#NAXIS2     = 2500
tilename = 'DES0102-4914_core'


header = {
    'NAXIS'   :  2,                    #/ Number of pixels along this axis
    'NAXIS1'  :  NAXIS1,          #/ Number of pixels along this axis
    'NAXIS2'  :  NAXIS2,          #/ Number of pixels along this axis
    'CTYPE1'  : 'RA---TAN',            #/ WCS projection type for this axis
    'CTYPE2'  : 'DEC--TAN',            #/ WCS projection type for this axis
    'CUNIT1'  : 'deg',                 #/ Axis unit
    'CUNIT2'  : 'deg',                 #/ Axis unit
    'CRVAL1'  :  ra_center,       #/ World coordinate on this axis
    'CRPIX1'  :  (NAXIS1+1)/2.0,      #/ Reference pixel on this axis
    'CD1_1'   :  -pixelscale/3600., #/ Linear projection matrix -- CD1_1 is negative
    'CD1_2'   :  0,                    #/ Linear projection matrix -- CD1_2 is zero, no rotation
    'CRVAL2'  :  dec_center,      #/ World coordinate on this axis
    'CRPIX2'  :  (NAXIS2+1)/2.0,      #/ Reference pixel on this axis
    'CD2_1'   :  0,                    #/ Linear projection matrix -- CD2_1 is zero, no rotation
    'CD2_2'   :  +pixelscale/3600.  #/ Linear projection matrix -- CD2_2 is positive
    }

# Now for consistecy, add lower-case keys, to make it 'case-insensity' fake
for k, v in header.items():
    header[k.lower()] = v
    

nx = header['NAXIS1']
ny = header['NAXIS2']
    
wcs = wcsutil.WCS(header)
RAC1,DECC1 = wcs.image2sky(1 , 1 )
RAC2,DECC2 = wcs.image2sky(nx, 1 )
RAC3,DECC3 = wcs.image2sky(nx, ny)
RAC4,DECC4 = wcs.image2sky(1 , ny)

ural  = min(RAC1,RAC2,RAC3,RAC4)
urau  = max(RAC1,RAC2,RAC3,RAC4)
udecl = min(DECC1,DECC2,DECC3,DECC4)
udecu = max(DECC1,DECC2,DECC3,DECC4)
crossRAzero = 'N'

columns = ('TILENAME',
           'RA',
           'DEC',
           'RAC1',
           'RAC2',
           'RAC3',
           'RAC4',
           'DECC1',
           'DECC2',
           'DECC3',
           'DECC4',
           'URAL',
           'URAU',
           'UDECL',
           'UDECU',
           'CROSSRAZERO',
           'PIXELSCALE',
           'NAXIS1',
           'NAXIS2',
           'CRPIX1',
           'CRPIX2',
           'CRVAL1',
           'CRVAL2',
           'CD1_1',
           'CD1_2',
           'CD2_1',
           'CD2_2')

values = (tilename,
          ra_center,
          dec_center,
          RAC1,
          RAC2,
          RAC3,
          RAC4,
          DECC1,
          DECC2,
          DECC3,
          DECC4,
          ural,
          urau,
          udecl,
          udecu,
          crossRAzero,
          pixelscale,
          header['NAXIS1'],
          header['NAXIS2'],
          header['CRPIX1'],
          header['CRPIX2'],
          header['CRVAL1'],
          header['CRVAL2'],
          header['CD1_1'],
          header['CD1_2'],
          header['CD2_1'],
          header['CD2_2'])

cols = ",".join(columns)
insert_cmd = "INSERT INTO %s (%s) VALUES %s" % (table,cols,values)
# Delete first:
delete_cmd = "DELETE from felipe.coaddtile_new where tilename='%s'" % tilename

cur = dbh.cursor()
sys.stdout.write("# Deleting  %s to %s\n" % (tilename,table))
cur.execute(delete_cmd)
sys.stdout.write("# Inserting %s to %s\n" % (tilename,table))
cur.execute(insert_cmd)
dbh.commit()
cur.close()
