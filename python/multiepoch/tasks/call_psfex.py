#!/usr/bin/env python


# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

import os
import sys
import time
import pandas as pd
import subprocess
from despymisc.miscutils import elapsed_time

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh


# JOB INTERNAL CONFIGURATION
PSFEX_EXE = 'psfex'
DETNAME   = 'det'
BKLINE    = "\\\n"

class Job(BaseJob):

    """
    psfex call for the multi-epoch pipeline
    runs psfex on coadd images and detection
    """

    class Input(IO):

        """
        The psfex call for the multi-epoch pipeline runs psfex on coadd images and detection image
        """

        ######################
        # Positional Arguments
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })
        # Optional Arguments
        tilename    = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir     = Unicode(None, help='The output directory for this tile.')
        execution_mode_psfex  = CUnicode("tofile",help="psfex excution mode",
                                          argparse={'choices': ('tofile','dryrun','execute')})
        psfex_parameters       = Dict({},help="A list of parameters to pass to SExtractor",argparse={'nargs':'+',})
        psfex_conf = CUnicode(help="Optional psfex configuration file")

        doBANDS  = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})
        detname  = CUnicode(DETNAME,help="File label for detection image, default=%s." % DETNAME)
        nthreads = CInt(1,help="Number of threads to use in stiff/psfex/swarp")

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

        def _argparse_postproc_psfex_parameters(self, v):
            return utils.arglist2dict(v, separator='=')

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

        # 0. Prepare the context
        self.prewash()
        
        # 2. Build the list of command line for SEx for psf
        cmd_list = self.get_psfex_cmd_list()

        # 3. check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.execution_mode_psfex
        if execution_mode == 'tofile':
            self.writeCall(cmd_list)

        elif execution_mode == 'dryrun':
            self.logger.info("For now we only print the commands (dry-run)")
            for band in self.ctx.dBANDS:
                self.logger.info(' '.join(cmd_list[band]))

        elif execution_mode == 'execute':
            self.runpsfex(cmd_list)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)

        return

    def writeCall(self,cmd_list):

        """ Write the psfex call to a file """

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_psfex_cmd_file(self.input.tiledir, self.input.tilename_fh)
        self.logger.info("Will write psfex call to: %s" % cmdfile)
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return


    def runpsfex(self,cmd_list):

        t0 = time.time()
        self.logger.info("Will proceed to run the psfex call now:")
        logfile = fh.get_psfex_log_file(self.input.tiledir, self.input.tilename_fh)
        log = open(logfile,"w")
        self.logger.info("Will write to logfile: %s" % logfile)

        for band in self.ctx.dBANDS:
            t1 = time.time()
            cmd  = ' '.join(cmd_list[band])
            self.logger.info("Executing psfex for BAND:%s" % band)
            self.logger.info("%s " % cmd)
            status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            if status != 0:
                raise RuntimeError("\n***\nERROR while running psfex, check logfile: %s\n***" % logfile)
            self.logger.info("Done band %s in %s" % (band,elapsed_time(t1)))
        self.logger.info("Total psfex time %s" % elapsed_time(t0))
        return

    def get_psfex_parameter_set(self,**kwargs):

        """
        Set the psfex default options for the psf run and have the
        options to overwrite them with kwargs to this function.
        """
        psfex_parameters = {
            'WRITE_XML' : 'Y',
            "NTHREADS"  : self.ctx.nthreads,
        }
        # Now update pars with kwargs
        psfex_parameters.update(kwargs)
        return psfex_parameters

    def get_psfex_cmd_list(self):
        
        """ Build the psfex call that runs psfex on coadd images and detection"""

        self.logger.info('assembling commands for psfex call')

        # Sortcuts for less typing
        tiledir  = self.input.tiledir
        tilename_fh = self.input.tilename_fh

        # The psfex configuration file
        if self.input.psfex_conf == '':
            self.ctx.psfex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.psfex')
            self.logger.info("Will use SEx for psf default configuration file: %s" % self.ctx.psfex_conf)
        
        # The updated parameters set for psfex
        pars = self.get_psfex_parameter_set(**self.input.psfex_parameters)

        # Loop over all bands and Detection
        psfex_cmd = {}
        for BAND in self.ctx.dBANDS:

            # input and output
            psfcat = fh.get_psfcat_file(tiledir,tilename_fh, BAND)
            psfxml = fh.get_psfxml_file(tiledir,tilename_fh, BAND)

            pars['XML_NAME'] = psfxml
            # Build the call
            cmd = []
            cmd.append("%s" % PSFEX_EXE)
            cmd.append("%s" % psfcat)
            cmd.append("-c %s" % self.ctx.psfex_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            psfex_cmd[BAND] = cmd

        return psfex_cmd


    def __str__(self):
        return 'Creates the psfex call for multi-epoch pipeline'



if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
