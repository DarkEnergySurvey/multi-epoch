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

from traitlets import Dict, Instance, CUnicode, Unicode
from mojo.jobs import base_job 

from multiepoch import file_handler as fh
import multiepoch.utils as utils

from multiepoch import tileinfo_utils
from despyastro import wcsutil

import fitsio

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

        plot_band    = CUnicode('', help='Plot single band only.')
        plot_outname = Unicode(None, help="Output file name for plot")

        def _validate_conditional(self):
            if self.tiledir == '' and self.plot_outname is None:
                mess = 'Need to define either tilename of plot_outname'
                raise IO_ValidationError(mess)

    def run(self):

        #print self.ctx.CCDS.dtype.names

        # Get self.ctx.tile_header_depth
        self.build_header_depth(binning=10)
        
        # Get the WCS for the tile -- we'll use it everywhere
        self.ctx.wcs = wcsutil.WCS(self.ctx.tile_header_depth)

        # Get the filters we found
        self.ctx.BANDS  = numpy.unique(self.ctx.CCDS['BAND'])
        #self.ctx.BANDS  = ['r']
        self.ctx.NBANDS = len(self.ctx.BANDS)

        # Re-pack the tile corners
        tile_racs  = numpy.array([self.ctx.tileinfo['RAC1'], self.ctx.tileinfo['RAC2'],
                                  self.ctx.tileinfo['RAC3'], self.ctx.tileinfo['RAC4']])
        tile_deccs = numpy.array([self.ctx.tileinfo['DECC1'], self.ctx.tileinfo['DECC2'],
                                  self.ctx.tileinfo['DECC3'], self.ctx.tileinfo['DECC4']])
        
        # Get the corners in images coordinates -- redudant as these are 1,NAXIS1 1,NAXIS2
        tile_xs,tile_ys = self.ctx.wcs.sky2image(tile_racs,tile_deccs)
        tile_xs = tile_xs.astype(int)
        tile_ys = tile_ys.astype(int)
        #print tile_xs
        #print tile_ys
        
        #tile_xs = numpy.array([0,self.ctx.tileinfo['NAXIS1']-1,self.ctx.tileinfo['NAXIS1']-1,0])
        #tile_ys = numpy.array([0,0,self.ctx.tileinfo['NAXIS2']-1,self.ctx.tileinfo['NAXIS2']-1])
        
        figure = self.plot_CCDcornersDESTILEsubplot_xy(tile_xs, tile_ys)

        # FileName of the plot
        #if self.input.plot_outname:
        #    filepath = self.input.plot_outname
        #else:
        #    # Use file-handler to set the name
        #    filepath = fh.get_ccd_plot_file(self.ctx.tiledir, self.ctx.tilename,self.ctx.search_type)
        filepath = '%s_xy_coverage.pdf' % self.ctx.tilename

        # Make sure that the filepath exists
        utils.check_filepath_exist(filepath,logger=self.logger.debug)
                             
        figure.savefig(filepath)
        self.logger.info("Wrote: %s" % filepath)


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

    def plot_CCDcornersDESTILEsubplot_xy(self, tile_xs, tile_ys, **kwargs):
        """ Plot the CCDs overlaping the DESTILENAME using subplots on image coordinates"""

        # Get kwargs and set defaults...
        FIGNUMBER    = kwargs.get('fignumber', 4)
        FIGSIZE      = kwargs.get('figsize',  18)

        # Figure out the layout depending on the number of filters
        # found in the overlapping ares

        # Get the filters we found
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

        NAXIS1 = self.ctx.tile_header_depth['NAXIS1']
        NAXIS2 = self.ctx.tile_header_depth['NAXIS2']
        tile_depth = {}
        
        kplot = 1
        for BAND in BANDS:

            tile_depth[BAND] = numpy.zeros((NAXIS1,NAXIS2),dtype=numpy.int)

            plt.subplot(nrows,ncols,kplot)
            ax = plt.gca()
            idx = numpy.where(self.input.CCDS['BAND'] == BAND)[0]
            filenames = self.input.CCDS['FILENAME'][idx]
            NCCDs = len(filenames)

            self.logger.info("Found %s CCDimages for filter %s overlaping " % (NCCDs, BAND))
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

                # Get the overlapping region
                xs = xs - 1
                ys = ys - 1
                X1 = int(max( xs.min(), tile_xs.min()))
                X2 = int(min( xs.max(), tile_xs.max()))
                Y1 = int(max( ys.min(), tile_ys.min()))
                Y2 = int(min( ys.max(), tile_ys.max()))
                #if filename == 'D00155284_r_c29_r2323p01_immasked.fits':
                tile_depth[BAND][Y1:Y2,X1:X2] += 1

            ####################################
            # OPTIONAL
            # Write a fits file
            outname  = '%s_%s_depth.fits' % (self.ctx.tilename,BAND)
            ofits = fitsio.FITS(outname,'rw',clobber=True)
            ofits.write(tile_depth[BAND],extname='SCI',header=self.ctx.tile_header_depth)
            print "Wrote: %s" %  outname
            ###################################
            print "%s - %s" % (BAND,numpy.median(tile_depth[BAND]))

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

        self.ctx.BANDS = BANDS
        fig.set_tight_layout(True) # This avoids backend warnings.
        return fig

    
    def plot_CCDcornersDESTILEsingle(self, BAND, tile_racs, tile_deccs, **kwargs):
        """ Plot the CCD corners of overlapping images for the tilename """

        # Get kwargs and set defaults...
        FIGNUMBER = kwargs.get('fignumber', 1)
        FIGSIZE   = kwargs.get('figsize',  10)

        aspect = math.cos(D2R*self.input.tileinfo['DEC'])
        fig = plt.figure(FIGNUMBER,figsize=(FIGSIZE,FIGSIZE*aspect))
        ax = plt.gca()

        idx = numpy.where(self.input.CCDS['BAND'] == BAND)[0]
        filenames = self.input.CCDS['FILENAME'][idx]
        NCCDs = len(filenames)

        # Initialize limits
        x1 =  1e10
        x2 = -1e10
        y1 = x1
        y2 = x2

        self.logger.info("Found %s CCDimages for filter %s overlaping " % (NCCDs, BAND))
        for filename in filenames:
            ras, decs = self.repackCCDcorners(filename)
            P1 = Polygon( zip(ras,decs), closed=True, hatch='',lw=0.2,alpha=0.1,Fill=True,color='k')
            P2 = Polygon( zip(ras,decs), closed=True, hatch='',lw=0.2,Fill=False,color='k') 
            ax.add_patch(P1)
            ax.add_patch(P2)

            # Keep track of the limits as Polygon as trouble
            x1 = min(x1,ras.min())
            x2 = max(x2,ras.max())
            y1 = min(y1,decs.min())
            y2 = max(y2,decs.max())

        # Draw the TILE footprint at the end
        P = Polygon(zip(tile_racs, tile_deccs),
                    closed=True, Fill=False, hatch='',lw=1.0, color='r')
        ax.add_patch(P)
        
        # Fix range
        plt.xlim(x1,x2)
        plt.ylim(y1,y2)
        plt.title("%s - %s band (%s images)" % (self.input.tilename, BAND, NCCDs))
        plt.xlabel("RA (degrees)")
        plt.ylabel("Decl. (degrees)")

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
