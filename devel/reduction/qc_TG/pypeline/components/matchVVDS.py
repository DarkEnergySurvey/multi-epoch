#! /usr/bin/env python


###########
# Imports #
###########

try:
        import sys
	import match
        from math import pi
        from os import stat
	import pipeline.io as io
except ImportError, err:
        print err
        exit()


def run():

	confio = io.ComponentConfig()

	sep = confio.getScalarById('angular_separation')

	FAT = pi/(180.0*3600.0)  # Unit Transformation (degree -> radian).
	E = sep*FAT # Hard wired for the separation of one arcsec.
	catIN = [[],[],[]] # Matrix of RA/DEC/z of input catalog.
	catREF = [[],[],[]] # Matrix of RA/DEC/z of reference catalog.

	fio = io.ComponentIO()

#	fin1 = open("Deep2c_col_opt_ir2.zphotlp").read().splitlines()
	fin1 = open(fio.getFileById('zphot_catalog')).read().splitlines()
#	fin2 = open("Deep2c_col_opt_ir2.ALL").read().splitlines()
	fin2 = open(fio.getFileById('color_catalog')).read().splitlines()
#	fref = open("VVDS.asc").read().splitlines()
	fref = open(fio.getFileById('reference_catalog')).read().splitlines()

	fout = fio.getFileById('zphot_catalog').replace('zphot','match')+"VVDS"


	n = 0
	for i in range(len(fin1)): # Storing data on matrix of input catalog.
		if fin1[i][0] != "#":
			for j in range(n,len(fin2)):
				if fin2[j][0] != "#" and int(fin2[j].split()[0]) == int(fin1[i].split()[0]) and float(fin1[i].split()[1] >= 0.0):
					n = j
					catIN[0].append(float(fin2[j].split()[1])*FAT*3600.0)
					catIN[1].append(float(fin2[j].split()[2])*FAT*3600.0)
					catIN[2].append(float(fin1[i].split()[1]))
					break
	for i in range(len(fref)): # Storing data on matrix of reference catalog.
		if fref[i][0] != "#":
			catREF[0].append(float(fref[i].split()[1])*FAT*3600.0)
			catREF[1].append(float(fref[i].split()[2])*FAT*3600.0)
			catREF[2].append(float(fref[i].split()[3]))

	match.match(catIN,catREF,E,fout)

	fio.addOutput(fout, "match_catalog", "ascii_catalog","file")
