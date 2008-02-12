#!/usr/bin/env python
"""
This is a module to make plots from a catalog chosen by QC Prototype.
(output from match between a photometric redshift catalog and a reference spectroscopic redshift catalog)
Created by Bruno Rossetto (14/08/2007).
"""

try:
        import sys
	from os import stat
        from math import cos, sin, pi
        import plotFactory
	import pipeline.io as io
except ImportError, err:
        print err
        exit()
def run():

	fio = io.ComponentIO()
	confio = io.ComponentConfig()

	nomeref = confio.getScalarById('reference_name')
	nome = confio.getScalarById('zphot_name')

	catName = fio.getFileById('match_catalog')

	X = open(catName).read().splitlines()

        erz = []
        erz1 = []
        edz = []
        edz1 = []
        nz = []
        zIN = []
        zIN1 = []
        zREF = []
        zREF1 = []
        dz = []

# Matching of Photometric Redshift with Spectroscopic Redshift from Reference Catalog
        if stat(catName)[6] != 0:
                for i in range(len(X)):
                        erz.append(float(X[i].split()[0]))
                        edz.append(float(X[i].split()[1]))
                        zIN.append(float(X[i].split()[2]))
                        zREF.append(float(X[i].split()[3]))
                        nz.append(int(X[i].split()[5]))
                for i in range(len(X)-1):
                        if int(X[i].split()[5]) < 2 and int(X[i+1].split()[5]) < 2:
                                erz1.append(float(X[i].split()[0]))
                                edz1.append(float(X[i].split()[1]))
                                if zIN[i] < 40 and zIN[i] >= 0 and zREF[i] >= 0 and zREF[i] < 40:
                                        dz.append(float(X[i].split()[4]))
                                        zIN1.append(float(X[i].split()[2]))
                                        zREF1.append(float(X[i].split()[3]))
                if X[len(X)-1].split()[5] < 2:
                        erz1.append(float(X[len(X)-1].split()[0]))
                        edz1.append(float(X[len(X)-1].split()[1]))
                        if zIN[len(X)-1] < 40 and zIN[len(X)-1] >= 0 and zREF[len(X)-1] >= 0 and zREF[len(X)-1] < 40:
                                dz.append(float(X[len(X)-1].split()[4]))
                                zIN1.append(float(X[len(X)-1].split()[2]))
                                zREF1.append(float(X[len(X)-1].split()[3]))


	plotFactory.plotDeltaZ(catName+"deltaz.png",nome,nomeref,erz,edz)
	fio.addOutput(catName+"deltaz.png", "plot_deltaz", "png_image","file")
	plotFactory.plotMultDeltaZ(catName+"multdeltaz.png",nome,nomeref,nz)
	fio.addOutput(catName+"multdeltaz.png", "plot_multdeltaz", "png_image","file")
	plotFactory.plotDeltazZ(catName+"deltazz.png",nome,nomeref,zREF1,dz)
	fio.addOutput(catName+"deltazz.png", "plot_deltazz", "png_image","file")
	plotFactory.plotzZ(catName+"zz.png",nome,nomeref,zREF1,zIN1)
	fio.addOutput(catName+"zz.png", "plot_zz", "png_image","file")
	plotFactory.plotHistzPhot(catName+"histzphot.png",nome,nomeref,zREF1,zIN1)
	fio.addOutput(catName+"histzphot.png", "plot_histz_phot", "png_image","file")
	plotFactory.plotHistzSpec(catName+"histzspec.png",nome,nomeref,zREF1,zIN1)
	fio.addOutput(catName+"histzspec.png", "plot_histz_spec", "png_image","file")


