#!/usr/bin/env python

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs

from mojo.jobs.base_job import BaseJob
import os,sys

from despymisc.miscutils import elapsed_time
import despyfits
import time
import multiprocessing



class Job(BaseJob):

    """
    INPUTS:
    
    - selt.ctx.sci
    - selt.ctx.wgt
    - clobber_MED   : Defined local clobber (to self.ctx.clobber_MEF)
    - MP_MEF        : Run the process in MP
    
    OUTPUTS:
    - self.ctx.combMEF
    """


    class Input(IO):
        
        """ Create MEF for the coadded fits files"""

        ######################
        # Required inputs
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",
                          argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",
                              input_file=True,
                              argparse={ 'argtype': 'positional', })
        # Optional Arguments
        basename    = CUnicode("",help="Base Name for coadd fits files in the shape: COADD_BASENAME_$BAND.fits")
        clobber_MEF = Bool(False, help="Cloober the existing MEF fits")

        def _validate_conditional(self):
            # if in job standalone mode json
            if self.mojo_execution_mode == 'job as script' and self.basename == "":
                mess = 'If job is run standalone basename cannot be ""'
                raise IO_ValidationError(mess)

    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""
        
        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # Make sure we set up the output dir
        self.ctx = contextDefs.set_tile_directory(self.ctx)
        # 1. Set up names 
        # 1a. Get the BANDs information in the context if they are not present
        self.ctx = contextDefs.set_BANDS(self.ctx)
        # 1b. Get the output names for SWarp
        self.ctx = contextDefs.set_SWarp_output_names(self.ctx)
        # 1c. Get the outnames for the catalogs
        self.ctx = contextDefs.setCatNames(self.ctx)

    def run(self):

        # Prepare the context
        self.prewash()
        
        t0 = time.time()
        self.create_MEFs(self.ctx.clobber_MEF)
        print "# MEFs Creation Total time: %s" % elapsed_time(t0)
        return

    def create_MEFs(self,clobber,extnames=['SCI','WGT']):

        """ Create the MEF files using despyfits"""
        
        for BAND in self.ctx.dBANDS:

            # Inputs
            filenames = [self.ctx.comb_sci[BAND],
                         self.ctx.comb_wgt[BAND]]
            # output name of the MEF
            outname = self.ctx.comb_MEF[BAND]

            # Call it
            t0 = time.time()
            print "# Making MEF file for BAND:%s --> %s" % (BAND,outname)

            # FELIPE: We need to modify the return of despyfits.makeMEF
            # to WARN and not quit with error when the file exists
            despyfits.makeMEF(filenames=filenames,outname=outname,clobber=clobber,extnames=extnames)
            print "# Done in %s\n" % elapsed_time(t0)

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




