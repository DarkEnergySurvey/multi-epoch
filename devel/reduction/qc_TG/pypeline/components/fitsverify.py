"""
This is a module to check if the input file is a valid FITS file and look for specific keywords on its header.
Created by Bruno Rossetto.
"""

###########
# Imports #
###########

try:
        import sys
        import plotFactory1 as P
        import pyfits
        from numpy import min, max, sqrt, array
        import time
        import pipeline.io as cpio
except ImportError, err:
        print err
        exit()

def run():

        conf = cpio.ComponentConfig()
        io = cpio.ComponentIO()

        fitsFile = io.getFileById('fits_me_image') #Input file. Must be given.

	X = pyfits.open(fitsFile)

	try:
		gain = str(X[0].header['gain'])
	except:
		print "Keyword GAIN not found."
		return False
	
	try:
		zp = str(X[0].header['zp'])
	except:
		print "Keyword ZP not found."
		return False
	
	try:
		ps = str(X[0].header['cd2_2']*3600.0)
	except:
		print "Keyword CD2_2 not found."
		return False

	io.addOutput(gain,'gain','float','','','config')
	io.addOutput(zp,'zero_point','float','','','config')
	io.addOutput(ps,'pixel_scale','float','','','config')

