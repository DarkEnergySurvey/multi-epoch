#!/usr/bin/env python


# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

import os
import sys
import time
import subprocess
from despymisc.miscutils import elapsed_time

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh


# JOB INTERNAL CONFIGURATION
PSFEX_EXE = 'psfex'
BKLINE = "\\\n"

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
        # Required inputs
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })
        # Optional Arguments
        tilename = Unicode(None, help="The Name of the Tile Name to query",argparse=False)
        tiledir  = CUnicode(None, help='The output directory for this tile.')
        psfex_execution_mode  = CUnicode("tofile",help="psfex excution mode",
                                          argparse={'choices': ('tofile','dryrun','execute')})
        psfex_parameters       = Dict({},help="A list of parameters to pass to SExtractor",argparse={'nargs':'+',})
        cleanupPSFcats         = Bool(False, help="Clean-up PSFcat.fits files")

        def _validate_conditional(self):
            ## if in job standalone mode json
            #if self.mojo_execution_mode == 'job as script' and self.basename == "":
            #    mess = 'If job is run standalone basename cannot be ""'
            #    raise IO_ValidationError(mess)
            pass

        def _argparse_postproc_psfex_parameters(self, v):
            return utils.arglist2dict(v, separator='=')
        

    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""

        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # Get the BANDs information in the context if they are not present
        self.ctx.update(contextDefs.get_BANDS(self.ctx.assoc, detname='det',logger=self.logger))

    def run(self):

        # 0. Prepare the context
        self.prewash()
        
        # 2. Build the list of command line for SEx for psf
        cmd_list = self.get_psfex_cmd_list()

        # 3. check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.get('psfex_execution_mode', 'tofile')
        if execution_mode == 'tofile':
            self.writeCall(cmd_list)

        elif execution_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            for band in self.ctx.dBANDS:
                self.logger.info(' '.join(cmd_list[band]))

        elif execution_mode == 'execute':
            self.runpsfex(cmd_list)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)

        # 4. Clean up psfcat files
        if self.input.cleanupPSFcats:
            self.cleanup_PSFcats(execute=True)

        return

    def writeCall(self,cmd_list):

        """ Write the psfex call to a file """

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_psfex_cmd_file(self.input.tiledir, self.input.tilename)
        self.logger.info("# Will write psfex call to: %s" % cmdfile)
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return


    def runpsfex(self,cmd_list):

        t0 = time.time()
        self.logger.info("# Will proceed to run the psfex call now:")
        logfile = fh.get_sexpsf_log_file(self.input.tiledir, self.input.tilename)
        log = open(logfile,"w")
        self.logger.info("# Will write to logfile: %s" % logfile)

        for band in self.ctx.dBANDS:
            t1 = time.time()
            cmd  = ' '.join(cmd_list[band])
            self.logger.info("# Executing psfex for BAND:%s" % band)
            self.logger.info("# %s " % cmd)
            status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            if status > 0:
                raise RuntimeError("\n***\nERROR while running psfex, check logfile: %s\n***" % logfile)
            self.logger.info("# Done band %s in %s\n" % (band,elapsed_time(t1)))
        self.logger.info("# Total psfex time %s" % elapsed_time(t0))
        return

    def get_psfex_parameter_set(self,**kwargs):

        """
        Set the psfex default options for the psf run and have the
        options to overwrite them with kwargs to this function.
        """
        psfex_parameters = {
            'WRITE_XML' : 'Y',
        }
        # Now update pars with kwargs
        psfex_parameters.update(kwargs)
        return psfex_parameters

    def get_psfex_cmd_list(self):
        
        """ Build the psfex call that runs psfex on coadd images and detection"""

        self.logger.info('# assembling commands for psfex call')

        # Sortcuts for less typing
        tiledir  = self.input.tiledir
        tilename = self.input.tilename

        # psfex default configuration
        psfex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.psfex')
        
        # The updated parameters set for psfex
        pars = self.get_psfex_parameter_set(**self.input.psfex_parameters)

        # Loop over all bands and Detection
        psfex_cmd = {}
        for BAND in self.ctx.dBANDS:

            # input and output
            psfcat = fh.get_psfcat_file(tiledir,tilename, BAND)
            psfxml = fh.get_psfxml_file(tiledir,tilename, BAND)

            pars['XML_NAME'] = psfxml
            # Build the call
            cmd = []
            cmd.append("%s" % PSFEX_EXE)
            cmd.append("%s" % psfcat)
            cmd.append("-c %s" % psfex_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            psfex_cmd[BAND] = cmd

        return psfex_cmd


    def cleanup_PSFcats(self,execute=False):

        for BAND in self.ctx.dBANDS:
            print "# Cleaning up %s" % self.ctx.psfcat[BAND]
            if execute:
                try:
                    os.remove(self.ctx.psfcat[BAND])
                except:
                    print "# Warning: cannot remove %s" % self.ctx.psfcat[BAND]
        return


    def __str__(self):
        return 'Creates the psfex call for multi-epoch pipeline'



if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
