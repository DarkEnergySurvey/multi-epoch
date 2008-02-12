"""
This is a module to make psf plots for a catalog extracted from an image chosen by QC Prototype.
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
	import pipeline.etc
except ImportError, err:
        print err
        exit()


def run():

        conf = cpio.ComponentConfig()
        io = cpio.ComponentIO()

        catFile = io.getFileById('ldac_catalog') #Input catalog. Must be given.
	sig = conf.getScalarById('sigma_cut') #Angular separation in arcsec. Must be given.

	if sig == 3.0:
		s = "3"
	if sig == 5.0:
		s = "5"
	if sig == 10.0:
		s = "10"
	

        X = pyfits.open(catFile)

        bol_flagtrim = (X[pipeline.etc.LDACObjectsTable].data.field('flag_trim') == 0).tolist()
        bol_flagmask = (X[pipeline.etc.LDACObjectsTable].data.field('flag_mask') == 0).tolist()
        bol_flagstate = (X[pipeline.etc.LDACObjectsTable].data.field('flag_state') == 0).tolist()
        bol_flagsigma = (X[pipeline.etc.LDACObjectsTable].data.field('flag_'+s+'sigma') == 0).tolist()
        bol_flagstar = (X[pipeline.etc.LDACObjectsTable].data.field('flag_star') == 0).tolist()

        ramin = min(X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000'))
        ramax = max(X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000'))
        decmin = min(X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000'))
        decmax = max(X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000'))

        star = []
        for i in range(len(bol_flagtrim)):
                a = bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i] and bol_flagsigma[i] and not(bol_flagstar[i])
                star.append(a)

	fwhm = (3600.0*X[pipeline.etc.LDACObjectsTable].data[array(star)].field('fwhm_world')).tolist()
	major = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('A_image').tolist()
	minor = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('B_image').tolist()
	pa = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('theta_j2000').tolist()
	ra = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('alpha_j2000').tolist()
	dec = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('delta_j2000').tolist()
	xrms = sqrt(X[pipeline.etc.LDACObjectsTable].data[array(star)].field('x2_image')).tolist()
	yrms = sqrt(X[pipeline.etc.LDACObjectsTable].data[array(star)].field('y2_image')).tolist()


	if len(ra) > 0 and len(dec) > 0 and len(fwhm) > 0:
		plotName = catFile.rstrip("fits")+'scaled_stars.png'
		P.plotScaledStarDistrib(plotName,ra,dec,fwhm,ramin,ramax,decmin,decmax)
		io.addOutput(plotName,'scaled_plot','plot','Scaled Distribution','Spatial distribution of stars scaled by seeing.','file')
	else:
		print "Scaled Distribution plot wasn't done. No stars found."
		
	if len(ra) > 0 and len(dec) > 0 and len(xrms) > 0 and len(yrms) > 0:
		plotName1 = catFile.rstrip("fits")+'psf_distortion.png'
		P.plotPsfDistortion(plotName1,ra,dec,xrms,yrms,ramin,ramax,decmin,decmax)
		io.addOutput(plotName1,'psf_distortion_plot','plot','Distortion Distribution','Distribution of PSF distortion.','file')
	else:
		print "Distortion Distribution plot wasn't done. No stars found."
		
	if len(fwhm) > 0:
		plotName2 = catFile.rstrip("fits")+'psf_histogram.png'
		P.plotPsfHist(plotName2,fwhm)
		io.addOutput(plotName2,'psf_histogram','plot','FWHM Histogram.','FWHM distribution.','file')
	else:
		print "FWHM Histogram wasn't done. No stars found."
		
	if len(major) > 0 and len(minor) and len(pa) > 0:
		plotName3 = catFile.rstrip("fits")+'ellipticity.png'
		P.plotEllipticity(plotName3,major,minor,pa)
		io.addOutput(plotName3,'ellipticity_plot','plot','Ellipticity','Ellipticity of point sources.','file')
	else:
		print "Ellipticity plot wasn't done. No stars found."

