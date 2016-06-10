#!/usr/bin/env python

"""
Simple call to DESDMcoords class to insert CCD corners into a
database. Update corners for non existing corners in the new DB schema
Large.  Batch Updathe corners in DB for non existing corners, for a
given reqnum and exec_name

F. Menanteau, Apr 2014.

July 2014, this function is not needed anymore as corners are now
written in the the Refact database automatically, using wcsutils

"""

import os,sys
from multiepoch.descoords import DEScoords

try:
    reqnum    = sys.argv[1]
    exec_name = sys.argv[2]
except:
    prog = os.path.basename(sys.argv[0])
    USAGE = "ERROR:\nUSAGE: %s <reqnum> <exec_name>\n       %s 203 immask\n" % (prog,prog)
    sys.exit(USAGE)


descoo = DEScoords(verbose=True)
descoo.updateCCDcorners_reqnum(reqnum,exec_name) # We need to do this only one
