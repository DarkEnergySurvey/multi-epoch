#!/usr/bin/env python

""" A series of plots to make sure that limits are set properly"""

import os,sys
import numpy
import matplotlib
#matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches     import Polygon
from matplotlib.collections import PatchCollection
import despyastro
import despymisc
from despyastro import tableio
import time


try:
    coaddtable = sys.argv[1]
except:
    coaddtable = 'coaddtiles_table_reproduced.dat'

# Get the start time
t0 = time.time()

print "# Reading %s" % coaddtable
cols = range(18)[1:]
(tilename) = tableio.get_str(coaddtable,cols=(0,))
(ra,dec,pixscale,naxis1,naxis2,
 rall,decll,raul,decul,raur,decur,ralr,declr,
 ural,urau,udecl,udecu) = tableio.get_data(coaddtable,cols=cols)

# Make them unique region polygons as a set of points
Ntiles = len(ra)
ras   = numpy.array([rall,ralr,raur,raul])
decs  = numpy.array([decll,declr,decul,decur])
uras  = numpy.array([ural ,urau, urau, ural])
udecs = numpy.array([udecl,udecl,udecu,udecu])

# Re-center on zero
ra   = numpy.where(ra   >= 275, ra  -360, ra)
ras  = numpy.where(ras  >= 275, ras -360, ras)
uras = numpy.where(uras >= 275, uras-360, uras)

################################
# A Large subsection of the sky
################################
dRA  = 60
dDEC = dRA
ra1   = -30
ra2   = ra1  + dRA
dec1  = -30
dec2  = dec1 + dDEC

plt.figure(1,figsize = (15,15))
ax = plt.gca()
patches = []
for k in range(Ntiles):

    if ra[k] > ra1 and ra[k] < ra2 and dec[k]> dec1 and dec[k]< dec2:
        #plt.scatter(ra[k],dec[k],marker='.',s=5,alpha=0.5,color='k')
        P1 = Polygon( zip(ras[:,k], decs[:,k]),  lw=0.5, ls="-", closed=True, fill=False, hatch='', ec='b')
        #P2 = Polygon( zip(uras[:,k],udecs[:,k]), lw=0.5, ls="--",closed=True, fill=False, hatch='', ec='r')
        patches.append(P1)  
        #patches.append(P2) 
p = PatchCollection(patches,match_original=True)#, cmap=matplotlib.cm.jet, alpha=0.4)
ax.add_collection(p)
plt.xlabel("R.A. (degrees)")
plt.ylabel("Decl.(degrees)")
plt.xlim(ra1,ra2)
plt.ylim(dec1,dec2)
print >>sys.stderr,"# Time:%s" % despymisc.miscutils.elapsed_time(t0)
plt.savefig("DES-zoom0.png")

#plt.show()
#sys.exit()

################################
# A Small subsection of the sky
################################
dRA  = 2.0
dDEC = dRA
ra1   = -10
ra2   = ra1  + dRA
dec1  = -30
dec2  = dec1 + dDEC

plt.figure(2,figsize = (15,15))
ax = plt.gca()
patches = []
for k in range(Ntiles):
    if ra[k] > ra1-1 and ra[k] < ra2+1 and dec[k]> dec1-1 and dec[k]< dec2+1: # +/-1 to make sure we get enough
        P1 = Polygon( zip(ras[:,k], decs[:,k]),  lw=0.5, ls="-", closed=True, fill=False, hatch='', ec='b')
        P2 = Polygon( zip(uras[:,k],udecs[:,k]), lw=0.5, ls="--",closed=True, fill=False, hatch='', ec='r')
        patches.append(P1)  
        patches.append(P2)
    if ra[k] > ra1 and ra[k] < ra2 and dec[k]> dec1  and dec[k]< dec2:
        plt.text(ra[k],dec[k],tilename[k],horizontalalignment='center', verticalalignment='center')

p = PatchCollection(patches,match_original=True)#, cmap=matplotlib.cm.jet, alpha=0.4)
ax.add_collection(p)
plt.xlabel("R.A. (degrees)")
plt.ylabel("Decl.(degrees)")
plt.xlim(ra1,ra2)
plt.ylim(dec1,dec2)
print >>sys.stderr,"# Time:%s" % despymisc.miscutils.elapsed_time(t0)
plt.savefig("DES-zoom1.png")

#################
# All sky next
#################
plt.figure(3,figsize = (30,15))
ax = plt.gca()
# The centers of the tiles
plt.scatter(ra,dec,marker='.',s=5,alpha=0.5,color='k')

patches = []
for k in range(Ntiles):
    P1 = Polygon( zip(ras[:,k], decs[:,k]),  lw=0.5, ls="-", closed=True, fill=False, hatch='', ec='b')
    #P2 = Polygon( zip(uras[:,k],udecs[:,k]), lw=0.5, ls="--",closed=True, fill=False, hatch='', ec='r')
    patches.append(P1)  
    #patches.append(P2) 

p = PatchCollection(patches,match_original=True)#, cmap=matplotlib.cm.jet, alpha=0.4)
ax.add_collection(p)

plt.xlabel("R.A. (degrees)")
plt.ylabel("Decl.(degrees)")
print >>sys.stderr,"# Time:%s" % despymisc.miscutils.elapsed_time(t0)
plt.xlim(275-360,275)
plt.ylim(-90,30)
plt.savefig("DES-alltiles-car.png")
plt.show()
plt.close()
