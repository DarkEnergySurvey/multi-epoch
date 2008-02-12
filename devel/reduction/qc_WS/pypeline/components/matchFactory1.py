#!/usr/bin/env python

# Library of match algorithms.
# Created by Bruno Rossetto.

###########
# Imports #
###########

try:
	import sys
	from math import pi, sqrt
	from os import stat
except ImportError, err:
        print err
        exit()

#####################################
# Astrometric match of two catalogs #
#####################################
	
def astroMatch(matchName,raIn,decIn,raRef,decRef,E):
	"""
	Method to compare catalogs and produce a astrometric match catalog.
	"""
	
##############################
# Opening file to be written #
##############################
	fw = open(matchName,"w")

########################################################
# Converting ra, dec and angular separation to radians #
########################################################
	E = (E/3600.0)*pi/180.0
	for i in range(len(raIn)):
		raIn[i] = raIn[i]*pi/180.0
		decIn[i] = decIn[i]*pi/180.0
	for i in range(len(raRef)):
		raRef[i] = raRef[i]*pi/180.0
		decRef[i] = decRef[i]*pi/180.0



################################################
# Comparing Catalogs and searching for matches #
################################################
	for x in range(len(raIn)):
		n = 0
		for y in range(len(raRef)):
			if abs(raIn[x]-raRef[y]) <= E and abs(decIn[x]-decRef[y]) <= E:
				n = n + 1
				dra = raIn[x]-raRef[y]
				ddec = decIn[x]-decRef[y]
				fw.write(str("%.8f" %(raIn[x]*180.0/pi))+"\t"+str("%.8f" %(decIn[x]*180.0/pi))+"\t"+str("%.8f" %(dra*3600.0*180.0/pi))+"\t"+str("%.8f" %(ddec*3600.0*180.0/pi))+"\t"+str(n)+"\n")

#################
# Closing files #
#################
	fw.close()



#####################################
# Photometric match of two catalogs #
#####################################
def photoMatch(matchName,raIn,decIn,magIn,raRef,decRef,magRef,E):
	"""
	Method to compare catalogs and produce a photometric match catalog.
	"""
	
##############################
# Opening file to be written #
##############################
	fw = open(matchName,"w")

########################################################
# Converting ra, dec and angular separation to radians #
########################################################
	E = (E/3600.0)*pi/180.0
	for i in range(len(raIn)):
		raIn[i] = raIn[i]*pi/180.0
		decIn[i] = decIn[i]*pi/180.0
	for i in range(len(raRef)):
		raRef[i] = raRef[i]*pi/180.0
		decRef[i] = decRef[i]*pi/180.0



################################################
# Comparing Catalogs and searching for matches #
################################################
	for x in range(len(raIn)):
		n = 0
		for y in range(len(raRef)):
			if abs(raIn[x]-raRef[y]) <= E and abs(decIn[x]-decRef[y]) <= E:
				n = n + 1
				dmag = magRef[y]-magIn[x]
				fw.write(str("%.3f" %(magIn[x]))+"\t"+str("%.3f" %(magRef[y]))+"\t"+str("%.3f" %(dmag))+"\t"+str(n)+"\n")

#################
# Closing files #
#################
	fw.close()


###############################
# Cross-match of two catalogs #
###############################
def crossMatch(matchName,raIn,decIn,magIn,raRef,decRef,magRef,E):
        """
        Method to compare catalogs and produce a cross-match catalog.
        """

##############################
# Opening file to be written #
##############################
        fw = open(matchName,"w")

########################################################
# Converting ra, dec and angular separation to radians #
########################################################
        E = (E/3600.0)*pi/180.0
        for i in range(len(raIn)):
                raIn[i] = raIn[i]*pi/180.0
                decIn[i] = decIn[i]*pi/180.0
        for i in range(len(raRef)):
                raRef[i] = raRef[i]*pi/180.0
                decRef[i] = decRef[i]*pi/180.0



################################################
# Comparing Catalogs and searching for matches #
################################################
        for x in range(len(raIn)):
                n = 0
                for y in range(len(raRef)):
                        if abs(raIn[x]-raRef[y]) <= E and abs(decIn[x]-decRef[y]) <= E:
                                n = n + 1
				dra = raIn[x]-raRef[y]
				ddec = decIn[x]-decRef[y]
                                dmag = magRef[y]-magIn[x]
                                fw.write(str("%.4f" %(raIn[x]))+"\t"+str("%.4f" %(decIn[x]))+"\t"+str("%.4f" %(dra))+"\t"+str("%.4f" %(ddec))+"\t"+str("%.4f" %(magIn[x]))+"\t"+str("%.4f" %(magRef[y]))+"\t"+str("%.4f" %(dmag))+"\t"+str(n)+"\n")

#################
# Closing files #
#################
        fw.close()

