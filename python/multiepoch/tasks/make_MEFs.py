#!/usr/bin/env python

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

from mojo.jobs.base_job import BaseJob
import os,sys

from despymisc.miscutils import elapsed_time
import despyfits
import time
import multiprocessing

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh

class Job(BaseJob):

    """
    Create the MEF files based on the comb_sci and comb_wht files
    """

    class Input(IO):
        
        """ Create MEF for the coadded fits files"""

        ######################
        # Required inputs
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })
        # Optional Arguments
        #basename     = CUnicode("",help="Base Name for coadd fits files in the shape: COADD_BASENAME_$BAND.fits")
        tilename = Unicode(None, help="The Name of the Tile Name to query",argparse=False)
        tiledir  = CUnicode(None, help='The output directory for this tile.')
        clobber_MEF  = Bool(False, help="Cloober the existing MEF fits")
        cleanupSWarp = Bool(False, help="Clean-up SWarp files")
        MEF_execution_mode  = CUnicode("tofile",help="excution mode",
                                       argparse={'choices': ('tofile','dryrun','execute')})

        def _validate_conditional(self):
            pass

    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""

        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # Get the BANDs information in the context if they are not present
        self.ctx.update(contextDefs.get_BANDS(self.ctx.assoc, detname='det',logger=self.logger))

    def run(self):

        # Prepare the context
        self.prewash()
        
        t0 = time.time()
        self.create_MEFs(self.ctx.clobber_MEF)

        if self.input.cleanupSWarp:
            self.cleanup_SWarpFiles(execute=True)
            
        self.logger.info("# MEFs Creation Total time: %s" % elapsed_time(t0))
        return

    def cleanup_SWarpFiles(self,execute=False):


        # Sortcuts for less typing
        tiledir  = self.input.tiledir
        tilename = self.input.tilename

        for BAND in self.ctx.dBANDS:

            SWarpfiles = [fh.get_sci_fits_file(tiledir, tilename, BAND),
                          fh.get_wgt_fits_file(tiledir, tilename, BAND),
                          fh.get_SCI_fits_file(tiledir,tilename, BAND, type='tmp_sci'), # tmp_sci.fits
                          fh.get_WGT_fits_file(tiledir,tilename, BAND, type='tmp_wgt')] # wgt_tmp.fits
                          
            for sfile in SWarpfiles:
                self.logger.info("# Cleaning up %s" % sfile)
                if execute:
                    try:
                        os.remove(sfile)
                    except:
                        self.logger.info("# Warning: cannot remove %s" % sfile)

        return
            

    def create_MEFs(self,clobber,extnames=['SCI','WGT'],verb=False):

        """ Create the MEF files using despyfits"""
        
        # check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.get('MEF_execution_mode')

        # Sortcuts for less typing
        tiledir  = self.input.tiledir
        tilename = self.input.tilename

        for BAND in self.ctx.dBANDS:

            # Inputs, SCI/WGT combined images
            filenames = [fh.get_sci_fits_file(tiledir, tilename, BAND),
                         fh.get_wgt_fits_file(tiledir, tilename, BAND)]
            # output name of the MEF
            outname = fh.get_mef_file(tiledir, tilename, BAND)
            # Call it
            t0 = time.time()

            if execution_mode == 'execute':
                self.logger.info("# Making MEF file for BAND:%s --> %s" % (BAND,outname))
                despyfits.makeMEF(filenames=filenames,outname=outname,clobber=clobber,extnames=extnames,verb=verb)
            elif execution_mode == 'dryrun' or execution_mode == 'tofile':
                self.logger.info("# Making MEF file for BAND:%s --> %s" % (BAND,outname))
            else:
                raise ValueError('Execution mode %s not implemented.' % execution_mode)
            self.logger.info("# Done in %s" % elapsed_time(t0))

        return

    def __str__(self):
        return 'Create MEF file for a coadded TILE'

# # In case we want to run it as multi-process -- not really necessary because of IO penalty
# def runMakeMEF(args):
#     """
#     Simple call despyfits.makeMEF passing a tuple, instead of kwargs to be compatible with multiprocess
#     """
#     filenames,outname,clobber,extnames = args
#     despyfits.makeMEF(filenames=filenames,outname=outname,clobber=clobber,extnames=extnames)
#     return
 
if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)




