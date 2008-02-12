#!/usr/bin/env python

import os
from math import pi, atan2
import pyfits
import numpy
import matplotlib
matplotlib.use("Agg")
from pylab import *
from matplotlib.ticker import FormatStrFormatter
import pipeline.io as cpio
import pipeline.etc
def run():

	conf = cpio.ComponentConfig()
	io = cpio.ComponentIO()

	catName = io.getFileById('ldac_catalog') #Input Catalog.
	imgName = io.getFileById('fits_me_image') #Input Catalog.
#	weightMapName = io.getFileById('test.weight') #Input Catalog.
	mag_lim = conf.getScalarById('mag_lim') #Limit Magnitude for masks.
	cslim = conf.getScalarById('cs_lim') 

	X = pyfits.open(catName)

	xpos = X[pipeline.etc.LDACObjectsTable].data.field('x_image').tolist()
	ypos = X[pipeline.etc.LDACObjectsTable].data.field('y_image').tolist()
	flag = X[pipeline.etc.LDACObjectsTable].data.field('flags').tolist()
	ra_obj = X[pipeline.etc.LDACObjectsTable].data.field('alpha_j2000').tolist()
	dec_obj = X[pipeline.etc.LDACObjectsTable].data.field('delta_j2000').tolist()
	mag_err = X[pipeline.etc.LDACObjectsTable].data.field('magerr_auto').tolist()
	ra_mask = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('mag_auto') <= mag_lim].field('alpha_j2000').tolist()
	dec_mask = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('mag_auto') <= mag_lim].field('delta_j2000').tolist()
	xrms_mask = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('mag_auto') <= mag_lim].field('x2_world').tolist()
	yrms_mask = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('mag_auto') <= mag_lim].field('y2_world').tolist()
	fwhm_mask = X[pipeline.etc.LDACObjectsTable].data[X[pipeline.etc.LDACObjectsTable].data.field('mag_auto') <= mag_lim].field('fwhm_world').tolist()
	cs = X[pipeline.etc.LDACObjectsTable].data.field('class_star').tolist()
	fr = X[pipeline.etc.LDACObjectsTable].data.field('flux_radius').tolist()
	mag = X[pipeline.etc.LDACObjectsTable].data.field('mag_auto').tolist()

	flag_state = def_flag_state(flag)
	flag_trim = def_flag_trim(imgName,xpos,ypos)
	flag_3sigma, flag_5sigma, flag_10sigma = def_flag_sigma(mag_err)
	masks = create_masks(ra_mask,dec_mask,xrms_mask,yrms_mask,fwhm_mask)
	masklist = masks.data.field('masks').tolist()
	flag_mask = def_flag_mask(ra_obj,dec_obj,masklist)
	flag_star = def_flag_star(cs,fr,mag,cslim)

	col_flag_trim = pyfits.Column(name="FLAG_TRIM", format='I', array=flag_trim)
	col_flag_mask = pyfits.Column(name="FLAG_MASK", format='I', array=flag_mask)
	col_flag_star = pyfits.Column(name="FLAG_STAR", format='I', array=flag_star)
	col_flag_state = pyfits.Column(name="FLAG_STATE", format='I', array=flag_state)
	col_flag_3sigma = pyfits.Column(name="FLAG_3SIGMA", format='I', array=flag_3sigma)
	col_flag_5sigma = pyfits.Column(name="FLAG_5SIGMA", format='I', array=flag_5sigma)
	col_flag_10sigma = pyfits.Column(name="FLAG_10SIGMA", format='I', array=flag_10sigma)
	cols = pyfits.ColDefs([col_flag_trim,col_flag_mask,col_flag_state,col_flag_3sigma,col_flag_5sigma,col_flag_10sigma,col_flag_star])
	X[pipeline.etc.LDACObjectsTable] = pyfits.new_table(X[pipeline.etc.LDACObjectsTable].columns + cols)


	X.append(masks)
	X[pipeline.etc.LDACObjectsTable].__setattr__('name','OBJECTS')
	X[pipeline.etc.LDACObjectsTable].__setattr__('name','MASKS')
	X.writeto('qc_'+catName)

	io.addOutput('qc_'+catName,'ldac_catalog','fits_catalog','','','file')

	X.close()

#######################
# Defining FLAG_STATE #
#######################
def def_flag_state(flag):
	"""
	Method to classificate object as good (flag_state = 0), bad (flag_state = 1), saturated (flag_state = 2) or bad and saturated (flag_state = 3).
	"""

	a = []
	sat = []
	for i in range(32):
		a.append(i*8+4)

	for i in a:
		sat.append(i)
		sat.append(i+1)
		sat.append(i+2)
		sat.append(i+3)

        fl3=7
        fl3b=16
        fl3e=23
        fl4=32
        fl6=39
        fl6a=48
        fl8=55

	flagstate = []
	for i in range(len(flag)):
		flsat = 0
		flbad = 0
		if flag[i] in sat:
			flsat = 1
		if (flag[i] > fl3 and flag[i] < fl3b) or (flag[i] > fl3e and flag[i] < fl4) or (flag[i] > fl6 and flag[i] < fl6a ) or (flag[i] > fl8):
			flbad = 1
		if flbad == 0 and flsat == 0:
			flagstate.append(int(0))
		elif flbad == 1 and flsat == 0:
			flagstate.append(int(1))
		elif flbad == 0 and flsat == 1:
			flagstate.append(int(2))
		else:
			flagstate.append(int(3))
	return flagstate


######################
# Defining FLAG_TRIM #
######################
def def_flag_trim(imgFile,x,y):
	"""
	Method to get trim region and say if object is inside it or not.
	"""
	conf = cpio.ComponentConfig()

	#This is a bypass if running on a DES environment
	b_trimmed = conf.getScalarById('trimmed') #Limit Magnitude for masks.

	flagtrim = []

	if b_trimmed:
		for i in range(len(x)):
			flagtrim.append(int(0))

		return flagtrim
	
	#Otherwise, we try to trim the image

	tf_cmd = "trimfits-edited -i "+imgFile+" -w "+imgFile+"\[2\] -t 0.8"
	saida = os.popen(tf_cmd).read().splitlines()
	xmin = float(saida[-4].split()[-1])
	xmax = float(saida[-3].split()[-1])
	ymin = float(saida[-2].split()[-1])
	ymax = float(saida[-1].split()[-1])

	for i in range(len(x)):
		if x[i] > xmin and x[i] < xmax and y[i] > ymin and y[i] < ymax:
			flagtrim.append(int(0))
		else:
			flagtrim.append(int(1))
	return flagtrim


#######################
# Defining FLAG_SIGMA #
#######################
def def_flag_sigma(err):
	"""
	Method to get sigma threshold and say if object is above it or not.
	"""

	flag3sigma = []
	flag5sigma = []
	flag10sigma = []
	for i in range(len(err)):
		if err[i] > 0.3:
			flag3sigma.append(int(1))
		else:
			flag3sigma.append(int(0))
		if err[i] > 0.2:
			flag5sigma.append(int(1))
		else:
			flag5sigma.append(int(0))
		if err[i] > 0.1:
			flag10sigma.append(int(1))
		else:
			flag10sigma.append(int(0))

	return flag3sigma, flag5sigma, flag10sigma

			
##################
# Creating Masks #
##################
def create_masks(ra,dec,xrms,yrms,fwhm):
	"""
	Method to create masks from a list of bright objects (ra, dec, x_rms, y_rms).
	"""

	masklist = []
	np = 8
	for i in range(len(ra)):
		xf1 = float(ra[i]) - (3./4.)*float(fwhm[i])
		xf2 = float(ra[i]) + (3./4.)*float(fwhm[i])
		yf1 = float(dec[i]) - (3./4.)*float(fwhm[i])
		yf2 = float(dec[i]) + (3./4.)*float(fwhm[i])
		x1 = float(ra[i]) - float(xrms[i])**(1./3.5)
		x2 = float(ra[i]) + float(xrms[i])**(1./3.5)
		y1 = float(dec[i]) - float(yrms[i])**(1./3.)
		y2 = float(dec[i]) + float(yrms[i])**(1./3.)
		masklist.append(str(np)+"  "+str(x1)+"  "+str(dec[i])+"  "+str(xf1)+"  "+str(yf2)+"  "+str(ra[i])+"  "+str(y2)+"  "+str(xf2)+"  "+str(yf2)+"  "+str(x2)+"  "+str(dec[i])+"  "+str(xf2)+"  "+str(yf1)+"  "+str(ra[i])+"  "+str(y1)+"  "+str(xf1)+"  "+str(yf1))
	

	mask_col = pyfits.Column(name = "MASKS", format = "400A", array = masklist)
	mask = pyfits.new_table(pyfits.ColDefs([mask_col]))
	
	return mask


######################
# Defining FLAG_MASK #
######################
def def_flag_mask(ra,dec,mask_list):
	"""
	Method to define if an object is inside a mask or not.
	"""

	flagmask = []
	for i in range(len(ra)):
		inzone = 0
		for j in range(len(mask_list)):
			if inzone == 1:
				break
			ramask = []
			decmask = []
			n = int(mask_list[j].split()[0])

			for k in range(1,n+1):
				ramask.append(float(mask_list[j].split()[2*k-1]))
				decmask.append(float(mask_list[j].split()[2*k]))
			ramask.append(float(mask_list[j].split()[1]))
			decmask.append(float(mask_list[j].split()[2]))


			angold = 0.0
			angsum = 0.0
			for k in range(len(ramask)):
				dra = float(ramask[k]) - float(ra[i])
				ddec = float(decmask[k]) - float(dec[i])
				if dra == 0.0 and ddec == 0.0:
					inzone = 1
					break
				ang = atan2(ddec,dra)
				dang = ang - angold
				if dang > pi:
					dang = dang - 2*pi
				if dang < -pi:
					dang = dang + 2*pi
				angold = ang
				
				if k > 0:
					angsum = angsum + dang

			if abs(angsum) > 6.27:
				inzone = 1

		flagmask.append(inzone)
	return flagmask


######################
# Defining FLAG_STAR #
######################
def def_flag_star(cs,fr,mag,cslim):
	"""
	Method to say if an object is a galaxy or a pontual source.
	"""


	frs = []

	for cs_trh in [0.985, 0.965, 0.945, 0.925, 0.9]:
		if len(frs) <= 1:
			frs = []
			for i in range(len(fr)):
				if cs[i] > cs_trh:
					frs.append(fr[i])
		else:
			break

	flagstar = []

	if len(frs) > 1:        #stars found

		frm = median(frs)
		frstd = std(frs)

		for i in range(len(cs)):
			if cs[i] >= cslim and fr[i] > (frm-3.5*frstd) and fr[i] < (frm+3.5*frstd):
				flagstar.append(int(1))
			else:
				flagstar.append(int(0))

	else:
		for i in range(len(cs)):
			flagstar.append(int(0))


	return flagstar

