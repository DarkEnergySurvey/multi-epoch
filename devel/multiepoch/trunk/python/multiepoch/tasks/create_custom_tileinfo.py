#!/usr/bin/env python

"""
Creates a custom tile geometry file or context
"""
import json
#import numpy
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider
import multiepoch.utils as utils
import time
from despymisc.miscutils import elapsed_time

from multiepoch import tileinfo_utils
from despyastro import astrometry

class Job(BaseJob):

    """
    Creates a custom tileinfo dictionary and adds it to the context
    """
    
    class Input(IO):

        # This is the description that argparse will register.
        """
        Collect the tile geometry information using the DESDM Database
        """
        
        # Required inputs
        ra_center  = CFloat(None,help="RA  Center of the TILE (degrees)",
                            argparse={'required': True,})
        dec_center = CFloat(None,help="DEC Center of the TILE (degrees)",
                            argparse={'required': True,})
        tilename   = CUnicode("", help="The Name of the Tile Name to query")

        # Optional params         
        json_tileinfo_file = CUnicode("",help="Name of the output json file where we will store the tile information")
        tilename_fh = CUnicode("",  help="Alternative tilename handle for unique identification default=TILENAME")

        # Geometry params
        pixelscale = CFloat(0.263,help="Pixel Scale in arcsec/pixel")
        xsize = CFloat(10,help="X-size of output tile in arcmin")
        ysize = CFloat(10,help="Y-size of output tile in arcmin")
        prefix = CUnicode('DES',help="Prefix to use for TILENAME")
        
        # Logging -- might be factored out
        stdoutloglevel     = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                      argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel       = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                      argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        def _validate_conditional(self):

            if self.tilename == '':
                TILENAME = "{prefix}J{ra}{dec}"
                ra  = astrometry.dec2deg(self.ra_center/15.,sep="",plussign=False)
                dec = astrometry.dec2deg(self.dec_center,   sep="",plussign=True)
                self.tilename = TILENAME.format(prefix=self.prefix,ra=ra,dec=dec)

            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

            if self.json_tileinfo_file == '':
                self.json_tileinfo_file  = "%s.json" % self.tilename
                
    def run(self):

        # Make a dictionary/header for the all columns from COADDTILE table
        t0 = time.time()
        kw = {'xsize' : self.ctx.xsize,
              'ysize' : self.ctx.ysize,
              'pixelscale' : self.ctx.pixelscale,
              'ra_cent'    : self.ctx.ra_center,
              'dec_cent'   : self.ctx.dec_center,
              'json_file'  : self.ctx.json_tileinfo_file,
              'logger'     : self.logger,
              }
        self.ctx.tileinfo = tileinfo_utils.define_tileinfo(self.ctx.tilename,**kw)
        self.logger.info("Writing ouput to: %s" % self.input.json_tileinfo_file)
        self.logger.info("Custom geometry ready %s" % elapsed_time(t0))

    def __str__(self):
        return "Create custom tileinfo" 


if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
                          
