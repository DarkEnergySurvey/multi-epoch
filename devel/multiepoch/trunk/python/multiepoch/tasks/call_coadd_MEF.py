#!/usr/bin/env python

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

from mojo.jobs.base_job import BaseJob
import os,sys

from despymisc.miscutils import elapsed_time
import time
import pandas as pd

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh
from multiepoch import coadd_MEF 

DETNAME = 'det'
COADD_MEF_EXE = 'coadd_MEF'
BKLINE = "\\\n"

class Job(BaseJob):

    """
    Create the MEF files based on the comb_sci and comb_wht files
    """

    class Input(IO):
        
        """
        Create MEF for the coadded fits files
        """

        ######################
        # Required inputs
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })
        #####################
        # Optional Arguments
        tilename     = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh  = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir      = Unicode(None, help="The output directory for this tile")
        clobber_MEF  = Bool(False, help="Cloober the existing MEF fits")
        cleanupSWarp = Bool(False, help="Clean-up SWarp files")
        execution_mode_MEF  = CUnicode("tofile",help="excution mode",
                                       argparse={'choices': ('tofile','dryrun','execute')})
        doBANDS       = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})
        detname       = CUnicode(DETNAME,help="File label for detection image, default=%s." % DETNAME)

        # Weight for mask
        weight_for_mask  = Bool(False, help="Create coadded weight for mask creation")

        # zipper params
        xblock    = CInt(1, help="Block size of zipper in x-direction")
        add_noise = Bool(False,help="Add Poisson Noise to the zipper")
        
        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        # Function to read ASCII/panda framework file (instead of json)
        # Comment if you want to use json files
        def _read_assoc_file(self):
            mydict = {}
            df = pd.read_csv(self.assoc_file,sep=' ')
            mydict['assoc'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

        # To also accept comma-separeted input lists
        def _argparse_postproc_doBANDS(self, v):
            return utils.parse_comma_separated_list(v)

    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""

        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # Get the BANDs information in the context if they are not present
        if self.ctx.get('gotBANDS'):
            self.logger.info("BANDs already defined in context -- skipping")
        else:
            self.ctx.update(contextDefs.get_BANDS(self.ctx.assoc, detname=self.ctx.detname,logger=self.logger,doBANDS=self.ctx.doBANDS))

        # Check info OK
        self.logger.info("BANDS:   %s" % self.ctx.BANDS)
        self.logger.info("doBANDS: %s" % self.ctx.doBANDS)
        self.logger.info("dBANDS:  %s" % self.ctx.dBANDS)

    def run(self):

        t0 = time.time()

        # Prepare the context
        self.prewash()

        # check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.execution_mode_MEF
        
        # Build the args dictionary for all BANDS
        args = self.assemble_args()

        if execution_mode == 'execute':
            for BAND in self.ctx.dBANDS:
                self.logger.info("Making MEF file for BAND:%s --> %s" % (BAND,args[BAND]['outname']))
                coadd_MEF.merge(**args[BAND])

        elif execution_mode == 'dryrun':
            for BAND in self.ctx.dBANDS:
                cmd = self.get_merge_cmd(args[BAND])
                self.logger.info(" ".join(cmd))
                
        elif execution_mode == 'tofile':
            bkline  = self.ctx.get('breakline',BKLINE)
            cmdfile = fh.get_mef_cmd_file(self.input.tiledir, self.input.tilename_fh)
            self.logger.info("Will write coadd_MEF call to: %s" % cmdfile)
            with open(cmdfile, "w") as fid:
                for BAND in self.ctx.dBANDS:
                    cmd = self.get_merge_cmd(args[BAND])
                    fid.write(bkline.join(cmd)+'\n')
                    fid.write('\n\n')
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)

        if self.input.cleanupSWarp and execution_mode == 'execute':
            self.cleanup_SWarpFiles(execute=True)
            
        self.logger.info("MEFs Creation Total time: %s" % elapsed_time(t0))
        return

    def get_merge_cmd(self,args):

        cmd = []
        cmd.append(COADD_MEF_EXE)
        for key in args.keys():
            if key == 'logger':
                continue
            if args[key] == True:
                cmd.append("--%s" % key)
            elif args[key]:
                cmd.append("--%s %s" % (key,args[key]))
        return cmd


    def assemble_args(self):
        """ Build the args dictionary to be pass as **kwrgs or comand-line """
        args = {}
        tiledir     = self.input.tiledir
        tilename_fh = self.input.tilename_fh
        for BAND in self.ctx.dBANDS:
            outname = fh.get_mef_file(tiledir, tilename_fh, BAND)
            args[BAND] = {'sci_file': fh.get_sci_fits_file(tiledir, tilename_fh, BAND),                          
                          'wgt_file': fh.get_wgt_fits_file(tiledir, tilename_fh, BAND),
                          'outname' : outname,
                          'logger'  : self.logger,
                          'clobber' : self.ctx.clobber_MEF,
                          'xblock'  : self.ctx.xblock,
                          'add_noise' : self.ctx.add_noise,
                          }
            if self.input.weight_for_mask:
                args[BAND]['msk_file'] = fh.get_msk_fits_file(tiledir, tilename_fh, BAND)

            if BAND == 'det':
                args[BAND]['band'] = BAND
            
        return args

    def cleanup_SWarpFiles(self,execute=False):

        # Sortcuts for less typing
        tiledir     = self.input.tiledir
        tilename_fh = self.input.tilename_fh

        for BAND in self.ctx.dBANDS:
            SWarpfiles = [fh.get_sci_fits_file(tiledir,tilename_fh, BAND),
                          fh.get_wgt_fits_file(tiledir,tilename_fh, BAND),
                          fh.get_gen_fits_file(tiledir,tilename_fh, BAND, type='tmp_sci') # tmp_sci.fits
                          ]
            if self.input.weight_for_mask:
                SWarpfiles.append(fh.get_msk_fits_file(tiledir,tilename_fh, BAND))
            
            for sfile in SWarpfiles:
                self.logger.info("Cleaning up %s" % sfile)
                if execute:
                    try:
                        os.remove(sfile)
                    except:
                        self.logger.info("Warning: cannot remove %s" % sfile)

        return

    def __str__(self):
        return 'Create MEF file for a coadded TILE fron SCI/MSK/WGT image planes using coadd_MEF'
 
if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)




