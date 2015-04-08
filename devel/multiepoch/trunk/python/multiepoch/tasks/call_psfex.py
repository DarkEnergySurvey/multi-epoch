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


# JOB INTERNAL CONFIGURATION
PSFEX_EXE = 'psfex'
BKLINE = "\\\n"

class Job(BaseJob):

    """
    psfex call for the multi-epoch pipeline
    runs psfex on coadd images and detection

    Inputs:
    - self.ctx.psfcat[BAND] 

    Ouputs
    - self.ctx.psf[BAND] 

    """

    class Input(IO):

        """
        The psfex call for the multi-epoch pipeline runs psfex on coadd images and detection image
        """

        ######################
        # Required inputs
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",
                          argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",
                              input_file=True,
                              argparse={ 'argtype': 'positional', })
        # Optional Arguments
        basename               = CUnicode("",help="Base Name for coadd fits files in the shape: COADD_BASENAME_$BAND.fits")
        psfex_execution_mode  = CUnicode("tofile",help="psfex excution mode",
                                          argparse={'choices': ('tofile','dryrun','execute')})
        psfex_parameters       = Dict({},help="A list of parameters to pass to SExtractor",
                                       argparse={'nargs':'+',})
        cleanupPSFcats         = Bool(False, help="Clean-up PSFcat.fits files")

        def _validate_conditional(self):
            # if in job standalone mode json
            if self.mojo_execution_mode == 'job as script' and self.basename == "":
                mess = 'If job is run standalone basename cannot be ""'
                raise IO_ValidationError(mess)

        def _argparse_postproc_psfex_parameters(self, v):
            return utils.arglist2dict(v, separator='=')
        

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

        # 0. Prepare the context
        self.prewash()
        
        # 1. get the update SEx parameters for psf --
        psfex_parameters = self.ctx.get('psfex_parameters', {})

        # 2. Build the list of command line for SEx for psf
        cmd_list = self.get_psfex_cmd_list(psfex_parameters=psfex_parameters)

        # 3. check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.get('psfex_execution_mode', 'tofile')
        if execution_mode == 'tofile':
            self.writeCall(cmd_list)

        elif execution_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            for band in self.ctx.dBANDS:
                print ' '.join(cmd_list[band])

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
        cmdfile = self.ctx.get('cmdfile', "%s_call_psfex.cmd" % self.ctx.basename)
        print "# Will write psfex call to: %s" % cmdfile
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return


    def runpsfex(self,cmd_list):

        t0 = time.time()
        print "# Will proceed to run the psfex call now:"
        logfile = self.ctx.get('SExpsf_logfile',  os.path.join(self.ctx.logdir,"%s_psfex.log" % self.ctx.filepattern))
        log = open(logfile,"w")
        print "# Will write to logfile: %s" % logfile

        for band in self.ctx.dBANDS:
            t1 = time.time()
            cmd  = ' '.join(cmd_list[band])
            print "# Executing psfex for BAND:%s" % band
            print "# %s " % cmd
            status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            if status > 0:
                raise RuntimeError("\n***\nERROR while running psfex, check logfile: %s\n***" % logfile)
            print "# Done band %s in %s\n" % (band,elapsed_time(t1))
        print "# Total psfex time %s" % elapsed_time(t0)
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

    def get_psfex_cmd_list(self,psfex_parameters={}):
        
        """ Build the psfex call that runs psfex on coadd images and detection"""

        # psfex default configuration
        psfex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.psfex')
        
        # The updated parameters set for SEx
        pars = self.get_psfex_parameter_set(**psfex_parameters)

        # Loop over all bands and Detection
        psfex_cmd = {}
        for BAND in self.ctx.dBANDS:
            pars['XML_NAME'] = self.ctx.psfexxml[BAND]
            # Build the call
            cmd = []
            cmd.append("%s" % PSFEX_EXE)
            cmd.append("%s" % self.ctx.psfcat[BAND])
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
