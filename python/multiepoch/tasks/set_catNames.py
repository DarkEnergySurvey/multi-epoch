from mojo.jobs.base_job import BaseJob
import os

class Job(BaseJob):


    """
    Setup the catalogs names for SExtractor and psfex

    Inputs:
    - None

    Outputs:
    - self.ctx.psfcat = {}
    - self.ctx.psf    = {}
    - self.ctx.psfexxml     = {}
    - self.ctx.checkimage  = {}
    - self.ctx.cat = {}
    """

    def __call__(self):

        self.setCatNames(**self.ctx.get_kwargs_dict())
        return

    def setCatNames(self,**kwargs):

        """ Set the names for input/ouput for psfex/Sextractor calls"""

        print "# Setting names for SExPSF/psfex and SExDual"

        # SExPSF
        self.ctx.psfcat = {}
        self.ctx.psf    = {}
        # PsfCall
        self.ctx.psfexxml     = {}
        # SExDual
        self.ctx.checkimage  = {}
        self.ctx.cat = {}
        
        for BAND in self.ctx.dBANDS:
            # SExPSF
            self.ctx.psf[BAND]       = os.path.join(self.ctx.tiledir,"%s_%s_psfcat.psf"  %  (self.ctx.tilename, BAND))
            self.ctx.psfcat[BAND]    = os.path.join(self.ctx.tiledir,"%s_%s_psfcat.fits" %  (self.ctx.tilename, BAND))
            # psfex
            self.ctx.psfexxml[BAND]  = os.path.join(self.ctx.tiledir,"%s_%s_psfex.xml"   %  (self.ctx.tilename, BAND))
            # SExDual
            self.ctx.cat[BAND]       = os.path.join(self.ctx.tiledir,"%s_%s_cat.fits"    %  (self.ctx.tilename, BAND))
            self.ctx.checkimage[BAND]= os.path.join(self.ctx.tiledir,"%s_%s_seg.fits"    %  (self.ctx.tilename, BAND))
        print "# Done"
        return

    def __str__(self):
        return 'Set the names for input/ouput for psfex/Sextractor call'




