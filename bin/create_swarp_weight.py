#!/usr/bin/env python

import os,sys
from multiepoch.DESfits import DESFITS
from despymisc.miscutils import elapsed_time

if __name__ == "__main__":

    # Get the start time
    t0 = time.time()
    fileName = sys.argv[1]
    outName  = sys.argv[2]
    clobber = False
    desfits = DESFITS(fileName,outName,clobber=False)
    # Replace bleed trails 2^6 = 64
    desfits.replace_weightBit(maskbit=64)
    desfits.write_weight()
    print "# Done in %s\n" % elapsed_time(t0)
    return 
