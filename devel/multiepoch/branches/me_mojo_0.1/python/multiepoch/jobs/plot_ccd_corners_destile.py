"""
Plot the CCDs overlaping the DESTILENAME using subplots or single BAND
"""

from mojo.jobs.base_job import BaseJob

from matplotlib import pyplot as plt
from matplotlib.patches import Polygon
import math
import numpy
import os

D2R = math.pi/180. # degrees to radians shorthand

class Job(BaseJob):

    def __call__(self):

        # Get all of the kwargs
        kwargs = self.ctx.get_kwargs_dict()

        # Decide if we want to plot MULTI panel (subplot) or SINGLE planel
        BAND = kwargs.pop('band', False)
        
        if BAND:
            self.plot_CCDcornersDESTILEsingle(BAND,**kwargs)
        else:
            self.plot_CCDcornersDESTILEsubplot(**self.ctx.get_kwargs_dict())
        
        return
    
    def plot_CCDcornersDESTILEsubplot(self,**kwargs):
        
        """ Plot the CCDs overlaping the DESTILENAME using subplots"""

        # Get kwargs and set defaults...
        FIGNUMBER = kwargs.get('fignumber', 4)
        FIGSIZE   = kwargs.get('figsize',  18)
        SHOW      = kwargs.get('show', False)

        # Figure out the layout depending on the number of filters
        # found in the overlapping ares
        ncols = 3
        if self.ctx.NBANDS > 3:
            nrows = 2
        else:
            nrows = 1

        # Figure out the aspects ratios for the main and sub-plots
        plot_aspect    = float(nrows)/float(ncols)
        subplot_aspect = 1/math.cos(D2R*self.ctx.tileinfo['DEC'])

        # The main figure
        fig = plt.figure(FIGNUMBER,figsize=(FIGSIZE, FIGSIZE*plot_aspect))
        
        # Initialize limits for all plots
        x1 =  1e10
        x2 = -1e10
        y1 = x1
        y2 = x2
        
        kplot = 1
        for BAND in self.ctx.BANDS:

            plt.subplot(nrows,ncols,kplot)
            ax = plt.gca()
            idx = numpy.where(self.ctx.CCDS['BAND'] == BAND)[0]
            filenames = self.ctx.CCDS['FILENAME'][idx]
            NCCDs = len(filenames)

            print "# Found %s CCDimages for filter %s overlaping " % (NCCDs, BAND)
            for filename in filenames:

                ras, decs = self.repackCCDcorners(filename)
                P1 = Polygon( zip(ras,decs), closed=True, hatch='',lw=0.2,alpha=0.1,Fill=True,color='k')
                P2 = Polygon( zip(ras,decs), closed=True, hatch='',lw=0.2,Fill=False,color='k') 
                ax.add_patch(P1)
                ax.add_patch(P2)

                # Keep track of the limits as Polygon as trouble
                x1 = min(x1, ras.min())
                x2 = max(x2, ras.max())
                y1 = min(y1, decs.min())
                y2 = max(y2, decs.max())

            # Draw the TILE footprint at the end
            P = Polygon(
                    zip(self.ctx.tileinfo['TILE_RACS'],
                        self.ctx.tileinfo['TILE_DECCS']),
                    closed=True, Fill=False, hatch='',lw=1.0, color='r'
                    )

            ax.add_patch(P)
        
            # Fix range
            plt.xlim(x1,x2)
            plt.ylim(y1,y2)
            plt.title("%s - %s band (%s images)" % (self.ctx.tilename, BAND, NCCDs))
            plt.xlabel("RA (degrees)")
            plt.ylabel("Decl. (degrees)")
            ax.set_aspect(subplot_aspect)
            kplot = kplot + 1

        fig.set_tight_layout(True) # This avoids backend warnings.
        figname = os.path.join(self.ctx.tiledir,"%s_overlap.pdf" % self.ctx.tilename)
        fig.savefig(figname)
        if SHOW: plt.show()
        print "# Wrote: %s" % figname

    def plot_CCDcornersDESTILEsingle(self,BAND,**kwargs):

        """ Plot the CCD corners of overlapping images for the tilename"""

        # Get kwargs and set defaults...
        FIGNUMBER = kwargs.get('fignumber', 1)
        FIGSIZE   = kwargs.get('figsize',  10)
        SHOW      = kwargs.get('show', False)

        aspect = math.cos(D2R*self.ctx.tileinfo['DEC'])
        fig = plt.figure(FIGNUMBER,figsize=(FIGSIZE,FIGSIZE*aspect))
        ax = plt.gca()

        idx = numpy.where(self.ctx.CCDS['BAND'] == BAND)[0]
        filenames = self.ctx.CCDS['FILENAME'][idx]
        NCCDs = len(filenames)

        # Initialize limits
        x1 =  1e10
        x2 = -1e10
        y1 = x1
        y2 = x2

        print "# Found %s CCDimages for filter %s overlaping " % (NCCDs, BAND)
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
        P = Polygon(
                zip(self.ctx.tileinfo['TILE_RACS'],
                    self.ctx.tileinfo['TILE_DECCS']),
                closed=True, Fill=False, hatch='',lw=1.0, color='r'
                )
        ax.add_patch(P)
        
        # Fix range
        plt.xlim(x1,x2)
        plt.ylim(y1,y2)
        plt.title("%s - %s band (%s images)" % (self.ctx.tilename,BAND,NCCDs))
        plt.xlabel("RA (degrees)")
        plt.ylabel("Decl. (degrees)")

        fig.set_tight_layout(True) # This avoids backend warnings.
        figname = os.path.join(self.ctx.tiledir,"%s_overlap_%s.pdf" % (self.ctx.tilename,BAND))
        fig.savefig(figname)
        if SHOW: plt.show()
        print "# Wrote: %s" % figname

        return



    def __str__(self):
        return 'plot ccd corners desfile'


    def repackCCDcorners(self, filename):
        """
        Repacks the CCD corners for plotting, from the existing
        dictionary of CCD images that fall inside the tile
        """
        
        ccds = self.ctx.CCDS  # short-cut
        k = numpy.where(ccds['FILENAME'] == filename)[0][0]
        ras   = numpy.array([ccds['RAC1'][k], ccds['RAC2'][k], ccds['RAC3'][k], ccds['RAC4'][k]])
        decs  = numpy.array([ccds['DECC1'][k],ccds['DECC2'][k],ccds['DECC3'][k],ccds['DECC4'][k]])
        return ras, decs
