#!/usr/bin/env python
"""
This is a library to make plots for a catalog extracted from an image chosen by QC Prototype.
Created by Bruno Rossetto. 
"""
###########
# Imports #
###########

try:
	import sys
	import matplotlib
	import math
	matplotlib.use("Agg")
	from pylab import *
	from matplotlib.ticker import MultipleLocator, FormatStrFormatter
	from os import popen, stat
	from Ft.Xml import MarkupWriter
except ImportError, err:
        print err
        exit()



###########################################################
# Method to plot Spatial Distribution of detected objects #
###########################################################

def plotSpatialDistrib(plotName,masks,ra,dec,r,d,ramin,ramax,decmin,decmax):
        """
        Method to plot Spatial Distribution of detected objects
        """

        figure(1, figsize=(6.4, 4.8))
        cla()
        ax = subplot(111)
        plot(ra,dec, markersize=2, marker='h', color='k', linewidth=0)
        plot(r,d, markersize=2, marker='h', color='r', mec='r', linewidth=0)
	if len(masks) != 0:
		for i in range(len(masks)):
			x = []
			y = []
			n = int(masks[i].split()[0])
			for j in range(1,n+1):
				x.append(float(masks[i].split()[2*j - 1]))
				y.append(float(masks[i].split()[2*j]))
			x.append(float(masks[i].split()[1]))
			y.append(float(masks[i].split()[2]))
			plot(x,y,markersize=0,c='b',lw=2)
        majorFormatter = FormatStrFormatter('%3.2f')
        ax.xaxis.set_major_formatter(majorFormatter)
        ax.yaxis.set_major_formatter(majorFormatter)
        axis('image')
        xlim(ramin,ramax)
        ylim(decmin,decmax)
        grid(True)
        setp(ax.get_yticklabels(), fontsize=9)
        setp(ax.get_xticklabels(), fontsize=9)
        xlabel(r'$\alpha\ \rm{(degree)}$')
        ylabel(r'$\delta\ \rm{(degree)}$')
        savefig("./"+plotName)
        close(1)
        return True



###########################################################
# Method to plot Spatial Distribution of detected objects #
###########################################################

def plotSpatialDelta(plotName,ra,dec,ramin,ramax,decmin,decmax):
	"""
	Method to plot Spatial Distribution of detected objects
	"""

	figure(1, figsize=(6.4, 4.8))
	cla()
	ax = subplot(111)
	plot(ra,dec, markersize=2, marker='h', color='k', linewidth=0)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	grid(True)
	setp(ax.get_yticklabels(), fontsize=9)
	setp(ax.get_xticklabels(), fontsize=9)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	savefig("./"+plotName)
	close(1)
	return True



############################################################
# Method to plot Spatial Distribution of saturated objects #
############################################################

def plotSatDistrib(plotName):
	"""
	Method to plot Spatial Distribution of saturated objects
	"""

	figure(18, figsize=(6.4, 4.8))
	cla()
	ax = subplot(111)
	for i in range(len(rasat)):
		a = array([rasat[i]])
		b = array([decsat[i]])
		plot(a,b, markersize=3*fwhmsat[i]/4, marker='h', color='k', linewidth=0)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	grid(True)
	setp(ax.get_yticklabels(), fontsize=9)
	setp(ax.get_xticklabels(), fontsize=9)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	savefig("./"+plotName)
	close(18)
	return True


###################################################
# Method to plot Spatial Distribution of Galaxies #
###################################################

def plotSpatialDistribGal(plotName,rg,dg,ramin,ramax,decmin,decmax):
	"""
	Method to plot Spatial Distribution of Galaxies
	"""

	figure(2, figsize=(6.4, 4.8))
	cla()
	ax = subplot(111)
	plot(rg,dg, markersize=2, marker='h', color='k', linewidth=0)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	grid(True)
	setp(ax.get_yticklabels(), fontsize=9)
	setp(ax.get_xticklabels(), fontsize=9)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	savefig("./"+plotName)
	close(2)
	return True


################################################
# Method to plot Spatial Distribution of Stars #
################################################

def plotSpatialDistribStar(plotName,rs,ds,ramin,ramax,decmin,decmax):
	"""
	Method to plot Spatial Distribution of Stars
	"""

	figure(3, figsize=(6.4, 4.8))
	cla()
	ax = subplot(111)
	plot(rs,ds,markersize=2,marker='h',color='k',linewidth=0)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	grid(True)
	setp(ax.get_yticklabels(), fontsize=9)
	setp(ax.get_xticklabels(), fontsize=9)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')	
	savefig("./"+plotName)
	close(3)
	return True


#######################################
# Method to plot smooth density field #
#######################################

def plotSmooth(plotName,r,d,ramin, ramax, decmin, decmax):
	"""
	Method to plot Smooth Density Field.
	"""

	x = []
	y = []

	filein = open("radec.txt","w")

	for i in range(len(r)):
		x.append(r[i])
		y.append(d[i])
		filein.write(str(r[i])+"\t"+str(d[i])+"\n")

	filein.close()

	xmin = min(x)
	xmax = max(x)
	ymin = min(y)
	ymax = max(y)

	C = popen("matrix < radec.txt").read().splitlines()

	Cs = zeros([len(C),len(C[0].split())],Float)

	for i in range(len(C)):
		for j in range(len(C[i].split())):
			Cs[i][j] = float(C[i].split()[j])


	xbin = (xmax - xmin)/len(C)
	ybin = (ymax - ymin)/len(C[0].split())

	xs = zeros(len(C)+1,Float)
	ys = zeros(len(C[0].split())+1,Float)
	for i in range(len(C)+1):
		xs[i] = xmin + xbin*i

	for i in range(len(C[0].split())+1):
		ys[i] = ymin + ybin*i

	Xs,Ys = meshgrid(xs,ys)

	figure(1, figsize=(6.4,4.8))
	ax = subplot(111)
	pcolor(Xs,Ys,transpose(Cs),shading='flat',cmap=cm.jet)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xmin,xmax = xlim()
	xlim(ramin,ramax)
	xlim(decmin,decmax)
        setp(ax.get_yticklabels(), fontsize=9)
        setp(ax.get_xticklabels(), fontsize=9)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	savefig("./"+plotName)
	close(1)

	popen("rm -rf radec.txt")
	return True



###############################################
# Method to plot Stellarity Index X Magnitude #
###############################################

def plotStellarityXMagnitude(plotName,filter,magg,csg,mags,css):
	"""
	Method to plot Stellarity Index X Magnitude
	"""

	figure(4, figsize=(6.4, 4.8))
	cla()
	plot(magg,csg, markersize=2, marker='h', color='k', linewidth=0)
	plot(mags,css, markersize=2, marker='h', color='r', mec='r', linewidth=0)
	ylim(-0.05,1.05)
	grid(True)
	xlabel(filter+' (MAG_AUTO)')
	ylabel('CLASS_STAR')
	savefig("./"+plotName)
	close(4)
	return True


###################################################################
# Method to plot separation between sources from Match against RA #
###################################################################

def plotDeltaRA(plotName,ra,epos):
        """
        Method to plot separation between sources from Match against RA.
        """

        figure(45, figsize=(6.40,4.80))
        ax = subplot(111)
        plot(ra,epos,markersize=2,marker='h',color='k',linewidth=0)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	setp(ax.get_yticklabels(), fontsize=9)
	setp(ax.get_xticklabels(), fontsize=9)
	ylabel(r'$\Delta\ \rm{position (arcsec)}$')
        xlabel(r'$\alpha\ \rm{(arcsec)}$')
        savefig("./"+plotName)
        close(45)
        return True



####################################################################
# Method to plot separation between sources from Match against DEC #
####################################################################

def plotDeltaDEC(plotName,dec,epos):
        """
        Method to plot separation between sources from Match against DEC.
        """

        figure(46, figsize=(6.40,4.80))
        ax = subplot(111)
        plot(dec,epos,markersize=2,marker='h',color='k',linewidth=0)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	setp(ax.get_yticklabels(), fontsize=9)
	setp(ax.get_xticklabels(), fontsize=9)
	ylabel(r'$\Delta\ \rm{position (arcsec)}$')
        xlabel(r'$\delta\ \rm{(arcsec)}$')
        savefig("./"+plotName)
        close(46)
        return True



########################################################
# Method to plot separation between sources from Match #
########################################################

def plotDelta(plotName,er,ed):
	"""
	Method to plot separation between sources from Match
	"""

	figure(5, figsize=(6.40,6.40))
	h = []
	for i in range(20):
		h.append(-1 + i*0.1)
	ax1 = axes([0.10,0.10,0.6,0.2])
	majorLocator = MultipleLocator(200)
	minorLocator = MultipleLocator(20)
	hist(er,bins=h,width=0.1,facecolor=None)
	ax1.yaxis.set_major_locator(majorLocator)
	ax1.yaxis.set_minor_locator(minorLocator)
	xlabel(r'$\Delta \alpha\ \rm{(arcsec)}$')
	ylabel('N')
	ax2 = axes([0.70,0.30,0.2,0.6])
	hist(ed,bins=h,width=0.1,facecolor=None,orientation='horizontal')
	ax2.xaxis.set_major_locator(majorLocator)
	ax2.xaxis.set_minor_locator(minorLocator)
	xmin, xmax = xlim()
	xlim(xmax,xmin)
	setp(ax2.get_yticklabels(), visible=False)
	xlabel('N')
	ax3 = axes([0.10,0.30,0.6,0.6],sharex=ax1,sharey=ax2)
	xlim(-0.9999,0.9999)
	ylim(-0.9999,0.9999)
	setp(ax3.get_xticklabels(), visible=False)
	plot(er,ed, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=0,c='k',lw=0.5,ls='--')
	axvline(x=0,c='k',lw=0.5,ls='--')
	ylabel(r'$\Delta \delta\ \rm{(arcsec)}$')
	savefig("./"+plotName)
	close(5)
	return True



#####################################
# Method to plot Match Multiplicity #
#####################################

def plotMultDelta(plotName,n):
	"""
	Method to plot Match Multiplicity.
	"""

	h = []
	for i in range(10):
		h.append(i)

	figure(6, figsize=(6.4, 4.8))
	cla()
	ax = subplot(111)
	majorLocator = MultipleLocator(1)
	hist(n,bins=h,width=1,fill=False,align='center')
	ymin, ymax = ylim()
	ylim(ymin,ymax+ymax/10)
	xlim(0,10)
	ax.xaxis.set_major_locator(majorLocator)
	xlabel('Number of matches for each object')
	ylabel('N')
	savefig("./"+plotName)
	close(6)
	return True


###################################################################################
# Method to plot magnitude differences between matched sources from Match Catalog #
###################################################################################

def plotDeltaMag(plotName,maIN,dma):
	""" 
	Method to plot magnitude differences between matched sources from Match Catalog
	"""

	figure(7, figsize=(6.4, 4.8))
	cla()
	magval = []
	for i in range (len(dma)):
		if maIN[i] <= 16.5:
			magval.append(float(dma[i]))
	m = mean(magval)
	med = median(magval)
	plot(maIN,dma, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=m,ls='--',lw=0.5,c='r')
	axhline(y=med,ls=':',lw=0.5,c='b')
	xlabel('m(MAG_AUTO)')
	ylabel('m - m(MAG_AUTO)')
	savefig("./"+plotName)
	close(7)
	return True


#####################################################################################################
# Method to plot magnitude differences between matched sources from Match Catalog corrected by mean #
#####################################################################################################

def plotMeanDeltaMag(plotName,maIN,dma):
	"""
	Method to plot magnitude differences between matched sources from Match Catalog corrected by mean
	"""

	figure(19, figsize=(6.4, 4.8))
	totval = []
	matotval = []
	for i in range (len(dma)):
		if maIN[i] <= 16.5:
			totval.append(float(dma[i]))
			matotval.append(float(maIN[i]))
	dmacor = []
	m = mean(totval)
	medi = median(totval)
	for i in range(len(dma)):
		dmacor.append(float(dma[i]-m))
	med = []
	x = []
	for i in range(30):
		sum = 0
		npts = 0
		x1 = i*0.5 + floor(min(maIN)) - 0.25
		x2 = i*0.5 + floor(min(maIN)) + 0.25
		for j in range(len(maIN)):
			if maIN[j] > x1 and maIN[j] < x2 and maIN[j] < 19.0:
				sum = sum + dmacor[j]
				npts = npts + 1
		if npts > 0:
			med.append(float(sum/npts))
			x.append(float(x2-0.25))
	plot(maIN,dmacor, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=0,ls='--',lw=0.5,c='k')
	plot(x,med,marker='h',ms=2,c='r',lw=1.5,markeredgecolor='r')
	xlabel('m(MAG_AUTO)')
	ylabel('m - m(MAG_AUTO)')
	savefig("./"+plotName)
	close(19)
	return True


############################################################################################################
# Method to plot binned magnitude differences between matched sources from Match Catalog corrected by mean #
############################################################################################################

def plotRmsDeltaMag(plotName,maIN,dma):
	"""
	Method to plot binned magnitude differences between matched sources from Match Catalog corrected by mean
	"""

	figure(20, figsize=(6.4, 4.8))
	totval = []
	matotval = []
	for i in range (len(dma)):
		if maIN[i] <= 16.5:
			totval.append(float(dma[i]))
			matotval.append(float(maIN[i]))
	dmacor = []
	m = mean(totval)
	medi = median(totval)
	for i in range(len(dma)):
		dmacor.append(float(dma[i]-m))
	med = []
	x = []
	err = []
	for i in range(30):
		sum = 0
		npts = 0
		sig = []
		x1 = i*0.5 + floor(min(maIN)) - 0.25
		x2 = i*0.5 + floor(min(maIN)) + 0.25
		for j in range(len(maIN)):
			if maIN[j] > x1 and maIN[j] < x2 and maIN[j] < 19.0:
				sum = sum + dmacor[j]
				npts = npts + 1
				sig.append(float(dmacor[j]))
		if npts > 0:
			med.append(float(sum/npts))
			x.append(float(x2-0.25))
		if npts > 1:
			err.append(float(std(sig)))
		elif npts == 1:
			err.append(float(0.0))
	axhline(y=0,ls='--',lw=0.5,c='k')
	errorbar(x,med,err,marker='h',ms=3,c='r',lw=0,markeredgecolor='r')
	xlim(floor(min(maIN)),ceil(max(maIN)))
	ylim(floor(min(dmacor)),ceil(max(dmacor)))
	xlabel('m(MAG_AUTO)')
	ylabel('m - m(MAG_AUTO)')
	savefig("./"+plotName)
	close(20)
	return True


##############################################
# Method to plot Magnitude X Magnitude Error #
##############################################

def plotErrorMag(plotName,ma,erma):
	"""
	Method to plot an Magnitude X Magnitude Error
	"""

	figure(8, figsize=(6.4, 4.8))
	cla()
	plot(ma,erma, markersize=2, marker='h', color='k', linewidth=0)
	grid(True)
	xlabel('m(MAG_AUTO)')
	ylabel('error m(MAG_AUTO)')
	savefig("./"+plotName)
	close(8)
	return True


##########################################
# Method to plot Magnitude X FluxRadius #
##########################################

def plotMagRadius(plotName,filter,fr,mag,frstar,magstar):
	"""
	Method to plot Magnitude X FluxRadius.
	"""

	figure(9, figsize=(6.4, 4.8))
	plot(fr,mag, markersize=2, marker='h', color='k', linewidth=0)
	plot(frstar,magstar, markersize=2, marker='h', color='r',mec='r', linewidth=0)
	ymin,ymax = ylim()
	ylim(ymax,ymin)
	xlim(0,10)
	grid(True)
	xlabel('FLUX_RADIUS (pixels)')
	ylabel(filter+' (MAG_AUTO)')
	savefig("./"+plotName)
	close(9)
	return True


###########################################
# Method to plot Ellipticity Distribution #
###########################################

def plotEllipticity(plotName,major,minor,pa):
	"""
	Method to plot Ellipticity Distribution.
	"""	

	figure(10, figsize=(6.4, 4.8))
	for i in range(len(major)):
		a = array([((major[i]-minor[i])/(major[i]+minor[i]))*cos(2*pa[i]*math.pi/180.0)])
		b = array([((major[i]-minor[i])/(major[i]+minor[i]))*sin(2*pa[i]*math.pi/180.0)])
		plot(a,b, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=0,c='k',lw=0.5,ls='--')
	axvline(x=0,c='k',lw=0.5,ls='--')
	xlim(-0.5,0.5)
	ylim(-0.5,0.5)
	xlabel('e1')
	ylabel('e2')
	savefig("./"+plotName)
	close(10)
	return True


#################################
# Method to plot PSF Distortion #
#################################

def plotPsfDistortion(plotName,rs,ds,xrms,yrms,ramin,ramax,decmin,decmax):
	"""
	Method to plot PSF Distortion.
	"""
	
	figure(11, figsize=(6.4, 4.8))
	cla()
	ax = subplot(111)
	s = 500.0*(ramax-ramin)
	quiver(rs,ds,xrms,yrms,pivot='middle',units='width',headwidth=1,headlength=1,width=0.0025,scale=s)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
        setp(ax.get_yticklabels(), fontsize=9)
        setp(ax.get_xticklabels(), fontsize=9)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	savefig("./"+plotName)
	close(11)
	return True


##################################
# Method to plot a PSF Histogram #
##################################

def plotPsfHist(plotName,fwhm):
	"""
	Method to plot a PSF Histogram.
	"""

	h = []
	for i in range(100):
		h.append(i*0.1)

	figure(12, figsize=(6.4, 4.8))
	ax = subplot(111)
	minorLocator = MultipleLocator(0.1)
	majorLocator = MultipleLocator(0.5)
	hist(fwhm,bins=h,width=0.1,fill=False)
	ymin, ymax = ylim()
	ylim(ymin,ymax+ymax/10)
	xlim(0,3.49)
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlabel('FWHM (arcsec)')
	ylabel('N')
	savefig("./"+plotName)
	close(12)
	return True


#########################################
# Method to plot Galaxy Count Histogram #
#########################################

def plotGalCount(plotName,maggal):
	"""
	Method to plot Galaxy Count Histogram.
	"""

	g = []
	for i in range(100):
		g.append(i*0.5 + floor(min(maggal)))

	figure(13, figsize=(6.4, 4.8))
	ax = subplot(111)
	minorLocator = MultipleLocator(0.5)
	majorLocator = MultipleLocator(1)
	hist(maggal,bins=g,width=0.5,fill=False)
	ymin, ymax = ylim()
	ylim(ymin,ymax+ymax/10)
	xlim(floor(min(maggal))-1,ceil(max(maggal))+1)
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlabel('m(MAG_AUTO)')
	ylabel('N')
	savefig("./"+plotName)
	close(13)
	return True


#######################################
# Method to plot Star Count Histogram #
#######################################

def plotStarCount(plotName,magstar):
	"""
	Method to plot Star Count Histogram.
	"""

	if len(magstar) == 0:
		return False
	k = []
	for i in range(100):
		k.append(i*0.5 + floor(min(magstar)))

	figure(14, figsize=(6.4, 4.8))
	ax = subplot(111)
	minorLocator = MultipleLocator(0.5)
	majorLocator = MultipleLocator(1)
	hist(magstar,bins=k,width=0.5,fill=False)
	ymin, ymax = ylim()
	ylim(ymin,ymax+ymax/10)
	xlim(floor(min(magstar))-1,ceil(max(magstar))+1)
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlabel('m(MAG_AUTO)')
	ylabel('N')
	savefig("./"+plotName)
	close(14)
	return True


##################################################
# Method to plot Star Counts Normalized per area #
##################################################

def plotNormStarCount(plotName,magstar,area):
	"""
	Method to plot Star Counts Normalized per area.
	"""
	
	if len(magstar) == 0:
		return False
	
	x = []
	me = []
	ME = []
	PE = []
	pe = []
	for i in range(50):
		k1 = float(i*0.5 + floor(min(magstar)))
		k2 = float(k1 + 0.5)
		su = 0
		SU = 0
		for j in range(len(magstar)):
			if magstar[j] > k1 and magstar[j] < k2:
				su = su + 1
#		if plotName.split("/")[-2] == "EIS.2005-06-07T20:32:52.697" or plotName.split("/")[-2] == "EIS.2005-12-09T18:57:47.798" or plotName.split("/")[-2] == "EIS.2005-12-09T22:58:22.340" or plotName.split("/")[-2] == "trim":
#			for j in range(len(M)):
#				if len(M) != 0.0:
#					if M[j] > k1 and M[j] < k2:
#						SU = SU + 1
		if su != 0:
			me.append(su/area)
		else:
			me.append(0)
		pe.append(sqrt(su)/area)
#		if plotName.split("/")[-2] == "EIS.2005-06-07T20:32:52.697" or plotName.split("/")[-2] == "EIS.2005-12-09T18:57:47.798" or plotName.split("/")[-2] == "EIS.2005-12-09T22:58:22.340" or plotName.split("/")[-2] == "trim":
#			if SU != 0:
#				ME.append(float(SU/area))
#			else:
#				ME.append(0)
#			PE.append(sqrt(SU/area))	

	for i in range(50):
		x.append(i*0.5 + 0.25  + floor(min(magstar)))

	a = []
	b = []
	c = []
	A = []
	B = []
	C = []
	for i in range(len(x)):
		if me[i] != 0. :
			a.append(x[i])
			b.append(me[i]) 
			c.append(pe[i])
#		if plotName.split("/")[-2] == "EIS.2005-06-07T20:32:52.697" or plotName.split("/")[-2] == "EIS.2005-12-09T18:57:47.798" or plotName.split("/")[-2] == "EIS.2005-12-09T22:58:22.340" or plotName.split("/")[-2] == "trim":
#			if ME[i] != 0. :
#				A.append(x[i])
#				B.append(ME[i])
#				C.append(PE[i])
	figure(16, figsize=(6.4, 4.8))
	ax = subplot(111)
	minorLocator = MultipleLocator(0.5)
	majorLocator = MultipleLocator(1)
	errorbar(a,b,c,marker='h',markersize=3,c='b',markeredgecolor='b',lw=0)
#	if plotName.split("/")[-2] == "EIS.2005-06-07T20:32:52.697" or plotName.split("/")[-2] == "EIS.2005-12-09T18:57:47.798" or plotName.split("/")[-2] == "EIS.2005-12-09T22:58:22.340" or plotName.split("/")[-2] == "trim":
#		errorbar(A,B,C,marker='s',markersize=3,c='r',markeredgecolor='r',lw=0)
	semilogy()
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlim(floor(min(magstar))-1,ceil(max(magstar))+1)
	xlabel('m(MAG_AUTO)')
	ylabel('N/(sq. deg.)')
	savefig("./"+plotName)
	close(16)
	return True


####################################################
# Method to plot Galaxy Counts Normalized per area #
####################################################

def plotNormGalCount(plotName,maggal,area):
	"""
	Method to plot Galaxy Counts Normalized per area.
	"""

	if len(maggal) == 0:
		return False
	x = []
	me = []
	pe = []
	for i in range(60):
		k1 = (i*0.5 + floor(min(maggal)))
		k2 = k1 + 0.5
		su = 0
		for j in range(len(maggal)):
			if maggal[j] > k1 and maggal[j] < k2:
				su = su + 1
		if su != 0:
			me.append(su/area)
		else:
			me.append(0)
		pe.append(sqrt(su)/area)

	for i in range(50):
		x.append(i*0.5 + 0.25  + floor(min(maggal)))

	a = []
	b = []
	c = []
	for i in range(len(x)):
		if me[i] != 0. :
			a.append(x[i])
			b.append(me[i])
			c.append(pe[i])
	figure(17, figsize=(6.4, 4.8))
	ax = subplot(111)
	minorLocator = MultipleLocator(0.5)
	majorLocator = MultipleLocator(1)
	errorbar(a,b,c,marker='h',markersize=3,c='k',lw=0)
#	errorbar(met,cmet,emet,marker='s',markersize=3,c='r',mec='r',lw=0)
#	errorbar(arn,carn,earn,marker='^',markersize=3,c='b',mec='b',lw=0)
#	errorbar(ber,cber,eber,marker='d',markersize=3,c='g',mec='g',lw=0)
	semilogy()
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlim(floor(min(maggal))-1,ceil(max(maggal))+1)
	xlabel('m(MAG_AUTO)')
	ylabel('N/(sq. deg.)')
	savefig(plotName)
	close(17)
	return True


#######################################################
# Method to plot Scaled Spatial Distribution of Stars #
#######################################################

def plotScaledStarDistrib(plotName,rs,ds,fwhm,ramin,ramax,decmin,decmax):
	"""
	Method to plot Scaled Spatial Distribution of Stars
	"""

	figure(15, figsize=(6.4, 4.8))
	ax = subplot(111)
	for i in range(len(fwhm)):
		a = array([rs[i]])
		b = array([ds[i]])
		plot(a,b,markersize=2.5*fwhm[i],marker='h',color='k',linewidth=0)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	grid(True)
        setp(ax.get_yticklabels(), fontsize=9)
        setp(ax.get_xticklabels(), fontsize=9)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	savefig("./"+plotName)
	close(15)
	return True


################################################
# Method to plot spatial distribution of masks #
################################################

def plotMasks(plotName,masks,ramin,ramax,decmin,decmax):
        """
        Method to plot Spatial Distribution of masks.
        """

        figure(100, figsize=(6.4, 4.8))
        ax = subplot(111)
        for i in range(len(masks)):
                x = []
                y = []
                n = int(masks[i].split()[0])
                for j in range(1,n+1):
                        x.append(float(masks[i].split()[2*j - 1]))
                        y.append(float(masks[i].split()[2*j]))
                x.append(float(masks[i].split()[1]))
                y.append(float(masks[i].split()[2]))
                plot(x,y,markersize=0,c='b',lw=2)
        majorFormatter = FormatStrFormatter('%3.2f')
        ax.xaxis.set_major_formatter(majorFormatter)
        ax.yaxis.set_major_formatter(majorFormatter)
        axis('image')
        xlim(ramax,ramin)
        ylim(decmin,decmax)
        grid(True)
        xlabel(r'$\alpha\ \rm{(degree)}$')
        ylabel(r'$\delta\ \rm{(degree)}$')
        savefig("./"+plotName+'masks.png')
        close(100)
        return True


#########################################
# Method to make o Color-Magnitude Plot #
#########################################

def plotColorMag(plotName,number,U,B,V,R,I,Z):
        """
        Method to make a Color-Magnitude plot.
        """

        Cor = []
        Mag = []
        Corm = []
        Magm = []
        Corm1 = []
        Magm1 = []
        Corm2 = []
        Magm2 = []
        Corm3 = []
        Magm3 = []
        if number == 0:
                col = 'b'
                for i in range(len(U)):
                        if U[i] > 2.0 and U[i] < 80.0 and B[i] > 2.0 and B[i] < 80.0:
                                Cor.append(U[i] - B[i])
                                Mag.append(B[i])
                for i in range(len(Z)):
                        if Z[i][0] != "#":
                                Corm.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
                                Magm.append(float(Z[i].split()[12]))
                                if Z[i].split()[0] == "1":
                                        Corm1.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
                                        Magm1.append(float(Z[i].split()[12]))
                                if Z[i].split()[0] == "2":
                                        Corm2.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
                                        Magm2.append(float(Z[i].split()[12]))
                                if Z[i].split()[0] == "3":
                                        Corm3.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
                                        Magm3.append(float(Z[i].split()[12]))
                mag = "B"
                cor = "U - B"
        if number == 1:
                col = 'g'
                for i in range(len(B)):
                        if B[i] > 2.0 and B[i] < 80.0 and V[i] > 2.0 and V[i] < 80.0:
                                Cor.append(B[i] - V[i])
                                Mag.append(V[i])
                for i in range(len(Z)):
                        if Z[i][0] != "#":
                                Corm.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                Magm.append(float(Z[i].split()[13]))
                                if Z[i].split()[0] == "1":
                                        Corm1.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                        Magm1.append(float(Z[i].split()[13]))
                                if Z[i].split()[0] == "2":
                                        Corm2.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                        Magm2.append(float(Z[i].split()[13]))
                                if Z[i].split()[0] == "3":
                                        Corm3.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                        Magm3.append(float(Z[i].split()[13]))
                mag = "V"
                cor = "B - V"
        if number == 2:
                col = 'y'
                for i in range(len(V)):
                        if V[i] > 2.0 and V[i] < 80.0 and R[i] > 2.0 and R[i] < 80.0:
                                Cor.append(V[i] - R[i])
                                Mag.append(R[i])
                for i in range(len(Z)):
                        if Z[i][0] != "#":
                                Corm.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                Magm.append(float(Z[i].split()[14]))
                                if Z[i].split()[0] == "1":
                                        Corm1.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                        Magm1.append(float(Z[i].split()[14]))
                                if Z[i].split()[0] == "2":
                                        Corm2.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                        Magm2.append(float(Z[i].split()[14]))
                                if Z[i].split()[0] == "3":
                                        Corm3.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                        Magm3.append(float(Z[i].split()[14]))
                mag = "R"
                cor = "V - R"
        if number == 3:
                col = 'r'
                for i in range(len(R)):
                        if R[i] > 2.0 and R[i] < 80.0 and I[i] > 2.0 and I[i] < 80.0:
                                Cor.append(R[i] - I[i])
                                Mag.append(I[i])
                for i in range(len(Z)):
                        if Z[i][0] != "#":
                                Corm.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
                                Magm.append(float(Z[i].split()[15]))
                                if Z[i].split()[0] == "1":
                                        Corm1.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
                                        Magm1.append(float(Z[i].split()[15]))
                                if Z[i].split()[0] == "2":
                                        Corm2.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
                                        Magm2.append(float(Z[i].split()[15]))
                                if Z[i].split()[0] == "3":
                                        Corm3.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
                                        Magm3.append(float(Z[i].split()[15]))
                mag = "I"
                cor = "R - I"
        count = 50 + int(100*rand())
        figure(count, figsize=(6.4, 4.8))
        plot(Cor,Mag,markersize=2,marker='h',color=col,mec=col,linewidth=0)
        xlabel(cor)
        ylabel(mag)
        ymin1, ymax1 = ylim()
        ylim(ymax1,ymin1)
        savefig(plotName+str(number)+'.png')
        close(count)

        figure(count+1, figsize=(6.4, 4.8))
        plot(Corm1,Magm1,markersize=2,marker='h',color='b',mec='b',linewidth=0)
        plot(Corm2,Magm2,markersize=2,marker='h',color='g',mec='g',linewidth=0)
        plot(Corm3,Magm3,markersize=2,marker='h',color='r',mec='r',linewidth=0)
        xlabel(cor)
        ylabel(mag)
        xmin,xmax = xlim()
        ymin,ymax = ylim()
        ylim(ymax,ymin)
        savefig(plotName+str(number)+'mod.png')
        close(count+1)

        figure(count+2, figsize=(6.4, 4.8))
        plot(Cor,Mag,markersize=2,marker='h',color=col,mec=col,linewidth=0)
        xlabel(mag)
        ylabel(cor)
        xlim(xmin,xmax)
        ylim(ymax,ymin)
        savefig(plotName+str(number)+'l.png')
        close(count+2)

        figure(count+3, figsize=(6.4, 4.8))
        plot(Cor,Mag,markersize=2,marker='h',color=col,mec=col,linewidth=0)
        plot(Corm,Magm,markersize=2,marker='h',color='k',linewidth=0)
        xlabel(mag)
        ylabel(cor)
        xlim(xmin,xmax)
        ylim(ymax,ymin)
        savefig(plotName+str(number)+'lmod.png')
        close(count+3)

        return True



#####################################
# Method to make o Color-Color Plot #
#####################################

def plotColor(plotName,number,U,B,V,R,I,Z):
        """
        Method to make a Color-Color plot.
        """

        Cor = []
        Mag = []
        Corm1 = []
        Magm1 = []
        Corm2 = []
        Magm2 = []
        Corm3 = []
        Magm3 = []
        Corm = []
        Magm = []
        if number == 0:
                col = 'b'
                for i in range(len(U)):
                        if U[i] > 2.0 and U[i] < 80.0 and B[i] > 2.0 and B[i] < 80.0 and V[i] > 2.0 and V[i] < 80.0:
                                Cor.append(U[i] - B[i])
                                Mag.append(B[i] - V[i])
                for i in range(len(Z)):
                        if Z[i][0] != "#":
                                Corm.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
                                Magm.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                if Z[i].split()[0] == "1":
                                        Corm1.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
                                        Magm1.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                if Z[i].split()[0] == "2":
                                        Corm2.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
                                        Magm2.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                if Z[i].split()[0] == "3":
                                        Corm3.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
                                        Magm3.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                mag = "B - V"
                cor = "U - B"
        if number == 1:
                col = 'g'
                for i in range(len(B)):
                        if B[i] > 2.0 and B[i] < 80.0 and V[i] > 2.0 and V[i] < 80.0 and R[i] > 2.0 and R[i] < 80.0:
                                Cor.append(B[i] - V[i])
                                Mag.append(V[i] - R[i])
                for i in range(len(Z)):
                        if Z[i][0] != "#":
                                Corm.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                Magm.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                if Z[i].split()[0] == "1":
                                        Corm1.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                        Magm1.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                if Z[i].split()[0] == "2":
                                        Corm2.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                        Magm2.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                if Z[i].split()[0] == "3":
                                        Corm3.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
                                        Magm3.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                mag = "V - R"
                cor = "B - V"
        if number == 2:
                col = 'r'
                for i in range(len(V)):
                        if V[i] > 2.0 and V[i] < 80.0 and R[i] > 2.0 and R[i] < 80.0 and I[i] > 2.0 and I[i] < 80.0:
                                Cor.append(V[i] - R[i])
                                Mag.append(R[i] - I[i])
                for i in range(len(Z)):
                        if Z[i][0] != "#":
                                Corm.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                Magm.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
                                if Z[i].split()[0] == "1":
                                        Corm1.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                        Magm1.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
                                if Z[i].split()[0] == "2":
                                        Corm2.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                        Magm2.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
                                if Z[i].split()[0] == "3":
                                        Corm3.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
                                        Magm3.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
                mag = "R - I"
                cor = "V - R"


        count = 50 + int(100*rand())
        figure(count, figsize=(6.4, 4.8))
        plot(Mag,Cor,markersize=2,marker='h',color=col,mec=col,linewidth=0)
        xlabel(mag)
        ylabel(cor)
        savefig(plotName+str(number)+'.png')
        close(count)

        figure(count+1, figsize=(6.4, 4.8))
        plot(Magm1,Corm1,markersize=2,marker='h',color='b',mec='b',linewidth=0)
        plot(Magm2,Corm2,markersize=2,marker='h',color='g',mec='g',linewidth=0)
        plot(Magm3,Corm3,markersize=2,marker='h',color='r',mec='r',linewidth=0)
        xlabel(mag)
        ylabel(cor)
        xmin, xmax = xlim()
        ymin, ymax = ylim()
        savefig(plotName+str(number)+'mod.png')
        close(count+1)

        figure(count+2, figsize=(6.4, 4.8))
        plot(Mag,Cor,markersize=2,marker='h',color=col,mec=col,linewidth=0)
        xlabel(mag)
        ylabel(cor)
        xlim(xmin,xmax)
        ylim(ymin,ymax)
        savefig(plotName+str(number)+'l.png')
        close(count+2)

        figure(count+3, figsize=(6.4, 4.8))
        plot(Mag,Cor,markersize=2,marker='h',color=col,mec=col,linewidth=0)
        plot(Magm,Corm,markersize=2,marker='h',color='k',linewidth=0)
        xlabel(mag)
        ylabel(cor)
        xlim(xmin,xmax)
        ylim(ymin,ymax)
        savefig(plotName+str(number)+'lmod.png')
        close(count+3)

        return True


############################################################################################
# Method to plot separation between sources from Match with Spectroscopic Redshift Catalog #
############################################################################################

def plotDeltaZ(catFileZ,nome,nomeref,erz,edz):
        """
        Method to plot separation between sources from Match with Spectroscopic Redshift Catalog
        """

        figure(23, figsize=(6.40,6.40))
        h = []
        for i in range(20):
                h.append(-1 + i*0.1)
        ax1 = axes([0.14,0.10,0.6,0.2])
        majorLocator = MultipleLocator(100)
        minorLocator = MultipleLocator(50)
        hist(erz,bins=h,width=0.1,facecolor=None)
        ax1.yaxis.set_major_locator(majorLocator)
        ax1.yaxis.set_minor_locator(minorLocator)
        xlabel(r'$\Delta \alpha\ \rm{(arcsec)}$')
        ylabel('N')
        xlim(-0.99999,0.99999)
        ax2 = axes([0.74,0.30,0.2,0.6])
        hist(edz,bins=h,width=0.1,facecolor=None,orientation='horizontal')
        ax2.xaxis.set_major_locator(majorLocator)
        ax2.xaxis.set_minor_locator(minorLocator)
        xmin, xmax = xlim()
        xlim(xmax,xmin)
        setp(ax2.get_yticklabels(), visible=False)
        xlabel('N')
        ylim(-0.99999,0.99999)
        ax3 = axes([0.14,0.30,0.6,0.6],sharex=ax1,sharey=ax2)
        setp(ax3.get_xticklabels(), visible=False)
        plot(erz,edz, markersize=2, marker='h', color='k', linewidth=0)
        axhline(y=0,c='k',lw=0.5,ls='--')
        axvline(x=0,c='k',lw=0.5,ls='--')
        ylabel(r'$\Delta \delta\ \rm{(arcsec)}$')
        savefig("./"+catFileZ+'delta3.png')
        close(23)
        return True



#####################################
# Method to plot Match (with Spectroscopic Redshift Catalog) Multiplicity #
#####################################

def plotMultDeltaZ(catFileZ,nome,nomeref,nz):
        """
        Method to plot Match (with Spectroscopic Redshift Catalog) Multiplicity.
        """

        h = []
        for i in range(10):
                h.append(i)

        figure(24, figsize=(6.4, 4.8))
        ax = subplot(111)
        majorLocator = MultipleLocator(1)
        hist(nz,bins=h,width=1,fill=False,align='center')
        ymin, ymax = ylim()
        ylim(ymin,ymax+ymax/10)
        xlim(0,10)
        ax.xaxis.set_major_locator(majorLocator)
        xlabel('Number of matches for each object')
        ylabel('N')
        savefig("./"+catFileZ+'multdelta3.png')
        close(24)
        return True



####################################################################################
# Method to plot redshift differences between matched sources from Match (with     #
# Spectroscopic Redshift Catalog) Catalog. Only objects with one association.      #
####################################################################################

def plotDeltazZ(catFileZ,nome,nomeref,zREF1,dz):
        """
        Method to plot redshift differences between matched sources from Match (with Spectroscopic Redshift Catalog) Catalog. Only objects with one association.
        """

        media = 0.0
        med = 0.0
        sig = 0.0

        figure(26, figsize=(6.4, 4.8))
        zval = []
        for i in range (len(dz)):
                zval.append(float(dz[i]))
        media = mean(zval)
        med = median(zval)
        sig = std(zval)
        plot(zREF1,dz, markersize=2, marker='h', color='k', linewidth=0)
        axhline(y=media,ls='--',lw=0.5,c='r')
        axhline(y=med,ls=':',lw=0.5,c='b')
        xlabel('z_spec ('+nomeref+')')
        ylabel('z_spec ('+nomeref+') - z_phot('+nome+')')
        savefig("./"+catFileZ+'deltaz4.png')
        close(26)
        return True


#######################################################################################
# Method to plot photometric redshift vs spectroscopic redshift. Only one association.#
#######################################################################################

def plotzZ(catFileZ,nome,nomeref,zREF1,zIN1):
        """
        Method to plot photometric redshift vs spectroscopic redshift. Only one association.
        """

        figure(27, figsize=(6.4, 4.8))
        plot(zREF1,zIN1, markersize=2, marker='h', color='k', linewidth=0)
        xlabel('z(spec)')
        ylabel('z(phot)')
        name = 'z('+nome+') x z('+nomeref+')'
        t = arange(0,10,0.1)
        s = t
        plot(t,s,markersize=0,color='r',linewidth=0.5,ls='-')
        xlim(0,6)
        ylim(0,6)
        savefig("./"+catFileZ+'z4.png')
        close(27)
        return True


############################################################
# Method to plot Redshift Histogram (only matched objects) #
############################################################

def plotHistZmatch(catFileZ,nome,nomeref,zREF1,zIN1):
        """
        Method to plot Redshift Histogram (only matched objects).
        """


        g = []
        for i in range(100):
                g.append(i*0.2)

        figure(28, figsize=(6.4, 4.8))
        ax = subplot(111)
        minorLocator = MultipleLocator(0.5)
        majorLocator = MultipleLocator(1)
        hist(zIN1,bins=g,width=0.2,fill=False)
        ymin, ymax = ylim()
        ylim(ymin,ymax+ymax/10)
        xlim(0,6)
        ax.xaxis.set_minor_locator(minorLocator)
        ax.xaxis.set_major_locator(majorLocator)
        xlabel('Z')
        ylabel('N')
        savefig(catFileZ+'histzphot.png')
        close(28)


        figure(29, figsize=(6.4, 4.8))
        ax = subplot(111)
        minorLocator = MultipleLocator(0.5)
        majorLocator = MultipleLocator(1)
        hist(zREF1,bins=g,width=0.2,fill=False)
        ymin, ymax = ylim()
        ylim(ymin,ymax+ymax/10)
        xlim(0,6)
        ax.xaxis.set_minor_locator(minorLocator)
        ax.xaxis.set_major_locator(majorLocator)
        xlabel('Z')
        ylabel('N')
        savefig(catFileZ+'histzspec.png')
        close(29)

        return True


######################################
# Method to plot Redshift Histogram  #
######################################

def plotHistZ(catFileZ,nome,z):
        """
        Method to plot Redshift Histogram.
        """


        g = []
        for i in range(100):
                g.append(i*0.2)

        figure(38, figsize=(6.4, 4.8))
        ax = subplot(111)
        minorLocator = MultipleLocator(0.5)
        majorLocator = MultipleLocator(1)
        hist(z,bins=g,width=0.2,fill=False)
        ymin, ymax = ylim()
        ylim(ymin,ymax+ymax/10)
        xlim(0,6)
        ax.xaxis.set_minor_locator(minorLocator)
        ax.xaxis.set_major_locator(majorLocator)
        xlabel('Z')
        ylabel('N')
        savefig(catFileZ+'histz.png')
        close(38)


################################################
# Method to plot Absolute Magnitude Histogram  #
################################################

def plotHistM(catFileZ,nome,m):
        """
        Method to plot Redshift Histogram (only matched objects).
        """


        g = []
        for i in range(100):
                g.append(i*0.5 + floor(min(m)))

        figure(39, figsize=(6.4, 4.8))
        ax = subplot(111)
        minorLocator = MultipleLocator(0.5)
        majorLocator = MultipleLocator(1)
        hist(m,bins=g,width=0.5,fill=False)
        ymin, ymax = ylim()
        ylim(ymin,ymax+ymax/10)
        xlim(floor(min(m)),ceil(max(m)))
        ax.xaxis.set_minor_locator(minorLocator)
        ax.xaxis.set_major_locator(majorLocator)
        xlabel('M')
        ylabel('N')
        savefig(catFileZ+'histm.png')
        close(39)


#################################################
# Method to plot Redshift vs Absolute Magnitude #
#################################################

def plotzM(catFileZ,nome,z,m):
        """
        Method to plot Redshift vs Absolute Magnitude.
        """


        x = []
        med = []
        for i in range(24):
                x1 = (0.25*i)
                x2 = (0.25*i+0.25)
                sum = 0.0
                n = 0.0
                for j in range(len(m)):
                        if z[j] > x1 and z[j] < x2:
                                sum = sum + m[j]
                                n = n + 1
                if sum != 0:
                        med.append(float(sum/n))
                        x.append(x1 + 0.125)


        figure(30, figsize=(6.4, 4.8))
        ax = subplot(111)
        minorLocator = MultipleLocator(0.5)
        majorLocator = MultipleLocator(1)
        plot(z,m,markersize=2,marker='h',color='k',linewidth=0)
        plot(x,med,marker='h',ms=3,c='r',lw=2,markeredgecolor='r')
        ax.xaxis.set_minor_locator(minorLocator)
        ax.xaxis.set_major_locator(majorLocator)
        ymin,ymax = ylim()
        ylim(ymax,ymin)
        xlabel('Z')
        ylabel('M')
        savefig(catFileZ+'zM.png')
        close(30)

        return True


####################################################################
# Method to plot Redshift Histogram (all objects from input files) #
####################################################################

def plotHistzAll(plotName,z,zr,nome,nomeRef):#,value):
        """
        Method to plot Redshift Histogram (all objects from input files).
        """


        g = []
        for i in range(100):
                g.append(i*0.2)

        a = hist(z,bins=g,width=0.2,fill=False)
        b = hist(zr,bins=g,width=0.2,fill=False)

        cla()

        figure(31, figsize=(6.4, 4.8))
        ax = subplot(111)
        minorLocator = MultipleLocator(0.2)
        majorLocator = MultipleLocator(1)
        hist(z,bins=g,width=0.2,fill=False)
        ymin, ymax = ylim()
        ylim(ymin,ymax+ymax/10)
        xlim(0,6)
        ax.xaxis.set_minor_locator(minorLocator)
        ax.xaxis.set_major_locator(majorLocator)
        xlabel('Z')
        ylabel('N')
        savefig(plotName+"phot.png")
        close(31)


        figure(32, figsize=(6.4, 4.8))
        ax = subplot(111)
        minorLocator = MultipleLocator(0.2)
        majorLocator = MultipleLocator(1)
        hist(zr,bins=g,width=0.2,fill=False)
        ymin, ymax = ylim()
        ylim(ymin,ymax+ymax/10)
        xlim(0,6)
        ax.xaxis.set_minor_locator(minorLocator)
        ax.xaxis.set_major_locator(majorLocator)
        xlabel('Z')
        ylabel('N')
        savefig(plotName+'spec.png')
        close(32)

        figure(33, figsize=(6.4, 4.8))
        ax = subplot(111)
        minorLocator = MultipleLocator(0.2)
        majorLocator = MultipleLocator(1)
        bar(b[1],b[0]/float(len(zr)),width=0.2,fill=False,edgecolor='b',lw='1.2')
        bar(a[1],a[0]/float(len(z)),width=0.2,fill=False,edgecolor='r')
        ymin, ymax = ylim()
        ylim(ymin,ymax+ymax/10)
        xlim(0,6)
        ax.xaxis.set_minor_locator(minorLocator)
        ax.xaxis.set_major_locator(majorLocator)
        xlabel('Z')
        ylabel('N')
        savefig(plotName+'.png')
        close(33)

        return True



##########################################################################################
# Method to plot photometric redshifts calculated with Hyperz versus the ones calculated #
# with Le Phare.                                                                         #
##########################################################################################

def plotHzLp(catFileZ1,catFileZ2):
        """
        Method to plot photometric redshifts calculated with Hyperz versus the ones calculated with Le Phare.
        """

        hyperz = open(catFileZ1).readlines()
        lephare = open(catFileZ2).readlines()
        nome = catFileZ1.replace("zphothz","zphot")

        zhz = []
        zlp = []
        n = 43
        for i in range(0,len(hyperz)):
                for j in range(n,len(lephare)):
                        if int(hyperz[i].split()[0]) == int(lephare[j].split()[0]):
                                n = j
                                zhz.append(float(hyperz[i].split()[1]))
                                zlp.append(float(lephare[j].split()[1]))
                                break

        figure(34, figsize=(6.4, 4.8))
        ax = subplot(111)
        zhzmean = mean(zhz)
        zhzmed = median(zhz)
        zlpmean = mean(zlp)
        zlpmed = median(zlp)
        plot(zhz,zlp, markersize=2, marker='h', color='k', linewidth=0)
        xlim(0,6)
        ylim(0,6)
        grid(True)
        xlabel('z (Hiperz)')
        ylabel('z (Le Phare)')
        savefig("./"+nome+'HzLp.png')
        close(34)
        return True



