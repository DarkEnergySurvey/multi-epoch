"""
This is a module to make Star/Galaxy classification plots for a catalog extracted from an image chosen by QC Prototype.
Created by Bruno Rossetto.
"""

###########
# Imports #
###########

try:
        import sys
        sys.path.append('/home/rossetto/devel/qctools/qctools/static/cgi-bin/')
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
        filter = conf.getScalarById('band') #Angular separation in arcsec. Must be given.

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

        star = []
        gal = []
        for i in range(len(bol_flagstar)):
                a =  bol_flagstar[i] and bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i] and bol_flagsigma[i] 
                b = not(bol_flagstar[i]) and bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i] and bol_flagsigma[i]
                gal.append(a)
                star.append(b)

	mag_gal = X[pipeline.etc.LDACObjectsTable].data[array(gal)].field('mag_auto').tolist()
	mag_star = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('mag_auto').tolist()
	fr_gal = X[pipeline.etc.LDACObjectsTable].data[array(gal)].field('flux_radius').tolist()
	fr_star = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('flux_radius').tolist()
	cs_gal = X[pipeline.etc.LDACObjectsTable].data[array(gal)].field('class_star').tolist()
	cs_star = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('class_star').tolist()


#########
# Plots #
#########

	if len(mag_gal+cs_gal+mag_star+cs_star+fr_gal+fr_star) == 0:
		print "Classification plots weren't done. No objects found."
	else:
		plotName = catFile.rstrip('fits')+'stellarity.png'
		plotName1 = catFile.rstrip('fits')+'flux_radius.png'

		P.plotStellarityXMagnitude(plotName,filter,mag_gal,cs_gal,mag_star,cs_star)
		P.plotMagRadius(plotName1,filter,fr_gal,mag_gal,fr_star,mag_star)

		io.addOutput(plotName,'stellarity_plot','plot','Stellarity Index X Magnitude','Stellarity index versus Magnitude. Red points are objects classified as stars.','file')
		io.addOutput(plotName1,'flux_radius_plot','plot','Magnitude X Half-light Radius','Magnitude versus half-light radius. Red points are objects classified as stars.','file')

##############
# Statistics #
##############

	ngal = str(len(mag_gal))
	nstar = str(len(mag_star))



