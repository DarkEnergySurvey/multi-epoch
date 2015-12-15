#!/usr/bin/env python

from multiepoch import tileinfo_utils
from despyastro import astrometry

if __name__ == "__main__":

    import argparse
    parser = argparse.ArgumentParser(description="Generate the corners for a COADDTILE table in the new" + \
                                     "schema and inserts the values into the DB\n")
    parser.add_argument("ra_center", action="store", type=float, default=None, 
                        help="RA Center of the TILE")
    parser.add_argument("dec_center", action="store", type=float, default=None,
                        help="DEC Center of the TILE")
    parser.add_argument("--tilename", action="store",default=None,
                        help="Name of the tile that you want to build")
    parser.add_argument("--xsize", action="store", type=float, default=10,
                        help="X-size of output tile in arcmin")
    parser.add_argument("--ysize", action="store", type=float, default=10,
                        help="Y-size of output tile in arcmin")
    parser.add_argument("--json_file", action="store", default=None, 
                        help="Y-size of output tile in arcmin")
    parser.add_argument("--pixelscale", action="store", default=0.263,
                        help="Pixel Scale in arcsec/pixel")
    parser.add_argument("--prefix", action="store",default='DES',
                        help="Prefix to use for TILENAME")
    args = parser.parse_args()

    # Create a tilename
    if not args.tilename:
        TILENAME = "{prefix}J{ra}{dec}"
        ra  = astrometry.dec2deg(args.ra_center/15.,sep="",plussign=False)
        dec = astrometry.dec2deg(args.dec_center,   sep="",plussign=True)
        args.tilename = TILENAME.format(prefix=args.prefix,ra=ra,dec=dec)

    if not args.json_file:
        args.json_file  = "%s.json" % args.tilename

    kw = {'xsize' : args.xsize,
          'ysize' : args.ysize,
          'pixelscale' : args.pixelscale,
          'ra_cent'    : args.ra_center,
          'dec_cent'   : args.dec_center,
          'json_file'  : args.json_file,
          }
    tileinfo_utils.define_tileinfo(args.tilename,**kw)
          
