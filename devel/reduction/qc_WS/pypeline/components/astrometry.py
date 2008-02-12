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
        sys.path.append('/home/rossetto/devel/qctools/qctools/static/cgi-bin/')
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
	outFile = "match.out" #Outputname of match file.
	sig = conf.getScalarById('sigma_cut') #Angular separation in arcsec. Must be given.

	if sig == 3.0:
		s = '3'
	if sig == 5.0:
		s = '5'
	if sig == 10.0:
		s = '10'
	

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

	ramin = min(X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000'))
	ramax = max(X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000'))
	decmin = min(X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000'))
	decmax = max(X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000'))

	a = Y[1].data.field('ra').tolist()
	b = Y[1].data.field('dec').tolist()

	raRef = []
	decRef = []

	for i in range(len(a)):
		if a[i] >= ramin and a[i] <= ramax and b[i] >= decmin and b[i] <= decmax:
			raRef.append(a[i])
			decRef.append(b[i])

	matchFactory1.astroMatch(outFile,raIn,decIn,raRef,decRef,E)

	X.close()
	Y.close()


	X = open(outFile).read().splitlines()

	dra = []
	ddec = []
	dpos = []
	ra = []
	dec = []
	n = []

	for i in range(len(X)-1):
		n.append(int(X[i].split()[4]))
		if int(X[i].split()[4]) == 1 and int(X[i+1].split()[4]) == 1:
			ra.append(float(X[i].split()[0]))
			dec.append(float(X[i].split()[1]))
			dra.append(float(X[i].split()[2]))
			ddec.append(float(X[i].split()[3]))
			dpos.append(sqrt(float(X[i].split()[2])**2 + float(X[i].split()[3])**2)) 

	if X[len(X)-1].split()[4] == 1:
		ra.append(float(X[len(X)-1].split()[0]))
		dec.append(float(X[len(X)-1].split()[1]))
		dra.append(float(X[len(X)-1].split()[2]))
		ddec.append(float(X[len(X)-1].split()[3]))

	plotName = catFile.rstrip(".fits")+'spatial_delta.png'
	plotName1 = catFile.rstrip(".fits")+'multi_delta.png'
	plotName2 = catFile.rstrip(".fits")+'delta.png'
	plotName3 = catFile.rstrip(".fits")+'delta_ra.png'
	plotName4 = catFile.rstrip(".fits")+'delta_dec.png'

	plotFactory1.plotSpatialDelta(plotName,ra,dec,ramin,ramax,decmin,decmax)
	plotFactory1.plotMultDelta(plotName1,n)
	plotFactory1.plotDelta(plotName2,dra,ddec)
	plotFactory1.plotDeltaRA(plotName3,ra,dpos)
	plotFactory1.plotDeltaDEC(plotName4,dec,dpos)

	os.popen('rm -rf '+outFile)

	io.addOutput(plotName,"spatial_distrib_plot","plot","Spatial Distribution","Spatial distribution of matched objects.","file")
	io.addOutput(plotName2,"delta_plot","plot",u'\u0394\u03B1 x \u0394\u03B4',"Angular Separation between matched sources.","file")
	io.addOutput(plotName3,"delta_ra_plot","plot",u'\u0394 x \u03B1',"Angular separation between matched sources versus RA.","file")
	io.addOutput(plotName4,"delta_dec_plot","plot",u'\u0394 x \u03B4',"Angular separation between matched sources versus DEC.","file")

