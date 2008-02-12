#!/usr/bin/env python
"""
This is a library to make plots from an image chosen by QC.
Created by Bruno Rossetto (14/08/2007). 
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


def beginXml():
	writer.startDocument()
	writer.processingInstruction(u'xml-stylesheet', u'type="text/xsl" href="productLog.xsl"')
	writer.startElement(u'analysis')


def writeXml(file,name,sub,stname,stvalue):
	writer.startElement(u'plot')
	writer.simpleElement(u'image', content=u''+file)
	writer.simpleElement(u'name', content=u''+name)
	writer.simpleElement(u'sub', content=u''+sub)
	writer.endElement(u'plot')

	if len(stname) !=0: 
	      writer.startElement(u'statistics')
	      for i in range(len(stname)):
		    writer.simpleElement(u'stats', attributes={u'name':u''+stname[i]}, content=u''+stvalue[i])
	      writer.endElement(u'statistics')

###########################################################
# Method to plot Spatial Distribution of detected objects #
###########################################################

def plotSpatialDistrib(plotName,col,r,d,ramin,ramax,decmin,decmax):
	"""
	Method to plot Spatial Distribution of detected objects
	"""

	masks = open(plotName.replace("ALL","MASKS_0")).read().splitlines()


	figure(1, figsize=(6.4, 4.8))
	ax = subplot(111)
	plot(r,d, markersize=2, marker='h', color=col, mec=col, linewidth=0)
	for i in range(1,len(masks)):
		x = []
		y = []
		n = int(masks[i].split()[0])
		for j in range(1,n+1):
			x.append(float(masks[i].split()[2*j - 1]))
			y.append(float(masks[i].split()[2*j]))
		x.append(float(masks[i].split()[1]))
		y.append(float(masks[i].split()[2]))
		plot(x,y,linewidth=1,markersize=0,c='orange',linewidth=2)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	grid(True)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
#	savefig("./"+plotName+'spatial1.svg')
	savefig(plotName)
	close(1)
	stname = []
	stvalue = []
	name = "Spatial Distribution of Detected Objects"
	sub = "Spatial distribution of detected objects. Showing only objects inside effective area."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"spatial1.png"
#	stname = ["Filter","Effective Area","Total Number of Objects","Number of Galaxies","Number of Stars"]
#	stvalue = [filter,str("%.3f" % area),str(len(ra)),str(len(rg)),str(len(rs))]
#	writeXml(file, name, sub, stname,stvalue)
	return True



############################################################
# Method to plot Spatial Distribution of saturated objects #
############################################################

def plotSatDistrib(plotName):
	"""
	Method to plot Spatial Distribution of saturated objects
	"""

	figure(18, figsize=(6.4, 4.8))
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
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	name = "Spatial Distribution of Saturated Objects"
	sub = "Spatial distribution of saturated objects. Showing only objects inside effective area."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"sat1.png"
#	savefig("./"+plotName+'sat1.svg')
	savefig(plotName)
	stvalue = []
	stname = []
	close(18)
#	writeXml(file, name, sub, stname,stvalue)
	return True


###################################################
# Method to plot Spatial Distribution of Galaxies #
###################################################

def plotSpatialDistribGal(plotName,col,rg,dg,ramin,ramax,decmin,decmax):
	"""
	Method to plot Spatial Distribution of Galaxies
	"""

	masks = open(plotName.replace("ALL","MASKS_0")).read().splitlines()
	

	figure(2, figsize=(6.4, 4.8))
	ax = subplot(111)
	plot(rg,dg, markersize=2, marker='h', color=col, mec=col, linewidth=0)
	for i in range(1,len(masks)):
		x = []
		y = []
		n = int(masks[i].split()[0])
		for j in range(1,n+1):
			x.append(float(masks[i].split()[2*j - 1]))
			y.append(float(masks[i].split()[2*j]))
		x.append(float(masks[i].split()[1]))
		y.append(float(masks[i].split()[2]))
		plot(x,y,linewidth=1,markersize=0,c='orange',linewidth=2)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	grid(True)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	name = 'Spatial Distribution of Galaxies'
	sub = 'Spatial distribution of galaxies.'
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"galspatial1.png"
#	savefig("./"+plotName+'galspatial1.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(2)
#	writeXml(file, name, sub, stname,stvalue)
	return True


################################################
# Method to plot Spatial Distribution of Stars #
################################################

def plotSpatialDistribStars(plotName,col,rs,ds,ramin,ramax,decmin,decmax):
	"""
	Method to plot Spatial Distribution of Stars
	"""

	masks = open(plotName.replace("ALL","MASKS_0")).read().splitlines()


	figure(3, figsize=(6.4, 4.8))
	ax = subplot(111)
	plot(rs,ds,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	for i in range(1,len(masks)):
		x = []
		y = []
		n = int(masks[i].split()[0])
		for j in range(1,n+1):
			x.append(float(masks[i].split()[2*j - 1]))
			y.append(float(masks[i].split()[2*j]))
		x.append(float(masks[i].split()[1]))
		y.append(float(masks[i].split()[2]))
		plot(x,y,linewidth=1,markersize=0,c='orange',linewidth=2)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	grid(True)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')	
	name = 'Spatial Distribution of Stars'
	sub = 'Spatial distribution of stars. Saturated objects are not shown.'
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"starspatial1.png"
#	savefig("./"+plotName+'starspatial1.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(3)
#	writeXml(file, name, sub, stname,stvalue)
	return True


###############################################
# Method to plot Stellarity Index X Magnitude #
###############################################

def plotStellarityXMagnitude(plotName,col,ma,cs):
	"""
	Method to plot Stellarity Index X Magnitude
	"""

	figure(4, figsize=(6.4, 4.8))
	plot(ma,cs, markersize=2, marker='h', color=col, mec=col, linewidth=0)
	ylim(-0.05,1.05)
	grid(True)
	xlabel('m(MAG_AUTO)')
	ylabel('CLASS_STAR')
	name = "Stellarity Index X Magnitude"
	sub = "Star/Galaxy classification. Objects with stellarity index above 0.9 and magnitude less than 21.0 are considered stars/point-like sources."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"stellaritymag0.png"
#	savefig("./"+plotName+'stellaritymag0.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(4)
#	writeXml(file, name, sub, stname, stvalue)
	return True


########################################################
# Method to plot separation between sources from Match #
########################################################

def plotDelta(plotName,matchFile):
	"""
	Method to plot separation between sources from Match
	"""

	if stat(matchFile)[6] == 0:
		return False

	figure(5, figsize=(6.40,6.40))
	h = []
	for i in range(20):
		h.append(-1 + i*0.1)
	ax1 = axes([0.14,0.10,0.6,0.2])
	majorLocator = MultipleLocator(100)
	minorLocator = MultipleLocator(50)
	hist(er,bins=h,width=0.1,facecolor=None)
	ax1.yaxis.set_major_locator(majorLocator)
	ax1.yaxis.set_minor_locator(minorLocator)
	xlabel(r'$\Delta \alpha\ \rm{(arcsec)}$')
	ylabel('N')
	xlim(-0.99999,0.99999)
	ax2 = axes([0.74,0.30,0.2,0.6])
	hist(ed,bins=h,width=0.1,facecolor=None,orientation='horizontal')
	ax2.xaxis.set_major_locator(majorLocator)
	ax2.xaxis.set_minor_locator(minorLocator)
	xmin, xmax = xlim()
	xlim(xmax,xmin)
	setp(ax2.get_yticklabels(), visible=False)
	xlabel('N')
	ylim(-0.99999,0.99999)
	ax3 = axes([0.14,0.30,0.6,0.6],sharex=ax1,sharey=ax2)
	setp(ax3.get_xticklabels(), visible=False)
	plot(er,ed, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=0,c='k',linewidth=0.5,ls='--')
	axvline(x=0,c='k',linewidth=0.5,ls='--')
	ylabel(r'$\Delta \delta\ \rm{(arcsec)}$')
	name = u'\u0394\u03B1 x \u0394\u03B4 (Match)'
	sub = "Spatial differences between matched sources."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"delta3.png"
#	savefig("./"+plotName+'delta3.svg')
	savefig(plotName)
	stname = []
	stvalue = []
#	stname = ["Number of Associated Objects","RA Offset","RA RMS","Dec Offset","Dec RMS","Number of Objects with only One Association","RA Offset","RA RMS","Dec Offset","Dec RMS"]
#	stvalue = [str(len(er)),str("%.3f" % mean(er)),str("%.3f" % std(er)),str("%.3f" % mean(ed)),str("%.3f" % std(ed)),str(len(er1)),str("%.3f" % mean(er1)),str("%.3f" % std(er1)),str("%.3f" % mean(ed1)),str("%.3f" % std(ed1))]
	close(5)
	writeXml(file,name,sub,stname,stvalue)
	return True



###########################################################################
# Method to plot separation between band coordinates and mean coordinates #
###########################################################################

def plotDeltaMean(plotName,col,dr,dd):
	"""
	Method to plot separation between band coordinates and mean coordinates
	"""
	c = 50 + int(100*rand())
	figure(c, figsize=(6.40,6.40))
	h = []
	for i in range(20):
		h.append(-0.05 + i*0.005)
	ax1 = axes([0.14,0.10,0.6,0.2])
	majorLocator = MultipleLocator(10000)
	minorLocator = MultipleLocator(5000)
	hist(dr,bins=h,width=0.005,facecolor=col)
	ax1.yaxis.set_major_locator(majorLocator)
	ax1.yaxis.set_minor_locator(minorLocator)
	xlabel(r'$\Delta \alpha\ \rm{(arcsec)}$')
	ylabel('N')
#                xlim(-0.05,0.05)
	ax2 = axes([0.74,0.30,0.2,0.6])
	hist(dd,bins=h,width=0.005,facecolor=col,orientation='horizontal')
	ax2.xaxis.set_major_locator(majorLocator)
	ax2.xaxis.set_minor_locator(minorLocator)
	xmin, xmax = xlim()
	xlim(xmax,xmin)
	setp(ax2.get_yticklabels(), visible=False)
	xlabel('N')
	ylim(-0.05,0.05)
	ax3 = axes([0.14,0.30,0.6,0.6],sharex=ax1,sharey=ax2)
	setp(ax3.get_xticklabels(), visible=False)
	plot(dr,dd, markersize=2, marker='h', color=col, mec=col, linewidth=0)
	axhline(y=0,c='k',linewidth=0.5,ls='--')
	axvline(x=0,c='k',linewidth=0.5,ls='--')
	ylabel(r'$\Delta \delta\ \rm{(arcsec)}$')
	name = u'\u0394\u03B1 x \u0394\u03B4 (Band Differences)'
	sub = "Spatial differences between band coordinates and mean coordinates."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"deltamean3.png"
#	savefig("./"+plotName+'deltamean3.svg')
	savefig(plotName)
	stname = []
	stvalue = []
#	stname = ["Filter","Number of Objects","RA Offset","Dec Offset","RA RMS","Dec RMS"]
#	stvalue = [filter,str(len(dr)),str("%.5f" % mean(dr)),str("%.5f" % mean(dd)),str("%.5f" % std(dr)),str("%.5f" % std(dd))]
	close(c)
#	writeXml(file,name,sub,stname,stvalue)
	return True



#####################################
# Method to plot Match Multiplicity #
#####################################

def plotMultDelta(catFile,matchFile):
	"""
	Method to plot Match Multiplicity.
	"""

	if stat(matchFile)[6] == 0:
		return False

	h = []
	for i in range(10):
		h.append(i)

	figure(6, figsize=(6.4, 4.8))
	ax = subplot(111)
	majorLocator = MultipleLocator(1)
	hist(n,bins=h,width=1,fill=False,align='center')
	ymin, ymax = ylim()
	ylim(ymin,ymax+ymax/10)
	xlim(0,10)
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlabel('Number of matches for each object')
	ylabel('N')
	name = 'Match Multiplicity'
	sub = "Number of matches for each object."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"multdelta3.png"
#	savefig("./"+catFile+'multdelta3.svg')
	savefig(catFile)
	stname = []
	stvalue = []
	close(6)
	writeXml(file,name,sub,stname,stvalue)
	return True


###########################################################################################
# Method to plot separation between sources from Match. Only objects with ona association #
###########################################################################################

def plotUniDelta(catFile,matchFile):
	"""
	Method to plot separation between sources from Match. Only objects with one association.
	"""

	if stat(matchFile)[6] == 0:
		return False

	figure(21, figsize=(6.40,6.40))
	h = []
	for i in range(20):
		h.append(-1 + i*0.1)
	ax1 = axes([0.10,0.10,0.6,0.2])
	majorLocator = MultipleLocator(100)
	minorLocator = MultipleLocator(50)
	hist(er1,bins=h,width=0.1,facecolor=None)
	ax1.yaxis.set_major_locator(majorLocator)
	ax1.yaxis.set_minor_locator(minorLocator)
	xlabel(r'$\Delta \alpha\ \rm{(arcsec)}$')
	ylabel('N')
	ax2 = axes([0.70,0.30,0.2,0.6])
	hist(ed1,bins=h,width=0.1,facecolor=None,orientation='horizontal')
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
	plot(er1,ed1, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=0,c='k',linewidth=0.5,ls='--')
	axvline(x=0,c='k',linewidth=0.5,ls='--')
	ylabel(r'$\Delta \delta\ \rm{(arcsec)}$')
	name = u'\u0394\u03B1 x \u0394\u03B4 (One Association)'
	sub = "Spatial differences between matched sources. Only objects with one association."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"delta3.png"
#	savefig("./"+catFile+'unidelta3.svg')
	savefig(catFile)
	stname = []
	stvalue = []
	close(21)
	writeXml(file,name,sub,stname,stvalue)
	return True



###################################################################################
# Method to plot magnitude differences between matched sources from Match Catalog #
###################################################################################

def plotDeltaMag(catFile,matchFile):
	""" 
	Method to plot magnitude differences between matched sources from Match Catalog
	"""

	if stat(matchFile)[6] == 0:
		return False

	figure(7, figsize=(6.4, 4.8))
	magval = []
	for i in range (len(dma)):
		if maIN1[i] <= 16.5:
			magval.append(float(dma[i]))
	mean = mean(magval)
	med = median(magval)
	sig = std(magval)
	plot(maIN1,dma, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=mean,ls='--',linewidth=0.5,c='r')
	axhline(y=med,ls=':',linewidth=0.5,c='b')
	xlabel('m(MAG_AUTO)')
	ylabel('m - m(MAG_AUTO)')
	name = u'\u0394m x m'
	sub = "Magnitude comparison with reference catalog."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"deltamag4.png"
#	savefig("./"+catFile+'deltamag4.svg')
	savefig(catFile)
	stname = []
	stvalue = []
#	stname = ["Mean Difference","Median Difference","Difference Dispersion"]
#	stvalue = [str("%.3f" % mean),str("%.3f" % med),str("%.3f" % sig)]
	close(7)
#	writeXml(file,name,sub,stname,stvalue)
	return True


#####################################################################################################
# Method to plot magnitude differences between matched sources from Match Catalog corrected by mean #
#####################################################################################################

def plotMeanDeltaMag(catFile,matchFile):
	"""
	Method to plot magnitude differences between matched sources from Match Catalog corrected by mean
	"""

	if stat(matchFile)[6] == 0:
		return False

	figure(19, figsize=(6.4, 4.8))
	totval = []
	matotval = []
	for i in range (len(dma)):
		if maIN1[i] <= 16.5:
			totval.append(float(dma[i]))
			matotval.append(float(maIN1[i]))
	dmacor = []
	mean = mean(totval)
	median = median(totval)
	for i in range(len(dma)):
		dmacor.append(float(dma[i]-mean))
	med = []
	x = []
	for i in range(30):
		sum = 0
		npts = 0
		x1 = i*0.5 + floor(min(maIN1)) - 0.25
		x2 = i*0.5 + floor(min(maIN1)) + 0.25
		for j in range(len(maIN1)):
			if maIN1[j] > x1 and maIN1[j] < x2 and maIN1[j] < 19.0:
				sum = sum + dmacor[j]
				npts = npts + 1
		if npts > 0:
			med.append(float(sum/npts))
			x.append(float(x2-0.25))
	plot(maIN1,dmacor, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=0,ls='--',linewidth=0.5,c='k')
	plot(x,med,marker='h',ms=2,c='r',linewidth=1.5,markeredgecolor='r')
	xlabel('m(MAG_AUTO)')
	ylabel('m - m(MAG_AUTO)')
	name = u'\u0394m x m (Corrected from Median)'
	sub = "Magnitude comparison with reference catalog. Corrected from mean magnitude difference. Red line shows the difference mean values per magnitude bin."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"mdeltamag4.png"
#	savefig("./"+catFile+'mdeltamag4.svg')
	savefig(catFile)
	stname = []
	stvalue = []
	close(19)
	writeXml(file,name,sub,stname,stvalue)
	return True


############################################################################################################
# Method to plot binned magnitude differences between matched sources from Match Catalog corrected by mean #
############################################################################################################

def plotBinDeltaMag(catFile,matchFile):
	"""
	Method to plot binned magnitude differences between matched sources from Match Catalog corrected by mean
	"""

	if stat(matchFile)[6] == 0:
		return False

	figure(20, figsize=(6.4, 4.8))
	totval = []
	matotval = []
	for i in range (len(dma)):
		if maIN1[i] <= 16.5:
			totval.append(float(dma[i]))
			matotval.append(float(maIN1[i]))
	dmacor = []
	mean = mean(totval)
	median = median(totval)
	for i in range(len(dma)):
		dmacor.append(float(dma[i]-mean))
	med = []
	x = []
	err = []
	for i in range(30):
		sum = 0
		npts = 0
		sig = []
		x1 = i*0.5 + floor(min(maIN1)) - 0.25
		x2 = i*0.5 + floor(min(maIN1)) + 0.25
		for j in range(len(maIN1)):
			if maIN1[j] > x1 and maIN1[j] < x2 and maIN1[j] < 19.0:
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
	axhline(y=0,ls='--',linewidth=0.5,c='k')
	errorbar(x,med,err,marker='h',ms=3,c='r',linewidth=0,markeredgecolor='r')
	xlim(floor(min(maIN1)),ceil(max(maIN1)))
	ylim(floor(min(dmacor)),ceil(max(dmacor)))
	xlabel('m(MAG_AUTO)')
	ylabel('m - m(MAG_AUTO)')
	name = u'\u0394m x m (per bin)'
	sub = "Magnitude comparison with reference catalog. Corrected from mean magnitude difference." 
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"bdeltamag4.png"
#	savefig("./"+catFile+'bdeltamag4.svg')
	savefig(catFile)
	stname = []
	stvalue = []
	close(20)
	writeXml(file,name,sub,stname,stvalue)
	return True


##############################################
# Method to plot Magnitude X Magnitude Error #
##############################################

def plotErrorMag(plotName):
	"""
	Method to plot an Magnitude X Magnitude Error
	"""

	figure(8, figsize=(6.4, 4.8))
	plot(ma,erma, markersize=2, marker='h', color='k', linewidth=0)
	grid(True)
	xmin, xmax = xlim()
	if xmax > 28.0:
		xlim(xmin,28)
	else:
		xlim(xmin,xmax)
	ymin, ymax = ylim()
	if ymax > 0.5:
		ylim(ymin,0.5)
	else:
		ylim(ymin,ymax)
	xlabel('m(MAG_AUTO)')
	ylabel('error m(MAG_AUTO)')
	name = 'Error X Magnitude'
	sub = "Error magnitude vs magnitude."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"errormag4.png"
#	savefig("./"+plotName+'errormag4.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(8)
#	writeXml(file,name,sub,stname,stvalue)
	return True


##########################################
# Method to plot Magnitude X FluxRadius #
##########################################

def plotMagRadius(plotName,col,frg,frs,mag,mas):
	"""
	Method to plot Magnitude X FluxRadius.
	"""

	figure(9, figsize=(6.4, 4.8))
	plot(frg,mag, markersize=2, marker='h', color=col, mec=col, linewidth=0)
	plot(frs,mas, markersize=2, marker='h', color=col, markeredgecolor=col, linewidth=0)
	ymin,ymax = ylim()
	ylim(ymax,ymin)
	xlim(0,10)
	grid(True)
	xlabel('FLUX_RADIUS (pixels)')
	ylabel('m(MAG_AUTO)')
	name = 'Magnitude X Half-Light Radius'
	sub = "Magnitude vs Half-light radius(FLUX_RADIUS)."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"magfluxrad0.png"
#	savefig("./"+plotName+'magfluxrad0.svg')
	savefig(plotName)
	close(9)
	stname = []
	stvalue = []
#	writeXml(file,name,sub,stname,stvalue)
	return True


###########################################
# Method to plot Ellipticity Distribution #
###########################################

def plotEllipticity(plotName):
	"""
	Method to plot Ellipticity Distribution.
	"""	

	e1 = []
	e2 = []
	figure(10, figsize=(6.4, 4.8))
	for i in range(len(xvar)):
		e1.append(float(((xvar[i]-yvar[i])/(xvar[i]+yvar[i]))*cos(2*pa[i]*math.pi/180.0)))
		e2.append(float(((xvar[i]-yvar[i])/(xvar[i]+yvar[i]))*sin(2*pa[i]*math.pi/180.0)))
		a = array([((xvar[i]-yvar[i])/(xvar[i]+yvar[i]))*cos(2*pa[i]*math.pi/180.0)])
		b = array([((xvar[i]-yvar[i])/(xvar[i]+yvar[i]))*sin(2*pa[i]*math.pi/180.0)])
		plot(a,b, markersize=2, marker='h', color='k', linewidth=0)
	axhline(y=0,c='k',linewidth=0.5,ls='--')
	axvline(x=0,c='k',linewidth=0.5,ls='--')
	xlim(-0.5,0.5)
	ylim(-0.5,0.5)
	xlabel('e1')
	ylabel('e2')
	name = 'Ellipticity Distribution'
	sub = "e1 vs e2 plot."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"ellipticity2.png"
#	savefig("./"+plotName+'ellipticity2.svg')
	savefig(plotName)
	stname = []
	stvalue = []
#	stname = ["e1 Mean","e2 Mean","e1 RMS","e2 RMS","Mean FWHM","Median FWHM"]
#	stvalue = [str("%.3f" % mean(e1)),str("%.3f" % std(e1)),str("%.3f" % mean(e2)),str("%.3f" % std(e2)),str("%.3f" % mean(fwhm)),str("%.3f" % median(fwhm))]
	close(10)
#	writeXml(file,name,sub,stname,stvalue)
	return True


#################################
# Method to plot PSF Distortion #
#################################

def plotPsfDistortion(plotName):
	"""
	Method to plot PSF Distortion.
	"""
	
	figure(11, figsize=(6.4, 4.8))
	ax = subplot(111)
	s = 500.0*(ramax-ramin)
	quiver(rs,ds,xrms,yrms,pivot='middle',units='width',headwidth=1,headlength=1,width=0.0025,scale=s)
	majorFormatter = FormatStrFormatter('%3.2f')
	ax.xaxis.set_major_formatter(majorFormatter)
	ax.yaxis.set_major_formatter(majorFormatter)
	axis('image')
	xlim(ramax,ramin)
	ylim(decmin,decmax)
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	name = 'PSF Distortion'
	sub = "Field of vectors of PSF distortion. A vector with inclination of 45 degrees means the same dispersion along x and y axis."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"psfdistort2.png"
#	savefig("./"+plotName+'psfdistort2.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(11)
	writeXml(file,name,sub,stname,stvalue)
	return True


##################################
# Method to plot a PSF Histogram #
##################################

def plotPsfHist(plotName):
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
	name = 'PSF Histogram'
	sub = "Number of objects per FWHM bin."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"psfhist2.png"
#	savefig("./"+plotName+'psfhist2.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(12)
	writeXml(file,name,sub,stname,stvalue)
	return True


#########################################
# Method to plot Galaxy Count Histogram #
#########################################

def plotGalCount(plotName,col,maggal):
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
	hist(maggal,bins=g,width=0.5,fill=True,facecolor=col)
	ymin, ymax = ylim()
	ylim(ymin,ymax+ymax/10)
	xlim(floor(min(maggal))-1,ceil(max(maggal))+1)
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlabel('m(MAG_AUTO)')
	ylabel('N')
	name = 'Galaxy Count'
	sub = "Number of galaxies per magnitude bin."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"galhist5.png"
#	savefig("./"+plotName+'galhist5.svg')
	savefig(plotName)
	stname = []
	stvalue = []
#	stname = ["Filter","Number of Galaxies","Number of Stars","Effective Area","Galaxies per Sq. Deg.","Stars per Sq. Deg."]
#	stvalue = [filter,str(len(maggal)),str(len(magstar)),str("%.3f" % area),str("%.3f" % (len(maggal)/area)),str("%.3f" % (len(magstar)/area))]
	close(13)
#	writeXml(file,name,sub,stname,stvalue)
	return True


#######################################
# Method to plot Star Count Histogram #
#######################################

def plotStarCount(plotName,col,magstar):
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
	hist(magstar,bins=k,width=0.5,fill=True,facecolor=col)
	ymin, ymax = ylim()
	ylim(ymin,ymax+ymax/10)
	xlim(floor(min(magstar))-1,ceil(max(magstar))+1)
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlabel('m(MAG_AUTO)')
	ylabel('N')
	name = 'Star Count'
	sub = "Number of stars per magnitude bin."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"starhist5.png"
#	savefig("./"+plotName+'starhist5.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(14)
#	writeXml(file,name,sub,stname,stvalue)
	return True


##################################################
# Method to plot Star Counts Normalized per area #
##################################################

def plotNormStarCount(plotName,col,magstar,M,area):
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
		for j in range(len(M)):
			if len(M) != 0.0:
				if M[j] > k1 and M[j] < k2:
					SU = SU + 1
		if su != 0:
			me.append(su/area)
		else:
			me.append(0)
		pe.append(sqrt(su)/area)
		if SU != 0:
			ME.append(float(SU))
		else:
			ME.append(0)
		PE.append(sqrt(SU))	

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
		if ME[i] != 0. :
			A.append(x[i])
			B.append(ME[i])
			C.append(PE[i])
	figure(16, figsize=(6.4, 4.8))
	ax = subplot(111)
	minorLocator = MultipleLocator(0.5)
	majorLocator = MultipleLocator(1)
	errorbar(a,b,c,marker='h',markersize=3,c=col,markeredgecolor=col,linewidth=0)
	errorbar(A,B,C,marker='s',markersize=3,c='k',markeredgecolor='k',linewidth=0)
	semilogy()
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlim(floor(min(magstar))-1,ceil(max(magstar))+1)
	xlabel('m(MAG_AUTO)')
	ylabel('N/(sq. deg.)')
	name = 'Star Count Normalized per Area'
	sub = "Star count per area per magnitude bin. Colored dots are values from this work; Black squares are values from Trilegal (Galactic Model)."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"starcount5.png"
#	savefig("./"+plotName+'starcount5.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(16)
#	writeXml(file,name,sub,stname,stvalue)
	return True


####################################################
# Method to plot Galaxy Counts Normalized per area #
####################################################

def plotNormGalCount(plotName,col,maggal,area):
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
	errorbar(a,b,c,marker='h',markersize=3,c=col,mec=col,linewidth=0)
#		errorbar(met,cmet,emet,marker='s',markersize=3,c='r',mec='r',linewidth=0)
#		errorbar(arn,carn,earn,marker='^',markersize=3,c='b',mec='b',linewidth=0)
#		errorbar(ber,cber,eber,marker='d',markersize=3,c='g',mec='g',linewidth=0)
	semilogy()
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlim(floor(min(maggal))-1,ceil(max(maggal))+1)
	xlabel('m(MAG_AUTO)')
	ylabel('N/(sq. deg.)')
	name = 'Galaxy Count Normalized per Area'
#		if catFile.split("/")[-2] == "EIS.2005-06-07T20:32:52.697" or catFile.split("/")[-2] == "EIS.2005-12-09T18:57:47.798":
	sub = "Galaxy count per area per magnitude bin."# Black dots are values from this work, green diamonds are values from Bertin & Dennefeld (1997), red squares are values from Metcalfe et al. (2000) and blue triangles are values from Arnouts et al. (2001).
#		if catFile.split("/")[-2] == "EIS.2005-12-09T22:58:22.340":
#			sub = "Galaxy count per area per magnitude bin. Black dots are values from this work, green diamonds are values from Mamon (1998), red squares are values from Metcalfe et al. (2000) and blue triangles are values from Arnouts et al. (2001)."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"galcount5.png"
#	savefig("./"+plotName+'galcount5.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(17)
#	writeXml(file,name,sub,stname,stvalue)
	return True


#######################################################
# Method to plot Scaled Spatial Distribution of Stars #
#######################################################

def plotScaledStarDistrib(plotName):
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
	xlabel(r'$\alpha\ \rm{(degree)}$')
	ylabel(r'$\delta\ \rm{(degree)}$')
	name = 'Scaled Distribution of Stars'
	sub = "Spatial variations of FWHM."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+plotName.lstrip("./qc")+"scalespatial2.png"
#	savefig("./"+plotName+'scalespatial2.svg')
	savefig(plotName)
	stname = []
	stvalue = []
	close(15)
	writeXml(file,name,sub,stname,stvalue)
	return True


#########################################
# Method to make o Color-Magnitude Plot #
#########################################

def plotColorMag(number,col):
	"""
	Method to make a Color-Magnitude plot.
	"""

	Cor = []
	Mag = []
	Corm = []
	Magm = []
	if number == 0:
		for i in range(len(U)):
			if U[i] > 2.0 and U[i] < 80.0 and B[i] > 2.0 and B[i] < 80.0:
				Cor.append(U[i] - B[i])
				Mag.append(B[i])
		for i in range(len(Z)):
			if Z[i][0] != "#":
				Corm.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
				Magm.append(float(Z[i].split()[12]))
		mag = "B"
		cor = "U - B"
	if number == 1:
		for i in range(len(B)):
			if B[i] > 2.0 and B[i] < 80.0 and V[i] > 2.0 and V[i] < 80.0:
				Cor.append(B[i] - V[i])
				Mag.append(V[i])
		for i in range(len(Z)):
			if Z[i][0] != "#":
				Corm.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
				Magm.append(float(Z[i].split()[13]))
		mag = "V"
		cor = "B - V"
	if number == 2:
		for i in range(len(V)):
			if V[i] > 2.0 and V[i] < 80.0 and R[i] > 2.0 and R[i] < 80.0:
				Cor.append(V[i] - R[i])
				Mag.append(R[i])
		for i in range(len(Z)):
			if Z[i][0] != "#":
				Corm.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
				Magm.append(float(Z[i].split()[14]))
		mag = "R"
		cor = "V - R"
	if number == 3:
		for i in range(len(R)):
			if R[i] > 2.0 and R[i] < 80.0 and I[i] > 2.0 and I[i] < 80.0:
				Cor.append(R[i] - I[i])
				Mag.append(I[i])
		for i in range(len(Z)):
			if Z[i][0] != "#":
				Corm.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
				Magm.append(float(Z[i].split()[15]))
		mag = "I"
		cor = "R - I"
	count = 50 + int(100*rand())
	figure(count, figsize=(6.4, 4.8))
	plot(Cor,Mag,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	xlabel(cor)
	ylabel(mag)
	ymin1, ymax1 = ylim()
	ylim(ymax1,ymin1)
	name = 'Color X Magnitude'
	sub = "Color vs Magnitude plot"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"colormag"+mag+".png"
#	savefig("./"+catFile+'colormag'+mag+'.svg')
	savefig(catFile)
	close(count)
	stname = []
	stvalue = []
	writeXml(file,name,sub,stname,stvalue)

	figure(count+1, figsize=(6.4, 4.8))
	plot(Corm,Magm,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	xlabel(cor)
	ylabel(mag)
	xmin,xmax = xlim()
	ymin,ymax = ylim()
	ylim(ymax,ymin)
	name = 'Color X Magnitude (model)'
	sub = "Color vs Magnitude plot (model)"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"colormag"+mag+"mod.png"
#	savefig("./"+catFile+'colormag'+mag+'mod.svg')
	savefig(catFile)
	close(count+1)
	stname = []
	stvalue = []
	writeXml(file,name,sub,stname,stvalue)

	figure(count+2, figsize=(6.4, 4.8))
	plot(Cor,Mag,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	xlabel(mag)
	ylabel(cor)
	xlim(xmin,xmax)
	ylim(ymax,ymin)
	name = 'Color X Magnitude'
	sub = "Color vs Magnitude plot"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"colormagl"+str(number)+".png"
	savefig("./"+catFile+'colormagl'+str(number)+'.svg')
	savefig("./"+catFile+'colormagl'+str(number)+'.png')
	close(count+2)
	stname = []
	stvalue = []
	writeXml(file,name,sub,stname,stvalue)

	figure(count+3, figsize=(6.4, 4.8))
	plot(Cor,Mag,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	plot(Corm,Magm,markersize=2,marker='h',color='k',linewidth=0)
	xlabel(mag)
	ylabel(cor)
	xlim(xmin,xmax)
	ylim(ymax,ymin)
	name = 'Color X Magnitude'
	sub = "Color vs Magnitude plot"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"colormagl"+str(number)+"mod.png"
	savefig("./"+catFile+'colormagl'+str(number)+'mod.svg')
	savefig("./"+catFile+'colormagl'+str(number)+'mod.png')
	close(count+3)
	stname = []
	stvalue = []
	writeXml(file,name,sub,stname,stvalue)

	return True



#####################################
# Method to make o Color-Color Plot #
#####################################

def plotColor(number,col):
	"""
	Method to make a Color-Color plot.
	"""

	Cor = []
	Mag = []
	Corm = []
	Magm = []
	if number == 0:
		for i in range(len(U)):
			if U[i] > 2.0 and U[i] < 80.0 and B[i] > 2.0 and B[i] < 80.0 and V[i] > 2.0 and V[i] < 80.0:
				Cor.append(U[i] - B[i])
				Mag.append(B[i] - V[i])
		for i in range(len(Z)):
			if Z[i][0] != "#":
				Corm.append(float(Z[i].split()[11]) - float(Z[i].split()[12]))
				Magm.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
		mag = "B - V"
		cor = "U - B"
	if number == 1:
		for i in range(len(B)):
			if B[i] > 2.0 and B[i] < 80.0 and V[i] > 2.0 and V[i] < 80.0 and R[i] > 2.0 and R[i] < 80.0:
				Cor.append(B[i] - V[i])
				Mag.append(V[i] - R[i])
		for i in range(len(Z)):
			if Z[i][0] != "#":
				Corm.append(float(Z[i].split()[12]) - float(Z[i].split()[13]))
				Magm.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
		mag = "V - R"
		cor = "B - V"
	if number == 2:
		for i in range(len(V)):
			if V[i] > 2.0 and V[i] < 80.0 and R[i] > 2.0 and R[i] < 80.0 and I[i] > 2.0 and I[i] < 80.0:
				Cor.append(V[i] - R[i])
				Mag.append(R[i] - I[i])
		for i in range(len(Z)):
			if Z[i][0] != "#":
				Corm.append(float(Z[i].split()[13]) - float(Z[i].split()[14]))
				Magm.append(float(Z[i].split()[14]) - float(Z[i].split()[15]))
		mag = "R - I"
		cor = "V - R"


	count = 50 + int(100*rand())
	figure(count, figsize=(6.4, 4.8))
	plot(Mag,Cor,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	xlabel(mag)
	ylabel(cor)
	name = 'Color X Color'
	sub = "Color vs Color plot"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"color"+str(number)+".png"
	savefig("./"+catFile+'color'+str(number)+'.svg')
	savefig("./"+catFile+'color'+str(number)+'.png')
	close(count)
	stname = []
	stvalue = []
	writeXml(file,name,sub,stname,stvalue)

	figure(count+1, figsize=(6.4, 4.8))
	plot(Magm,Corm,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	xlabel(mag)
	ylabel(cor)
	xmin, xmax = xlim()
	ymin, ymax = ylim()
	name = 'Color X Color (model)'
	sub = "Color vs Color plot (model)"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"color"+str(number)+"mod.png"
	savefig("./"+catFile+'color'+str(number)+'mod.svg')
	savefig("./"+catFile+'color'+str(number)+'mod.png')
	close(count+1)
	stname = []
	stvalue = []
	writeXml(file,name,sub,stname,stvalue)

	figure(count+2, figsize=(6.4, 4.8))
	plot(Mag,Cor,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	xlabel(mag)
	ylabel(cor)
	xlim(xmin,xmax)
	ylim(ymin,ymax)
	name = 'Color X Color'
	sub = "Color vs Color plot"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"colorl"+str(number)+".png"
	savefig("./"+catFile+'colorl'+str(number)+'.svg')
	savefig("./"+catFile+'colorl'+str(number)+'.png')
	close(count+2)
	stname = []
	stvalue = []
	writeXml(file,name,sub,stname,stvalue)

	figure(count+3, figsize=(6.4, 4.8))
	plot(Mag,Cor,markersize=2,marker='h',color=col,mec=col,linewidth=0)
	plot(Magm,Corm,markersize=2,marker='h',color='k',linewidth=0)
	xlabel(mag)
	ylabel(cor)
	xlim(xmin,xmax)
	ylim(ymin,ymax)
	name = 'Color X Color'
	sub = "Color vs Color plot"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFile.lstrip("./qc")+"colorl"+str(number)+"mod.png"
	savefig("./"+catFile+'colorl'+str(number)+'mod.svg')
	savefig("./"+catFile+'colorl'+str(number)+'mod.png')
	close(count+3)
	stname = []
	stvalue = []
	writeXml(file,name,sub,stname,stvalue)

	return True

def endXml():
	writer.endElement(u'analysis')
	writer.endDocument()
	output_xml.close()


def beginSection(section):
	writer.startElement(u'group', attributes={u'name':u''+section} )


def endSection():
	writer.endElement(u'group')
	
	
	
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
	axhline(y=0,c='k',linewidth=0.5,ls='--')
	axvline(x=0,c='k',linewidth=0.5,ls='--')
	ylabel(r'$\Delta \delta\ \rm{(arcsec)}$')
	name = u'\u0394\u03B1 x \u0394\u03B4 ('+nome+')'
	sub = "Spatial differences between matched sources between "+nome+" and "+nomeref+"."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFileZ.lstrip("./qc")+"delta3.png"
#	savefig("./"+catFileZ+'delta3.svg')
	savefig(catFileZ)
	stname = []
	stvalue = []
#	stname = ["Number of Associated Objects","RA Offset","Dec Offset","RA RMS","Dec RMS","Number of Objects with only One Association","RA Offset","Dec Offset","RA RMS","Dec RMS"]
#	stvalue = [str(len(erz)),str("%.3f" % mean(erz)),str("%.3f" % mean(edz)),str("%.3f" % std(erz)),str("%.3f" % std(edz)),str(len(erz1)),str("%.3f" % mean(erz1)),str("%.3f" % mean(edz1)),str("%.3f" % std(erz1)),str("%.3f" % std(edz1))]
	close(23)
#	writeXml(file,name,sub,stname,stvalue)
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
	name = 'Match Multiplicity ('+nome+')'
	sub = "Number of matches for each object between "+nome+" and "+nomeref+"."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFileZ.lstrip("./qc")+"multdelta3.png"
#	savefig("./"+catFileZ+'multdelta3.svg')
	savefig(catFileZ)
	stname = []
	stvalue = []
	close(24)
#	writeXml(file,name,sub,stname,stvalue)
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
	axhline(y=media,ls='--',linewidth=0.5,c='r')
	axhline(y=med,ls=':',linewidth=0.5,c='b')
	xlabel('z_spec ('+nomeref+')')
	ylabel('z_spec ('+nomeref+') - z_phot('+nome+')')
	name = u'\u0394z x z ('+nomeref+')'
	sub = "Comparison of photometric redshift ("+nome+") with spectroscopic redshift ("+nomeref+")."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFileZ.lstrip("./qc")+"deltaz4.png"
#	savefig("./"+catFileZ+'deltaz4.svg')
	savefig(catFileZ)
	stname = []
	stvalue = []
#	stname = ["Mean Difference","Median Difference","Difference Dispersion"]
#	stvalue = [str("%.3f" % mean),str("%.3f" % med),str("%.3f" % sig)]
	close(26)
#	writeXml(file,name,sub,stname,stvalue)
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
	sub = "Comparison of photometric redshift ("+nome+") with spectroscopic redshift ("+nomeref+")."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFileZ.lstrip("./qc")+"z4.png"
#	savefig("./"+catFileZ+'z4.svg')
	savefig(catFileZ)
	stname = []
	stvalue = []
	close(27)
#	writeXml(file,name,sub,stname,stvalue)
	return True


############################################################
# Method to plot Redshift Histogram (only matched objects) #
############################################################

def plotHistzPhot(catFileZ,nome,nomeref,zREF1,zIN1):
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
	name = 'Photometric Redshift Histogram ('+nome+')'
	sub = "Number of matched objects per redshift bin."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFileZ.lstrip("./qc")+"histz5.png"
#	savefig("./"+catFileZ+'histz5.svg')
	savefig(catFileZ)
	stname = []
	stvalue = []
	close(28)
#	writeXml(file,name,sub,stname,stvalue)
	
	return True

def plotHistzSpec(catFileZ,nome,nomeref,zREF1,zIN1):
	"""
	Method to plot Redshift Histogram (only matched objects).
	"""


	g = []
	for i in range(100):
		g.append(i*0.2)

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
	name = 'Spectroscopic Redshift Histogram ('+nomeref+')'
	sub = "Number of matched objects per redshift bin."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFileZ.lstrip("./qc")+"histz.png"
#	savefig("./"+catFileZ+'histz.svg')
	savefig(catFileZ)
	stname = []
	stvalue = []
	close(29)
#	writeXml(file,name,sub,stname,stvalue)

	return True


#################################################
# Method to plot Redshift vs Absolute Magnitude #
#################################################

def plotzM(catFileZ,nome,col):
	"""
	Method to plot Redshift vs Absolute Magnitude.
	"""

	Z2 = open(catFileZ).read().splitlines()

	nz1 = 0
	nnz1 = 0

	m = []
	z = []
	for i in range(len(Z2)):
		if Z2[i][0] != "#" and float(Z2[i].split()[1]) >= 0.0:
			z.append(float(Z2[i].split()[1]))
			m.append(float(Z2[i].split()[col]))

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
	plot(x,med,marker='h',ms=3,c='r',linewidth=2,markeredgecolor='r')
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	ymin,ymax = ylim()
	ylim(ymax,ymin)
	xlabel('Z')
	ylabel('M')
	name = 'Absolute Magnitude X Redshift ('+nome+')'
	sub = "Absolute magnitude vs photometric redshift."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFileZ.lstrip("./qc")+"zM5.png"
#	savefig("./"+catFileZ+'zM5.svg')
	savefig(catFileZ)
	stname = []
	stvalue = []
	close(30)
#	writeXml(file,name,sub,stname,stvalue)

	return True


####################################################################
# Method to plot Redshift Histogram (all objects from input files) #
####################################################################

def plotHistzAll(catFileZ,refFileZ,col,nome,nomeRef):#,value):
	"""
	Method to plot Redshift Histogram (all objects from input files).
	"""

	
	Z2 = open(catFileZ).read().splitlines()
	Z3 = open(refFileZ).read().splitlines()
	nz1 = 0
	nnz1 = 0
	nzr = 0
	z = []
	zr = []
	for i in range(len(Z2)):
		if Z2[i][0] != "#" and float(Z2[i].split()[1]) >= 0.0:
			z.append(float(Z2[i].split()[1]))
			nz1 = nz1 + 1
		if Z2[i][0] != "#" and float(Z2[i].split()[1]) < 0.0:
			nnz1 = nnz1 + 1  # number of objects with no redshift solution

	for i in range(len(Z3)):
		if Z3[i][0] != "#":
			zr.append(float(Z3[i].split()[col]))
			nzr = nzr + 1

	g = []
	for i in range(100):
		g.append(i*0.2)

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
	name = 'Photometric Redshift Histogram ('+nome+')'
	sub = "Number of objects per redshift bin."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+catFileZ.lstrip("./qc")+"histz5all.png"
#	savefig("./"+catFileZ+'histz5all.svg')
	savefig(catFileZ)
	stname = []
	stvalue = []
#	stname = ["Number of Objects with redshift solution ("+nome+")","Number of Objects with no redshift solution ("+nome+")"]
#	stvalue = [str(nz1),str(nnz1)]
	close(31)
#	writeXml(file,name,sub,stname,stvalue)
	

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
	name = 'Spectroscopic Redshift Histogram ('+nomeRef+')'
	sub = "Number of objects per redshift bin."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+refFileZ.lstrip("./qc")+"histzall.png"
	savefig("./"+refFileZ+'histzall.svg')
	savefig("./"+refFileZ+'histzall.png')
	stname = []
	stvalue = []
#	stname = ["Number of Objects ("+nomeRef+")"]
#	stvalue = [str(nzr)]
	close(32)
#	writeXml(file,name,sub,stname,stvalue)

	figure(33, figsize=(6.4, 4.8))
	ax = subplot(111)
	minorLocator = MultipleLocator(0.2)
	majorLocator = MultipleLocator(1)
	hist(zr,bins=g,width=0.2,fill=False,edgecolor='b',normed=1)
	hist(z,bins=g,width=0.2,fill=False,edgecolor='r',normed=1)
	ymin, ymax = ylim()
	ylim(ymin,ymax+ymax/10)
	xlim(0,6)
	ax.xaxis.set_minor_locator(minorLocator)
	ax.xaxis.set_major_locator(majorLocator)
	xlabel('Z')
	ylabel('N')
	name = 'Redshift Histogram ('+nomeRef+')'
	sub = "Number of objects per redshift bin. Blue line represents spectroscopic redshift ("+nomeRef+"); red line represents photometric redshift ("+nome+")"
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+nome+nomeRef+"histznorm.png"
	savefig("./"+nome+nomeRef+'histznorm.svg')
	savefig("./"+nome+nomeRef+'histznorm.png')
	stname = []
	stvalue = []
#	stname = ["Number of Objects ("+nomeRef+")"]
#	stvalue = [str(nzr)]
	close(33)
#	writeXml(file,name,sub,stname,stvalue)

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
	name = 'z(Hiperz) x z(Le Phare)'
	sub = "Comparison of photometric redshift methods."
	file = "http://astroserver02.on.br:81/~rossetto/ColorCatalog/"+nome+"HzLp.png"
	savefig("./"+nome+'HzLp.svg')
	savefig("./"+nome+'HzLp.png')
	stname = []
	stvalue = []
#	stname = ["Number of objects","Mean z (Hiperz)","Median z (Hiperz)","Mean z (Le Phare)","Median z (Le Phare)",]
#	stvalue = [str(len(zhz)),str("%.3f" % zhzmean),str("%.3f" % zhzmed),str("%.3f" % zlpmean),str("%.3f" % zlpmed)]
	close(34)
#	writeXml(file,name,sub,stname,stvalue)
	return True



#####################
# Beginning of Main #
#####################

if __name__ == "__main__":
	mkplot = plotFactory(sys.argv[1],sys.argv[2])

################
# End of Class #
################

