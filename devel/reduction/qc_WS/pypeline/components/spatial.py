"""
This is a module to make spatial distribution plots for a catalog extracted from an image chosen by QC Prototype.
Created by Bruno Rossetto.
"""

###########
# Imports #
###########

try:
        import sys
        import plotFactory1 as P
        import pyfits
        from numpy import min, max, sqrt, array, cos, pi
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

	ra_all_gal = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('flag_star') == 0].field('alpha_j2000').tolist()
	dec_all_gal = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('flag_star') == 0].field('delta_j2000').tolist()

        bol_flagtrim = (X[pipeline.etc.LDACObjectsTable].data.field('flag_trim') == 0).tolist()
        bol_flagmask = (X[pipeline.etc.LDACObjectsTable].data.field('flag_mask') == 0).tolist()
        bol_flagstate = (X[pipeline.etc.LDACObjectsTable].data.field('flag_state') == 0).tolist()
        bol_flagsigma = (X[pipeline.etc.LDACObjectsTable].data.field('flag_'+s+'sigma') == 0).tolist()
        bol_flagstar = (X[pipeline.etc.LDACObjectsTable].data.field('flag_star') == 0).tolist()

	if (X[pipeline.etc.LDACObjectsTable+1]).size() != 0:
		masks = X[pipeline.etc.LDACObjectsTable+1].data.field('masks').tolist()
	else:
		masks = []

        ramin = min(X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000'))
        ramax = max(X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000'))
        decmin = min(X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000'))
        decmax = max(X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000'))

        util = []
	inutil = []
	star = []
	gal = []
        for i in range(len(bol_flagtrim)):
                a = bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i] and bol_flagsigma[i]
                util.append(a)
		inutil.append(not(a))
	for i in range(len(bol_flagstar)):
		a =  bol_flagstar[i] and util[i]
		b = not(bol_flagstar[i]) and util[i]
		gal.append(a)
		star.append(b)

        rain = X[pipeline.etc.LDACObjectsTable].data[array(util)].field('alpha_j2000').tolist()
        decin = X[pipeline.etc.LDACObjectsTable].data[array(util)].field('delta_j2000').tolist()

        raout = X[pipeline.etc.LDACObjectsTable].data[array(inutil)].field('alpha_j2000').tolist()
        decout = X[pipeline.etc.LDACObjectsTable].data[array(inutil)].field('delta_j2000').tolist()

        ragal = X[pipeline.etc.LDACObjectsTable].data[array(gal)].field('alpha_j2000').tolist()
        decgal = X[pipeline.etc.LDACObjectsTable].data[array(gal)].field('delta_j2000').tolist()

        rastar = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('alpha_j2000').tolist()
        decstar = X[pipeline.etc.LDACObjectsTable].data[array(star)].field('delta_j2000').tolist()


#########
# Plots #
#########
	if len(rain) > 0 and len(raout) > 0 and len(decin) > 0 and len(decout) > 0:
		plotName = catFile.rstrip(".fits")+"spatial.png"
		P.plotSpatialDistrib(plotName,masks,rain,decin,raout,decout,ramax,ramin,decmin,decmax)
		io.addOutput(plotName,"spatial_distrib_plot","plot",'Spatial Distribution','Spatial distribution of all objects and masks.',"file")
	else:
		print "Spatial Distribution plot wasn't done. No objects found."

	if len(ra_all_gal) > 0 and len(dec_all_gal) > 0:
		plotName1 = catFile.rstrip(".fits")+"smooth.png"
		P.plotSmooth(plotName1,ra_all_gal,dec_all_gal)
		io.addOutput(plotName1,"smooth_density_plot","plot",'Smooth Density Field','Smooth density field.',"file")
	else:
		print "Smooth Density Field wasn't done. No objects found."

	if len(ragal) > 0 and len(decgal) > 0:
		plotName2 = catFile.rstrip(".fits")+"spatial_gal.png"
		P.plotSpatialDistribGal(plotName2,ragal,decgal,ramax,ramin,decmin,decmax)
		io.addOutput(plotName2,"spatial_distrib_gal_plot","plot",'Spatial Distribution (Galaxies)','Spatial distribution of galaxies.',"file")
	else:
		print "Spatial Distribution plot for galaxies wasn't done. No galaxies found."

	if len(rastar) > 0 and len(decstar) > 0:
		plotName3 = catFile.rstrip(".fits")+"spatial_star.png" 
		P.plotSpatialDistribStar(plotName3,rastar,decstar,ramax,ramin,decmin,decmax)
		io.addOutput(plotName3,"spatial_distrib_star_plot","plot",'Spatial Distribution (Point Sources)','Spatial distribution of point sources.',"file")
	else:
		print "Spatial Distribution plot for stars wasn't done. No stars found."



##############
# Statistics #
##############

        ramin_util = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('flag_trim') == 0].field('alpha_j2000').min()
        ramax_util = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('flag_trim') == 0].field('alpha_j2000').max()
        decmin_util = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('flag_trim') == 0].field('delta_j2000').min()
        decmax_util = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('flag_trim') == 0].field('delta_j2000').max()

        area_total = abs((cos(decmin*pi/180.) - cos(decmax*pi/180.))*(180./pi)*(ramax - ramin))
        area_util = abs((cos(decmin_util*pi/180.) - cos(decmax_util*pi/180.))*(180./pi)*(ramax_util - ramin_util))
        area_trim = area_total - area_util

        area_mask = 0.0
	if (X[pipeline.etc.LDACObjectsTable+1]).size() != 0:
		masks = X[pipeline.etc.LDACObjectsTable+1].data.field('masks').tolist()

		for i in range(len(masks)):
			x = []
			y = []
			n = int(masks[i].split()[0])
			for j in range(1,n+1):
				x.append(float(masks[i].split()[2*j - 1]))
				y.append(float(masks[i].split()[2*j]))
			area_mask = area_mask + ((max(x)-min(x))*(max(y)-min(y)))/2.
        area = area_total - (area_trim + area_mask)
# O calculo da area util total ainda nao leva em conta a sobreposicao entre trimmed_area e masked_area.
# Essa correcao sera implementada em breve.

	#Number of objects
	ntot = str(len(rain) + len(raout))
	neff = str(len(rain))
	
	#Areas
	total_area = str(area_total)
	effective_area = str(area)

