#!/usr/bin/env python

import os,sys
import math
import despydb
import matplotlib.pyplot as plt
from matplotlib.patches import Ellipse, Polygon
import numpy
from despyastro import astrometry


"""
This is the transaltion of gencoaddtile.tcl in order to
reproduce the *same* centers as before, following Brian's code
Some problems with the code,are as it is now:
 1) The code goes beyond 360 and produces:
    URALL> 360 or URAUR > 360
    RALL > 360 or RALR > 360 or RAUL > 360 or RAUR > 360

 2) Left is Right and Right is Left in the common astronomical
    reference frame.

  3) the tile corners RA, DEC [LL,LR,UL,UR] are not consistent with
     the definition in the *.head files that are used later on by
     SWarp

  4) Tiles at just below RA<275 have a bad overlap with tiles starting
     at RA=275

We'd like to avoid all of these in the future version of the code.

Felipe Menanteau, April 2014

"""

# These are the default limits from Brian.'s tcl code.
# Set up the limits
RA_range = 360         # We need 360 to complete sky
dec0 = +30.0           # kept from original code to preserve centers
dec1 = -89.99          # Get crazy beyong abs(85) kept for consistency only

ra0  = 275.0           # kept from original code to preserve centers
#ra0 = 275.0 - 360.0   # Hack to go for -- to ++ usefuld for plottinh
ra1  = ra0 + RA_range  # Kept from original code to preserve centers

d2r = math.pi/180. # degrees to radians shorthand

overlap1  =  1/60.
overlap2  =  2/60.
overlap5  =  5/60.
overlap10 = 10/60.

######################################
# This sets up the size of the tile
pixscale = 0.263
naxis    = 10000
naxis1   = naxis 
naxis2   = naxis
######################################


try:
    desdmfile = os.environ["des_services"]
except:
    desdmfile = None

# Connect to desoper for comparisons
dbh = despydb.desdbi.DesDbi(desdmfile,"db-desoper")
cur = dbh.cursor()

# Matplotlib handle
plt.figure(1,figsize = (15,15))
ax = plt.gca()

# Define plotting boundaries
pwidth = 20
r1  = ra0
r2  = ra0  + pwidth
d2  = dec0
d1  = dec0 - pwidth

# Open outfile and prepare header
coaddtable_redo = "coaddtiles_table_reproduced.dat"
co = open(coaddtable_redo,"w")
co.write("# %-10s "   % 'tilename')
co.write("%10s %10s " % ('ra','dec'))
co.write("%8s "       % 'pixscale')
co.write("%6s %6s "   % ('naxis1','naxis2'))
co.write("%10s "*8    % ('rall','decll','raul','decul','raur','decur','ralr','declr'))
co.write("%10s "*4    % ('ural','urau','udecl','udecu'))
co.write("\n")

done = False
dec = dec0
ntiles = 1
while done is False:

    # OVERLAP!
    # one arcminute overlap, works everywhere but close to pole (dec < -75 deg)
    # Completely add-hoc definition!  It should be changed to a
    # desirable overlap. We would like overlap should be 1 arcminute ALWAYS
    # regardless of RA,DEC!!!  We could ajust the code so increase in ra,dec
    # are diferent and allows for such overlap, but that will change the centroids
    # We should have ra_overlap, dec_overlap
    if abs(dec) < 74:
        overlap = overlap1 # This is the one we want all the time
    elif abs(dec) < 82.5:
        overlap = overlap2
    elif abs(dec) < 87.6:
        overlap = overlap5
    else:
        overlap = overlap10

    # Various width defintions
    # Width in degrees
    degwidth     = pixscale*naxis/3600.
    # Width for declination
    dechalfwidth  =  degwidth/2.0
    # Unique Width for declination
    udechalfwidth = (degwidth-overlap)/2.0
    # Increase in declination (silly -1 sign)
    incrdec  = -1.0*(degwidth - overlap)
    
    ra      = ra0
    firstra = 1

    counter = 1
    while ra < ra1:

        # Dec uppper (decu) and lower (decl), signs don't make sense in original code, were reversed
        decu = dec + dechalfwidth # DECU = DECUL = DECUR | Left and Right are equal in Decl.
        decl = dec - dechalfwidth # DECL = DECLL = DECLR | Left and Right are equal in Decl.

        # DECU = DECUL = DECUR
        decul = decu 
        decur = decu
        # DECL = DECLL = DECLR
        decll = decl 
        declr = decl
        
        # RA upper, lower and middle/unique increases
        rauincr = degwidth/math.cos(decu*d2r)/2.0 # Upper increase
        ralincr = degwidth/math.cos(decl*d2r)/2.0 # Lower increse
        raaincr = (degwidth - overlap)/math.cos(dec*d2r)/2.0 # RA increase for unique vals

        # RA Uppers RAUL, RAUR
        raul = ra - rauincr
        raur = ra + rauincr
        # RA Lowers RALL, RALR
        rall = ra - ralincr
        ralr = ra + ralincr
        
        # Unique Dec. lower and upper bounds
        udecl = dec - udechalfwidth
        udecu = dec + udechalfwidth

        # Unique lower and uppper bounds
        ural = ra - raaincr
        urau = ra + raaincr

        # !!??
        if raul > 360 and raur > 360 and rall > 360 and ralr > 360 and ural > 360 and urau > 360 :
            raul = raul - 360.0
            raur = raur - 360.0
            rall = rall - 360.0
            ralr = ralr - 360.0
            ural = ural - 360.0
            urau = urau - 360.0

        if firstra == 1:
            saveural = ural
            firstra = 0

        # Quick fix for formating the RA
        if ra > 360:
            raavgp = ra - 360.0
        elif ra < 0: # In case we want to do negative RAs but want to keep names sanes
            raavgp = ra + 360.0
        else:
            raavgp = ra

        prefix = 'DES'
        RA  = astrometry.dec2deg(raavgp/15.,sep="",short='yes')
        DEC = astrometry.dec2deg(dec,       sep="",short='yes')
        if dec < 0.0:
            tilename = "%s%s%s" % (prefix,RA,DEC)
        else:
            tilename = "%s%s+%s" % (prefix,RA,DEC)
            
        newra = ra + (degwidth - overlap)/math.cos(dec*d2r)

        # We only need this for a full wrap around
        if newra > ra1 and RA_range>=360:
            urau = saveural

        # ------------- DB Section ------------------------
        # Query the COADDTILE table to compare results
        query = """select TILENAME,
                          RA,DEC,
                          RALL,  RALR,  RAUL,   RAUR,  
                          DECLL, DECLR, DECUL,  DECUR,
                          URALL, URAUR, UDECLL, UDECUR 
                          from COADDTILE where TILENAME='%s'""" % tilename
        cur.execute(query)
        (TILENAME,
         RA,DEC,
         RALL,  RALR,  RAUL,   RAUR, 
         DECLL, DECLR, DECUL,  DECUR,
         URALL, URAUR, UDECLL, UDECUR) = cur.fetchone()
        # ------------- DB Section ------------------------

        sys.stdout.write("# Comparing %s -- ntiles: %8d\r" % (tilename,ntiles))
        sys.stdout.flush()
        ntiles = ntiles + 1
        
        # Check that tilenames are consistent
        if TILENAME != tilename:
            print "Tilenames are not the same!!!"

        # Check for variations in the centroid
        dRA  = abs(raavgp - RA)
        dDEC = abs(dec    - DEC)
        if dRA > 1e-3 or dDEC > 1e-3:
            print "problem for %s, %s %s" % (tilename, dRA, dDEC)
            
        # Check (U)pper and (L)ower RAs
        dURAL = abs(URALL-ural)
        dURAU = abs(URAUR-urau)
        if dURAL > 1e-3 or dURAU > 1e-3:
            print "problem for %s, %s %s" % (tilename, dURAL, dURAU)

        # Check (U)pper and (L)ower DECs
        dUDECL = abs(UDECLL-udecl)
        dUDECU = abs(UDECUR-udecu)
        if dUDECL > 1e-3 or dUDECU > 1e-3:
            print "problem for %s, %s %s" % (tilename, dUDECL, dUDECU)


        # Save the informatio to output file
        co.write("%12s "          % tilename)
        co.write("%10.6f %10.6f " % (raavgp,dec))
        co.write("%8.4f "         % pixscale)
        co.write("%6d %6d "       % (naxis1,naxis2))
        co.write("%10.6f "*8      % (rall,decll,raul,decul,raur,decur,ralr,declr))
        co.write("%10.6f "*4      % (ural,urau,udecl,udecu))
        co.write("\n")
        
        # Only plot a subsection of it -- to speed up things
        if raavgp > r1 and raavgp < r2 and dec > d1 and dec < d2:

            # Make the unique region polygons as a set of points
            ras  = numpy.array([rall,ralr,raur,raul])
            decs = numpy.array([decll,declr,decul,decur])

            P = Polygon( zip(ras,decs), closed=True, fill=False, hatch='', ec='b')
            ax.add_patch(P)

            # Make the unique region polygons as a set of points
            uras  = numpy.array([ural ,urau, urau, ural])
            udecs = numpy.array([udecl,udecl,udecu,udecu])
            # plt.plot(uras,udecs,'r-')
            P = Polygon( zip(uras,udecs), closed=True, fill=False, hatch='', ec='r')
            ax.add_patch(P)

            # And the centroid
            plt.plot(ra,dec,'k.')

        ra = newra
        counter = counter + 1
        # End or RA loop

    # Increase in Declination
    dec = dec + incrdec
    # Stop when we reach dec1
    if dec < dec1: done = True

print "# Done, wrote new table to: %s" % coaddtable_redo
# Plot out
plt.show()
plt.close()
