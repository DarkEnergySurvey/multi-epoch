#!/usr/bin/env python

from multiepoch.DESfits import DESFITS, elapsed_time
import time
import sys

if __name__ == "__main__":

    # Get the start time
    t0 = time.time()
    
    fileName = sys.argv[1]
    outName  = sys.argv[2]
    clobber = False

    desfits = DESFITS(fileName,outName,clobber=clobber)
    #desfits.fix_weight_bleedtrail()
    #desfits.replace_weightBit(maskbit=4)

    # Replace bleed trails 2^6 = 64
    desfits.replace_weightBit(maskbit=64)

    # All interp pixels
    #desfits.replace_weightBit(maskbit=4)

    #desfits.write()
    desfits.write_weight()
    print >>sys.stderr,"# Time:%s" % elapsed_time(t0)
