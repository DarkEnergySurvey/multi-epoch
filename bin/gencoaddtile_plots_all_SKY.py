#!/usr/bin/env python

""" A series of plots to make sure that limits are set properly"""

import os,sys
import numpy
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.patches     import Polygon
from matplotlib.collections import PatchCollection
import despymisc
from despyastro import tableio
import time

# For all sky plots
from pywcsgrid2.allsky_axes import make_allsky_axes_from_header, allsky_header

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
ras   = numpy.array([rall,ralr,raur,raul,rall])
decs  = numpy.array([decll,declr,decul,decur,decll])
uras  = numpy.array([ural ,urau, urau, ural,ural])
udecs = numpy.array([udecl,udecl,udecu,udecu,udecl])

# Re-center on zero
#ra   = numpy.where(ra   >= 275, ra  -360, ra)
#ras  = numpy.where(ras  >= 275, ras -360, ras)
#uras = numpy.where(uras >= 275, uras-360, uras)

proj_list = ["CYP", "CEA", "CAR", "MER", "SFL", "PAR", "MOL", ]
proj = "MOL"

fig = plt.figure(figsize=(20,10))
rect = 111

# Make 'fake' header 
coord, lon_center = "fk5", 0
header = allsky_header(coord=coord, proj=proj,
                       lon_center=lon_center, cdelt=0.01)
#print header
ax = make_allsky_axes_from_header(fig, rect, header, lon_center=lon_center)
ax.set_title("DES Tiles, %s projection" % proj, position=(0.5, 1.1))

## The centers of the tiles
for k in range(Ntiles):
    if (ra[k] >330 or ra[k] <= 15) and dec[k] > -10 and  dec[k] < 10:
        ax['fk5'].plot(ras[:,k],  decs[:,k],   lw=0.1, ls="-", color='blue')
        ax['fk5'].plot(uras[:,k], udecs[:,k],  lw=0.5, ls="--",color='red')


print >>sys.stderr,"# Time:%s" % despymisc.miscutils.elapsed_time(t0)
ax.grid(True)
plt.show()
