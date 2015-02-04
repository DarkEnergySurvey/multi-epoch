#!/usr/bin/env python

import os,sys
from multiepoch import DEScoordsOLD


################################################################
# New corners scheme for runs we want
# We only need to do this once for El Gordo and RJX runs
query = """where
          (run='20140421093250_20121207' or run='20140421090850_20121124')
          and IMAGETYPE='red'"""
descoo = DEScoordsOLD(verbose=True)
descoo.insertCCDcorners_query(query,tablename='felipe.imagecorners',clobber=False)
