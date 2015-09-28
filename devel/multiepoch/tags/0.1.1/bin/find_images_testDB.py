#!/usr/bin/env python

import os,sys
import astrometry
import despydb
from despyastro import wcsutil

def main():

    """ Simple scripts to get the images that made it into a tile """
    
    # Setup desar queries here
    section = "db-destest"
    try:
        desdmfile = os.environ["des_services"]
    except KeyError:
        desdmfile = None
    dbh = despydb.desdbi.DesDbi(desdmfile,section)
    cur = dbh.cursor()

    wcskeys = ["BAND",
               "TILENAME",
               "CCDNUM",
               "AIRMASS",
               "EXPTIME",
               "DEVICE_ID",
               "GAINA",
               "RDNOISEA",
               "GAINB",
               "RDNOISEB",
               "EQUINOX",
               "WCSDIM",
               "CTYPE1",
               "CUNIT1",
               "CRVAL1",
               "CRPIX1",
               "CD1_1",
               "CD1_2",
               "PV1_1",
               "PV1_2",
               "PV1_3",
               "PV1_4",
               "PV1_5",
               "PV1_6",
               "PV1_7",
               "PV1_8",
               "PV1_9",
               "PV1_10",
               "CTYPE2",
               "CUNIT2",
               "CRVAL2",
               "CRPIX2",
               "CD2_1",
               "CD2_2",
               "PV2_1",
               "PV2_2",
               "PV2_3",
               "PV2_4",
               "PV2_5",
               "PV2_6",
               "PV2_7",
               "PV2_8",
               "PV2_9",
               "PV2_10",
               "SKYBRITE",
               "SKYSIGMA",
               "ELLIPTIC",
               "FWHM",
               "SCAMPNUM",
               "SCAMPCHI",
               "SCAMPFLG",
               "PV1_0",
               "PV2_0",
               "NAXIS1",
               "NAXIS2",
               "NEXTEND",
               "RA",
               "DEC",
               "PCOUNT",
               "GCOUNT",
               "BZERO",
               "BSCALE",
               "BUNIT",
               "CCDNAME",
               "DETSEC",
               "CCDSUM",
               "DATASEC",
               "TRIMSEC",
               "AMPSECA",
               "BIASSECA",
               "SATURATA",
               "SATURATB",
               "LTM1_1",
               "LTM2_2",
               "LTV1",
               "LTV2",
               "RADESYS",
               "WCSAXES",
               "AMPSECB",
               "BIASSECB",
               "ZP",
               "ZERO",
               "SKY",
               "SEESIGA",
               "PSFSCALE",
               "PSF_FWHM",
               "PSF_BETA",
               "SATURATE",
               "NOBJECT_SCAMP",
               "NITE",
               "EXPNUM",
               "CAMSYM",
               "RAC1",
               "DECC1",
               "RAC2",
               "DECC2",
               "RAC3",
               "DECC3",
               "RAC4",
               "DECC4"]
    

    query_list = ",".join(wcskeys) + ',exec_name, wgb.filename'
    query      = """select %s
                    from image, wgb
                    where image.filename=wgb.filename and reqnum=60 and exec_name='maskcosmics'
                    order by ccdnum,wrapnum """ % query_list

    print query
    cur.execute(query)
    desc = [d[0].lower() for d in cur.description]
    result = []
    for line in cur:
        d = dict(zip(desc, line))
        result.append(d)
    cur.close()

    for header in result:
        x = header['naxis1']/2.0
        y = header['naxis2']/2.0
        wcs = wcsutil.WCS(header)
        ra,dec = wcs.image2sky(x,y)
        print ra,dec,header['tilename'],header['ctype1'],header['ctype2']
        sys.exit()
    
#print "# SQL query done in:  %s" % elapsed_time(t0)
main()
