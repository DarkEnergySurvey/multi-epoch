"""
Plot the CCDs overlaping the DESTILENAME using subplots or single BAND
"""

import os
import math
import numpy

import matplotlib
matplotlib.use('Agg')

from matplotlib import pyplot as plt
from matplotlib.patches import Polygon
from mpl_toolkits.axes_grid1 import make_axes_locatable

from traitlets import Dict, Instance, CUnicode, Unicode, CInt, CBool, Bool
from mojo.jobs import base_job 

from multiepoch import file_handler as fh
from multiepoch import tileinfo_utils
import multiepoch.utils as utils
from despyastro import wcsutil

import fitsio
import json

D2R = math.pi/180. # degrees to radians shorthand

class Job(base_job.BaseJob):

    """
    Computes and draws the depth of DES tile

    """

    class Input(base_job.IO):

        # Positional arguments --- must be provided, either by the context or the command-line
        tilename = Unicode(None, help='The name of the tile being plotted.')
        tiledir  = Unicode('', help='The tile output directory.')
        tileinfo = Dict(None, help="The tileinfo dictionary", argparse=False)

        # we initialize an empty recarray here to not get a comparison warning
        # this is suboptimal, default value should be None, but then we get a
        # numpy comparison warning ..
        CCDS = Instance(numpy.core.records.recarray, ([], 'int'), 
                help='The CCDS we want to plot.')

        binning        = CInt(100, help='Binning fraction')
        plot_ccds  = Bool(False, help="Plot CCDS")
        plot_depth = Bool(False, help="Plot depth")
        outname_ccds_plot  = Unicode('', help="Output file name for ccds plot")
        outname_depth_plot = Unicode('', help="Output file name for depth plot")
        outname_fraction = Unicode('', help="Output file name for depth json file with fraction")


    def run(self):

        # Get self.ctx.tile_header_depth
        self.build_header_depth(binning=self.ctx.binning)
        
        # Get the WCS for the tile -- we'll use it everywhere
        self.ctx.wcs = wcsutil.WCS(self.ctx.tile_header_depth)

        # Get the filters we found
        self.ctx.BANDS  = numpy.unique(self.ctx.CCDS['BAND'])
        self.ctx.NBANDS = len(self.ctx.BANDS)

        # Get the tile xy coordinates
        tile_xs,tile_ys = self.get_tile_xy_corners()

        # Estimate depth and fraction @ depth  and store in dictionary
        tile_depth = {}
        fraction = {}
        for BAND in self.ctx.BANDS:
            tile_depth[BAND], fraction[BAND] = self.get_tile_depth(tile_xs, tile_ys, BAND)

        # Write the fractions to a json file
        if self.ctx.outname_fraction == '':
            self.ctx.outname_fraction = fh.get_plot_depth_fraction(self.ctx.tiledir, self.ctx.tilename)
        with open(self.ctx.outname_fraction, 'w') as fp:
            fp.write(json.dumps(fraction,sort_keys=False,indent=4))
        self.logger.info("Wrote depth fractions to: %s" % self.ctx.outname_fraction)
        
        # Plot the depth per band in subplots
        if self.ctx.plot_depth:
            self.logger.info("Plotting Depth")
            figure_depth = self.plot_depth_subplot(tile_depth)
            if self.ctx.outname_depth_plot == '':
                self.ctx.outname_depth_plot = fh.get_plot_depth_file(self.ctx.tiledir, self.ctx.tilename)
            utils.check_filepath_exist(self.ctx.outname_depth_plot,logger=self.logger.debug)
            figure_depth.savefig(self.ctx.outname_depth_plot)
            plt.close()
            self.logger.info("Wrote: %s" % self.ctx.outname_depth_plot)

        # Plotting overlap in xy
        if self.ctx.plot_ccds:
            self.logger.info("Plotting CCDS in image coordinates")
            figure_ccds = self.plot_CCDcornersDESTILEsubplot_xy(tile_xs, tile_ys)
            if self.ctx.outname_ccds_plot  == '':
                self.ctx.outname_ccds_plot  = fh.get_ccd_plot_file_image(self.ctx.tiledir, self.ctx.tilename,self.ctx.search_type)
            utils.check_filepath_exist(self.ctx.outname_ccds_plot ,logger=self.logger.debug)
            figure_ccds.savefig(self.ctx.outname_ccds_plot)
            plt.close()
            self.logger.info("Wrote: %s" % self.ctx.outname_ccds_plot)


    def plot_depth_subplot(self, tile_depth, **kwargs):
        """ Plot the CCDs overlaping the DESTILENAME using subplots on image coordinates"""

        # Get kwargs and set defaults...
        FIGNUMBER    = kwargs.get('fignumber', 1)
        FIGSIZE      = kwargs.get('figsize',  18)

        # Figure out the layout depending on the number of filters
        # found in the overlapping ares
        BANDS  = self.ctx.BANDS
        NBANDS = self.ctx.NBANDS
        ncols = 3
        if NBANDS > 3:
            nrows = 2
        else:
            nrows = 1

        # Figure out the aspects ratios for the main and sub-plots
        plot_aspect    = float(nrows)/float(ncols)
        subplot_aspect = 1

        # The main figure
        fig = plt.figure(FIGNUMBER,figsize=(FIGSIZE, FIGSIZE*plot_aspect))

        kplot = 1
        for BAND in BANDS:

            plt.subplot(nrows,ncols,kplot)
            ax = plt.gca()
            plt.imshow(tile_depth[BAND], origin='lower', cmap="gray")
            plt.title("%s - %s band" % (self.ctx.tilename, BAND))
            plt.xlabel("X (pixels)")
            plt.ylabel("Y (pixels)")

            # Make color-bar same height
            divider = make_axes_locatable(ax)
            cax = divider.append_axes("right", size="5%", pad=0.05)
            cbar = plt.colorbar(cax=cax)
            # Fixes lines in the colorbar
            cbar.solids.set_rasterized(True)
            cbar.solids.set_edgecolor("face")
            ax.set_aspect(subplot_aspect)
            kplot = kplot + 1

        fig.set_tight_layout(True) # This avoids backend warnings.
        return fig


    def get_tile_xy_corners(self):

        # Re-pack the tile corners and get the x,y edges of the tiles in the new wcs reference frame
        tile_racs  = numpy.array([self.ctx.tileinfo['RAC1'], self.ctx.tileinfo['RAC2'],
                                  self.ctx.tileinfo['RAC3'], self.ctx.tileinfo['RAC4']])
        tile_deccs = numpy.array([self.ctx.tileinfo['DECC1'], self.ctx.tileinfo['DECC2'],
                                  self.ctx.tileinfo['DECC3'], self.ctx.tileinfo['DECC4']])
        tile_xs,tile_ys = self.ctx.wcs.sky2image(tile_racs,tile_deccs)
        tile_xs = tile_xs.astype(int)
        tile_ys = tile_ys.astype(int)
        return tile_xs,tile_ys

    def build_header_depth(self,binning=1):

        """ Construct the header of the tile """

        # Binning
        xsize_arcsec = self.ctx.tileinfo['PIXELSCALE']*self.ctx.tileinfo['NAXIS1']
        NAXIS1 = int(self.ctx.tileinfo['NAXIS1']/binning)
        NAXIS2 = int(self.ctx.tileinfo['NAXIS2']/binning)
        pixelscale = xsize_arcsec/NAXIS1
        kw = {
            'pixelscale' : pixelscale,
            'NAXIS1'     : NAXIS1,
            'NAXIS2'     : NAXIS2,
            'ra_cent'    : self.ctx.tileinfo['RA_CENT'],
            'dec_cent'   : self.ctx.tileinfo['DEC_CENT'],
            }
        self.ctx.tile_header_depth = tileinfo_utils.create_header(**kw)


    def get_tile_depth(self, tile_xs, tile_ys, BAND, **kwargs):
        
        """ Get the tile depth """

        depths = kwargs.get('depths',[1,2,3,4,5,6])
        
        NAXIS1 = self.ctx.tile_header_depth['NAXIS1']
        NAXIS2 = self.ctx.tile_header_depth['NAXIS2']
        tile_depth = numpy.zeros((NAXIS1,NAXIS2),dtype=numpy.int)

        idx = numpy.where(self.ctx.CCDS['BAND'] == BAND)[0]
        filenames = self.ctx.CCDS['FILENAME'][idx]
        NCCDs = len(filenames)
        for filename in filenames:

            # Get the CCD corners in the image coordinate system
            ras, decs = self.repackCCDcorners(filename)
            xs, ys = self.ctx.wcs.sky2image(ras,decs)
            # Get the overlapping region
            xs = xs - 1
            ys = ys - 1
            X1 = int(max( xs.min(), tile_xs.min()))
            X2 = int(min( xs.max(), tile_xs.max()))
            Y1 = int(max( ys.min(), tile_ys.min()))
            Y2 = int(min( ys.max(), tile_ys.max()))
            # We might want to change this to t_eff * exposure_time/survey_exposure_time
            tile_depth[Y1:Y2,X1:X2] += 1
            ####################################
            # OPTIONAL WRITE
            # Write a fits file
            #outname  = '%s_%s_depth.fits' % (self.ctx.tilename,BAND)
            #ofits = fitsio.FITS(outname,'rw',clobber=True)
            #ofits.write(tile_depth[BAND],extname='SCI',header=self.ctx.tile_header_depth)
            #print "Wrote: %s" %  outname
            ###################################

        self.logger.info("Median depth for %s:  %s" % (BAND,numpy.median(tile_depth)))
        fraction = fraction_greater_than(tile_depth,depths=depths)
        return tile_depth, fraction


    def plot_CCDcornersDESTILEsubplot_xy(self, tile_xs, tile_ys, **kwargs):
        """ Plot the CCDs overlaping the DESTILENAME using subplots on image coordinates"""

        # Get kwargs and set defaults...
        FIGNUMBER    = kwargs.get('fignumber', 4)
        FIGSIZE      = kwargs.get('figsize',  18)

        # Figure out the layout depending on the number of filters
        # found in the overlapping ares
        BANDS  = self.ctx.BANDS
        NBANDS = self.ctx.NBANDS
        ncols = 3
        if NBANDS > 3:
            nrows = 2
        else:
            nrows = 1

        # Figure out the aspects ratios for the main and sub-plots
        plot_aspect    = float(nrows)/float(ncols)
        subplot_aspect = 1

        # The main figure
        fig = plt.figure(FIGNUMBER,figsize=(FIGSIZE, FIGSIZE*plot_aspect))
        
        # Initialize limits for all plots
        x1 =  1e10
        x2 = -1e10
        y1 = x1
        y2 = x2

        kplot = 1
        for BAND in BANDS:

            plt.subplot(nrows,ncols,kplot)
            ax = plt.gca()
            idx = numpy.where(self.input.CCDS['BAND'] == BAND)[0]
            filenames = self.input.CCDS['FILENAME'][idx]
            NCCDs = len(filenames)

            self.logger.info("Plotting %s CCDimages for filter %s overlaping " % (NCCDs, BAND))
            for filename in filenames:

                ras, decs = self.repackCCDcorners(filename)
                xs, ys = self.ctx.wcs.sky2image(ras,decs)
                
                P1 = Polygon(zip(xs,ys), closed=True,hatch='',lw=0.2,alpha=0.1,Fill=True,color='k')
                P2 = Polygon(zip(xs,ys), closed=True,hatch='',lw=0.2,Fill=False, color='k') 
                ax.add_patch(P1)
                ax.add_patch(P2)

                # Keep track of the limits as Polygon as trouble
                x1 = min(x1, xs.min())
                x2 = max(x2, xs.max())
                y1 = min(y1, ys.min())
                y2 = max(y2, ys.max())

            # Draw the TILE footprint at the end
            P = Polygon(zip(tile_xs, tile_ys),
                        closed=True, Fill=False, hatch='',lw=1.0, color='r')
            ax.add_patch(P)
            
            # Fix range
            plt.xlim(x1,x2)
            plt.ylim(y1,y2)
            plt.title("%s - %s band (%s images)" % (self.ctx.tilename, BAND, NCCDs))
            plt.xlabel("X (pixels)")
            plt.ylabel("Y (pixels)")
            ax.set_aspect(subplot_aspect)
            kplot = kplot + 1

        fig.set_tight_layout(True) # This avoids backend warnings.
        return fig

    def repackCCDcorners(self, filename):
        """
        Repacks the CCD corners for plotting, from the existing
        dictionary of CCD images that fall inside the tile
        """
        ccds = self.input.CCDS  # short-cut
        k = numpy.where(ccds['FILENAME'] == filename)[0][0]
        ras   = numpy.array([ccds['RAC1'][k], ccds['RAC2'][k], ccds['RAC3'][k], ccds['RAC4'][k]])
        decs  = numpy.array([ccds['DECC1'][k],ccds['DECC2'][k],ccds['DECC3'][k],ccds['DECC4'][k]])
        return ras, decs


    def __str__(self):
        return 'plot the ccd corners for a DES tile'

def fraction_greater_than(image,depths=[1,2,3,4,5]):

    Npixels = image.shape[0]*image.shape[1]
    fraction = {}
    for N in depths:
        Ngreater = len(numpy.where(image>=N)[0])
        fraction[N] = float(Ngreater)/float(Npixels)
    return fraction
    
