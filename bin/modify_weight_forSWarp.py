#!/usr/bin/env python

from multiepoch.DESfits import DESFITS, elapsed_time
from despymisc.miscutils import elapsed_time
import time
import sys

if __name__ == "__main__":

    # Get the start time
    t0 = time.time()
    try:
        fileName = sys.argv[1]
        outName  = sys.argv[2]
    except:
        usage = """
        ERROR:
        USAGE: %s <filename> <outname>
        """ % os.path.basename(sys.argv[0])
        sys.exit(usage)

    desfits = DESFITS(fileName,outName,clobber=False)
    # Replace bleed trails 2^6 = 64
    desfits.replace_weightBit(maskbit=64)
    desfits.write_weight()
    print "# Done in %s\n" % elapsed_time(t0)
