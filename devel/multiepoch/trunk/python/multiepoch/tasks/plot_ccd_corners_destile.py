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

from multiepoch import output_handler


D2R = math.pi/180. # degrees to radians shorthand

class Job(base_job.BaseJob):
    '''
    '''

    class Input(base_job.IO):

        tilename = Unicode(None, help='The name of the tile being plotted.')
        tiledir = Unicode(None, help='The tile output directory.')
        tileinfo = Dict(None, help="The tileinfo dictionary", argparse=False)

        # we initialize an empty recarray here to not get a comparison warning
        # this is suboptimal, default value should be None, but then we get a
        # numpy comparison warning ..
        CCDS = Instance(numpy.core.records.recarray, ([], 'int'), 
                help='The CCDS we want to plot.')

        plot_band = Unicode('', help='Plot single band only.')
        plot_outname = CUnicode('ccd_corners.pdf', help="Output file name for plot")


    def run(self):

        # Re-pack the tile corners
        tile_racs  = numpy.array([
            self.input.tileinfo['RAC1'], self.input.tileinfo['RAC2'],
            self.input.tileinfo['RAC3'], self.input.tileinfo['RAC4']
            ])
        tile_deccs = numpy.array([
            self.input.tileinfo['DECC1'], self.input.tileinfo['DECC2'],
            self.input.tileinfo['DECC3'], self.input.tileinfo['DECC4']
            ])
        
        if self.input.plot_band:
            figure = self.plot_CCDcornersDESTILEsingle(tile_racs, tile_deccs, self.input.plot_band)
        else:
            figure = self.plot_CCDcornersDESTILEsubplot(tile_racs, tile_deccs)

        dh = output_handler.get_tiledir_handler(self.input.tiledir, logger=self.logger)
        filepath = dh.place_file(output_handler.me_filename(
            base=self.input.tilename, band=self.input.plot_band,
            ftype='overlap', ext='pdf'))

        figure.savefig(filepath)
        self.logger.info("Wrote: %s" % filepath)
    

    def plot_CCDcornersDESTILEsubplot(self, tile_racs, tile_deccs, **kwargs):
        """ Plot the CCDs overlaping the DESTILENAME using subplots """

        # Get kwargs and set defaults...
        FIGNUMBER    = kwargs.get('fignumber', 4)
        FIGSIZE      = kwargs.get('figsize',  18)

        # Figure out the layout depending on the number of filters
        # found in the overlapping ares

        # Get the filters we found
        BANDS  = numpy.unique(self.input.CCDS['BAND'])
        NBANDS = len(BANDS)

        ncols = 3
        if NBANDS > 3:
            nrows = 2
        else:
            nrows = 1

        # Figure out the aspects ratios for the main and sub-plots
        plot_aspect    = float(nrows)/float(ncols)
        subplot_aspect = 1/math.cos(D2R*self.input.tileinfo['DEC'])

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

            self.logger.info("Found %s CCDimages for filter %s overlaping " % (NCCDs, BAND))
            for filename in filenames:

                ras, decs = self.repackCCDcorners(filename)
                P1 = Polygon( zip(ras,decs), closed=True,
                        hatch='', lw=0.2, alpha=0.1, Fill=True, color='k')
                P2 = Polygon( zip(ras,decs), closed=True, 
                        hatch='', lw=0.2, Fill=False, color='k') 
                ax.add_patch(P1)
                ax.add_patch(P2)

                # Keep track of the limits as Polygon as trouble
                x1 = min(x1, ras.min())
                x2 = max(x2, ras.max())
                y1 = min(y1, decs.min())
                y2 = max(y2, decs.max())

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
            ax.set_aspect(subplot_aspect)
            kplot = kplot + 1

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
