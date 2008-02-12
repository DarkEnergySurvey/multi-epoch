"""
This is a module to make counts plots for a catalog extracted from an image chosen by QC Prototype.
Created by Bruno Rossetto.
"""

###########
# Imports #
###########

try:
        import sys
        import plotFactory1 as P
        import pyfits
	import numpy
	from numpy import pi, cos, sin, min, max
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

        ramin = X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000').min()
        ramax = X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000').max()
        decmin = X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000').min()
        decmax = X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000').max()

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
	area = area_total - (area_trim) #+ area_mask) 
# O calculo da area util total ainda nao leva em conta a sobreposicao entre trimmed_area e masked_area.
# Essa correcao sera implementada em breve.
			
	star = []
	gal = []
	for i in range(len(bol_flagtrim)):
		a = bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i] and bol_flagsigma[i] and bol_flagstar[i]
		b = bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i] and bol_flagsigma[i] and not(bol_flagstar[i])
		gal.append(a)
		star.append(b)

	mag_gal = X[pipeline.etc.LDACObjectsTable].data[numpy.array(gal)].field('mag_auto').tolist()
	mag_star = X[pipeline.etc.LDACObjectsTable].data[numpy.array(star)].field('mag_auto').tolist()

	if len(mag_gal) > 0:
		plotName = catFile.rstrip("fits")+"mag_gal_hist.png"
		plotName2 = catFile.rstrip("fits")+"mag_gal_count.png"
		P.plotGalCount(plotName,mag_gal)
		P.plotNormGalCount(plotName2,mag_gal,area)
		io.addOutput(plotName,'mag_gal_hist','plot','Galaxy Distribution','Distribution of galaxies per magnitude bin.','file')
		io.addOutput(plotName2,'mag_gal_count','plot','Galaxy Count','Normalized galaxy counts.','file')
	else:
		print "Galaxy counts weren't done. No galaxies found. "

	if len(mag_star) > 0:
		plotName1 = catFile.rstrip("fits")+"mag_star_hist.png"
		plotName3 = catFile.rstrip("fits")+"mag_star_count.png"
		P.plotStarCount(plotName1,mag_star)
		P.plotNormStarCount(plotName3,mag_star,area)
		io.addOutput(plotName1,'mag_star_hist','plot','Star Distribution','Distribution of stars per magnitude bin.','file')
		io.addOutput(plotName3,'mag_star_count','plot','Star Count','Normalized star counts.','file')
	else:
		print "Star counts weren't done. No stars found. "

