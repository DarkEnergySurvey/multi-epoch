#!/usr/bin/env python

import time
from despymisc.miscutils import elapsed_time
from multiepoch.destiling import DEStiling

"""
Example:
  gencoaddtile_newcorners.py --sectionDB db-destest --writeDB
  gencoaddtile_newcorners.py --sectionDB db-desoper --writeDB
"""

def cmdline():
    import argparse
    parser = argparse.ArgumentParser(description="Generate the corners for a COADDTILE table in the new" + \
                                     "schema and inserts the values into the DB\n")
    # The positional arguments
    parser.add_argument("--tablename", action="store",default='felipe.coaddtile_new',
                        help="Name of the COADDTILE table [i.e. felipe.coaddtile_new]")
    parser.add_argument("--sectionDB", action="store", default='db-desoper',choices=['db-desoper','db-destest'],
                        help="DB Section to write COADDTILE table to")
    parser.add_argument("--writeDB", action="store_true", default=False,
                        help="Write the corners into the DB [default=False]")
    parser.add_argument("--checkDB", action="store_true", default=False,
                        help="Compare values against DB (slow) [default=False]")
    args = parser.parse_args()
    print "# Will run:"
    print "# %s " % parser.prog
    for key in vars(args):
        print "# \t--%-10s\t%s" % (key,vars(args)[key])
    return args

if __name__ == "__main__":

    args   = cmdline()
    kwargs = vars(args)

    t0 = time.time()
    pixscale = 0.263 # Do not change unless you know! 
    NAXIS1   = 10000 # Do not change unless you know! 
    NAXIS2   = 10000 # Do not change unless you know !
    tiles = DEStiling(pixscale, NAXIS1, NAXIS2, tileprefix='DES')
    tiles.generateTiles(ra_ini     = 275, # do not edit or tiles will change
                        dec_ini    = +30, # do not edit or tiles will change
                        dec_end    = -85,
                        ra_range   = 360,
                        **kwargs)
    print "# Time:%s" % elapsed_time(t0)
