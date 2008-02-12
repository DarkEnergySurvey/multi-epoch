"""
This is a module to match a catalog extracted from an image chosen by QC Prototype with USNOB1 reference catalog.
Created by Bruno Rossetto.
"""

###########
# Imports #
###########

try:
        import sys
	import os
        import matchFactory1
	import plotFactory1
	import pyfits
	from numpy import min, max, sqrt, array
	import time
	import pipeline.io as cpio
	import pipeline.etc
except ImportError, err:
        print err
        exit()


def run():
	
	conf = cpio.ComponentConfig()
	io = cpio.ComponentIO()

	catFile = io.getFileById('ldac_catalog') #Input catalog. Must be given.
	refFile = io.getFileById("ref_file") #Reference Catalog file.
	E = conf.getScalarById('ang_sep') #Angular separation in arcsec. Must be given.
	filter = conf.getScalarById('band') #Filter band. Must be given.
	outFile = "match.out" #Outputname of match file.
	sig = conf.getScalarById('sigma_cut') #Angular separation in arcsec. Must be given.

	if sig == 3.0:
		s = "3"
	if sig == 5.0:
		s = "5"
	if sig == 10.0:
		s = "10"
	

	if filter == "B":
		band = "B2_mag"
	elif filter == "R":
		band = "R2_mag"
	elif filter == "I":
		band = "I_mag"
	else:
		return False

	X = pyfits.open(catFile)
	Y = pyfits.open(refFile)

	bol_flagtrim = (X[pipeline.etc.LDACObjectsTable].data.field('flag_trim') == 0).tolist()
	bol_flagmask = (X[pipeline.etc.LDACObjectsTable].data.field('flag_mask') == 0).tolist()
	bol_flagstate = (X[pipeline.etc.LDACObjectsTable].data.field('flag_state') == 0).tolist()
	bol_flagsigma = (X[pipeline.etc.LDACObjectsTable].data.field('flag_'+s+'sigma') == 0).tolist()

	util = []
	for i in range(len(bol_flagtrim)):
		a = bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i] and bol_flagsigma[i]
		util.append(a)

	raIn = X[pipeline.etc.LDACObjectsTable].data[array(util)].field('alpha_j2000').tolist()
	decIn = X[pipeline.etc.LDACObjectsTable].data[array(util)].field('delta_j2000').tolist()
	magIn =  X[pipeline.etc.LDACObjectsTable].data[array(util)].field('mag_auto').tolist()
	magerr = X[pipeline.etc.LDACObjectsTable].data[array(util)].field('magerr_auto').tolist()

	ramin = min(X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000'))
	ramax = max(X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000'))
	decmin = min(X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000'))
	decmax = max(X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000'))

	a = Y[1].data[Y[1].data.field(band) > 0.0].field('ra').tolist()
	b = Y[1].data[Y[1].data.field(band) > 0.0].field('dec').tolist()
	c = Y[1].data[Y[1].data.field(band) > 0.0].field(band).tolist()

	raRef = []
	decRef = []
	magRef = []

	for i in range(len(a)):
		if a[i] >= ramin and a[i] <= ramax and b[i] >= decmin and b[i] <= decmax:
			raRef.append(a[i])
			decRef.append(b[i])
			magRef.append(c[i])

	matchFactory1.photoMatch(outFile,raIn,decIn,magIn,raRef,decRef,magRef,E)

	X.close()
	Y.close()


	X = open(outFile).read().splitlines()

	dmag = []
	mag1 = []
	mag2 = []

	for i in range(len(X)-1):
		if int(X[i].split()[3]) == 1 and int(X[i+1].split()[3]) == 1:
			mag1.append(float(X[i].split()[0]))
			mag2.append(float(X[i].split()[1]))
			dmag.append(float(X[i].split()[2]))

	if int(X[len(X)-1].split()[3]) == 1:
		mag1.append(float(X[len(X)-1].split()[0]))
		mag2.append(float(X[len(X)-1].split()[1]))
		dmag.append(float(X[len(X)-1].split()[2]))

	plotName = catFile.rstrip(".fits")+'mag_error.png'
	plotName1 = catFile.rstrip(".fits")+'delta_mag.png'
	plotName2 = catFile.rstrip(".fits")+'delta_mag_mean.png'
	plotName3 = catFile.rstrip(".fits")+'delta_mag_rms.png'

	plotFactory1.plotDeltaMag(plotName1,mag1,dmag)
	plotFactory1.plotMeanDeltaMag(plotName2,mag1,dmag)
	plotFactory1.plotRmsDeltaMag(plotName3,mag1,dmag)
	plotFactory1.plotErrorMag(plotName,magIn,magerr)

	os.popen('rm -rf '+outFile)

	io.addOutput(plotName,"mag_error_plot","plot",'','',"file")
	io.addOutput(plotName1,"delta_mag_plot","plot",'','',"file")
	io.addOutput(plotName2,"delta_mag_mean_plot","plot",'','',"file")
	io.addOutput(plotName3,"delta_mag_rms_plot","plot",'','',"file")

