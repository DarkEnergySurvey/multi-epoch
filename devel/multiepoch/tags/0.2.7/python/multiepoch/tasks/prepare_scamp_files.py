#!/usr/bin/env python

from despyastro import tableio
import os
import sys
import numpy
import time
import pandas as pd
import multiprocessing as mp

from despymisc.miscutils import elapsed_time
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh
import despyfitsutils.fitsutils as fitsutils

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.utils import log as mojo_log

# JOB INTERNAL CONFIGURATION
DETNAME = 'det'
BKLINE = "\\\n"
class Job(BaseJob):

    class Input(IO):

        """ Scamp preparation call for the multiepoch pipeline"""

        ######################
        # Required inputs
        catlist      = Dict(None,help="The Dictionary containing input CCD-level catalog list ",argparse=False)
        cats_file    = CUnicode('',help="Name of the output ASCII catalog list storing the information for scamp", input_file=True,
                                argparse={ 'argtype': 'positional', })
        ######################
        # Optional arguments
        execution_mode_scamp_prep  = CUnicode("tofile",help="Scamp prepare files excution mode",
                                              argparse={'choices': ('tofile','dryrun','execute')})
        MP_cats      = CInt(1, help = ("Run using multi-process, 0=automatic, 1=single-process [default]"))

        tilename    = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir     = Unicode(None, help="The output directory for this tile")

        local_archive = CUnicode("", help="The local filepath where the input fits files (will) live")
        doBANDS       = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})
        detname       = CUnicode(DETNAME,help="File label for detection image, default=%s." % DETNAME)

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        def _read_cats_file(self):
            mydict = {}
            df = pd.read_csv(self.cats_file,sep=' ')
            mydict['catlist'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):

            # Get logger
            logger = mojo_log.get_logger({})
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename
            if self.stdoutloglevel == 'DEBUG':
                os.environ['FITSUTILS_DEBUG'] = "4"
                
        # To also accept comma-separeted input lists
        def _argparse_postproc_doBANDS(self, v):
            return utils.parse_comma_separated_list(v)

    def prewash_scamp(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""

        # Re-cast the ctx.assoc/ctx.catlist as dictionary of arrays instead of lists
        self.ctx.catlist  = utils.dict2arrays(self.ctx.catlist)

        # Get the BANDs information in the context if they are not present
        if self.ctx.get('gotBANDS'):
            self.logger.info("BANDs already defined in context -- skipping")
        else:
            self.ctx.update(contextDefs.get_BANDS(self.ctx.catlist, detname=self.ctx.detname,logger=self.logger,doBANDS=self.ctx.doBANDS))

        # Check info OK
        self.logger.info("BANDS:   %s" % self.ctx.BANDS)
        self.logger.info("doBANDS: %s" % self.ctx.doBANDS)
        self.logger.info("dBANDS:  %s" % self.ctx.dBANDS)
        
    def run(self):
        
        execution_mode = self.input.execution_mode_scamp_prep

        # 0. Prepare the context
        self.prewash_scamp()

        # 1. Get the unitnames for the doBANDS selection
        self.ctx.unitnames = contextDefs.get_scamp_unitnames(self.ctx)

        # 2. Get the list of command lines
        cmdlist = self.get_combine_cats_cmd_list(execution_mode)
        
        # 3. Execute a cmdlist according to execution_mode 
        # --> the input list files are written to the AUX directory
        self.logger.info("Combining CCD catalogs for scamp")
        if execution_mode == 'tofile':
            self.writeCall_combine_cats(cmdlist)
        elif execution_mode == 'dryrun':
            [self.logger.info(' '.join(cmdlist[key])) for key in cmdlist.keys()]
        elif execution_mode == 'execute':
            self.combine_cats_for_scamp(self.input.MP_cats)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)
        return

    # Create the inputs and combine the catalogs for scamp
    # -------------------------------------------------------------------------
    def combine_cats_for_scamp(self,MP):
        """
        Combine the CCD catalogs into exposure-based catalogs for scamp inputs
        """

        t0 = time.time()
        NP = utils.get_NP(MP)  # Figure out NP to use, 0=automatic
        
        if NP > 1 :
            p = mp.Pool(processes=NP)
        # Loop over all unitnames
        for UNITNAME in self.ctx.unitnames:
            self.logger.info("Combining CCD catalogs for %s" % UNITNAME)
            # The sorted CCD catlist per UNITNAME and the output name for the conbined cat as args
            args = (','.join(contextDefs.get_ccd_catlist(self.ctx.catlist,UNITNAME)),
                    fh.get_expcat_file(self.input.tiledir, self.input.tilename_fh, UNITNAME))
            kw = {}
            if NP > 1:
                p.apply_async(fitsutils.combine_cats, args, kw)
            else:
                fitsutils.combine_cats(*args)
                
        if NP > 1:
            p.close()
            p.join()

        self.logger.info("Total Catalog Combine time: %s" % elapsed_time(t0))
        return

    # Assemble the command
    # -------------------------------------------------------------------------
    def get_combine_cats_cmd_list(self,execute_mode):

        # Sortcuts for less typing
        tiledir     = self.input.tiledir
        tilename_fh = self.input.tilename_fh
        # The dictionary, keyed to UNITNAMES where we store the commands
        combine_cats_cmd = {}
        for UNITNAME in self.ctx.unitnames:
            # Get the sorted CCD catlist per UNITNAME
            ccd_catlist = contextDefs.get_ccd_catlist(self.ctx.catlist,UNITNAME)
            # Write out the file list containing the input catalogs
            if execute_mode == 'tofile' or execute_mode == 'dryrun':
                tableio.put_data(fh.get_catlist_file(tiledir, tilename_fh, UNITNAME),(ccd_catlist,),format="%s")
                self.logger.debug("Writing CCD red catalog list file: %s" % fh.get_catlist_file(tiledir, tilename_fh, UNITNAME))

            # Build the cmdline string
            cmd = []
            cmd.append("combine_cats.py")
            cmd.append("--list  %s" % fh.get_catlist_file(tiledir, tilename_fh, UNITNAME))
            cmd.append("--outcat %s" % fh.get_expcat_file(tiledir, tilename_fh, UNITNAME))
            combine_cats_cmd[UNITNAME] = cmd
        return combine_cats_cmd

    # 'EXECUTION' FUNCTIONS
    # -------------------------------------------------------------------------
    def writeCall_combine_cats(self,cmd_list,mode='w'):

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_combine_cats_cmd_file(self.input.tiledir, self.input.tilename_fh)
        self.logger.info("Will write combine_cats call to: %s" % cmdfile)
        with open(cmdfile, mode) as fid:
            for key in cmd_list.keys():
                fid.write(bkline.join(cmd_list[key])+'\n')
                fid.write('\n\n')
        return


    def __str__(self):
        return 'Prepares the file for the scamp call'

if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)



