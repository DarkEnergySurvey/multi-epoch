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
import multiprocessing
from despymisc.miscutils import elapsed_time

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh


# JOB INTERNAL CONFIGURATION
SEX_EXE = 'sex'
BKLINE = "\\\n"

class Job(BaseJob):

    """
    SExtractor call for psf creation
    """

    class Input(IO):

        """ SExtractor call to build inputs for psfex"""

        #######################
        # Positional Arguments
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })

        # Optional Arguments
        tilename = Unicode(None, help="The Name of the Tile Name to query")
        tiledir  = Unicode(None, help='The output directory for this tile.')
        SExpsf_execution_mode  = CUnicode("tofile",help="SEx for psfex excution mode",
                                          argparse={'choices': ('tofile','dryrun','execute')})
        SExpsf_parameters       = Dict({},help="A list of parameters to pass to SExtractor",
                                       argparse={'nargs':'+',})
        MP_SEx        = CInt(1,help="run using multi-process, 0=automatic, 1=single-process [default]")

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

        def _argparse_postproc_SExpsf_parameters(self, v):
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
        
        # 1. Build the list of command line for SEx for psf
        cmd_list = self.get_SExpsf_cmd_list()

        # 2. check execution mode and write/print/execute commands accordingly --------------
        executione_mode = self.ctx.get('SExpsf_execution_mode', 'tofile')
        if executione_mode == 'tofile':
            self.writeCall(cmd_list)

        elif executione_mode == 'dryrun':
            self.logger.info("# For now we only print the commands (dry-run)")
            for band in self.ctx.dBANDS:
                self.logger.info(' '.join(cmd_list[band]))

        elif executione_mode == 'execute':
            MP = self.ctx.MP_SEx # MP or single Processs
            self.runSExpsf(cmd_list,MP=MP)
        else:
            raise ValueError('Execution mode %s not implemented.' % executione_mode)
        return

    def writeCall(self,cmd_list):

        """ Write the SEx psf call to a file """

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_sexpsf_cmd_file(self.input.tiledir, self.input.tilename)
        self.logger.info("# Will write SExpsf call to: %s" % cmdfile)
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return


    def runSExpsf(self,cmd_list,MP):

        self.logger.info("# Will proceed to run the SEx psf call now:")
        t0 = time.time()
        NP = utils.get_NP(MP) # Figure out NP to use, 0=automatic
        
        # Case A -- NP=1
        if NP == 1:
            logfile = fh.get_sexpsf_log_file(self.input.tiledir, self.input.tilename)
            log = open(logfile,"w")
            self.logger.info("# Will write to logfile: %s" % logfile)
            for band in self.ctx.dBANDS:
                t1 = time.time()
                cmd  = ' '.join(cmd_list[band])
                self.logger.info("# Executing SEx/psf for BAND:%s" % band)
                self.logger.info("# %s " % cmd)
                status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
                if status > 0:
                    raise RuntimeError("\n***\nERROR while running SExpsf, check logfile: %s\n***" % logfile)
                self.logger.info("# Done band %s in %s\n" % (band,elapsed_time(t1)))
            
        # Case B -- multi-process in case NP > 1
        else:
            self.logger.info("# Will Use %s processors" % NP)
            cmds = []
            logs = []
            for band in self.ctx.dBANDS:
                cmds.append(' '.join(cmd_list[band]))
                logfile = fh.get_sexpsf_log_file(self.input.tiledir, self.input.tilename,band)
                logs.append(logfile)
                self.logger.info("# Will write to logfile: %s" % logfile)
                
            pool = multiprocessing.Pool(processes=NP)
            pool.map(utils.work_subprocess_logging, zip(cmds,logs))

        self.logger.info("# Total SEx psf time %s" % elapsed_time(t0))
        return


    def get_SExpsf_parameter_set(self,**kwargs):

        """
        Set the SEx default options for the psf run and have the
        options to overwrite them with kwargs to this function.
        """

        SExpsf_parameters = {
            'CATALOG_TYPE'    : "FITS_LDAC",
            'WEIGHT_TYPE'     : 'MAP_WEIGHT',
            'PARAMETERS_NAME' : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param_psfex'),
            'FILTER_NAME'     : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.conv'),
            'STARNNW_NAME'    : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.nnw'),
            'SATUR_LEVEL'     : 65000,
            'VERBOSE_TYPE'    : 'NORMAL',
            'DETECT_MINAREA'  : 3, # WHY??????????? son small!!!!
            }

        # Now update pars with kwargs
        SExpsf_parameters.update(kwargs)
        return SExpsf_parameters

    def get_SExpsf_cmd_list(self):
        
        """ Build/Execute the SExtractor call for psf on the detection image"""

        # Sortcuts for less typing
        tiledir  = self.input.tiledir
        tilename = self.input.tilename

        self.logger.info('# assembling commands for SWarp call')

        # The updated parameters set for SEx
        pars = self.get_SExpsf_parameter_set(**self.input.SExpsf_parameters)

        # SEx default configuration
        # FIXME : this only works in the eeups context where MULTIEPOCH_DIR is set ..
        sex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.sex')

        SExpsf_cmd = {}
        # Loop over all bands and Detection
        for BAND in self.ctx.dBANDS:

            pars["WEIGHT_IMAGE"]  = "%s" % fh.get_wgt_fits_file(tiledir,tilename, BAND)
            pars["CATALOG_NAME"] = "%s"  % fh.get_psfcat_file(tiledir,tilename, BAND)

            # Build the call
            cmd = []
            cmd.append("%s" % SEX_EXE)
            cmd.append("%s" % fh.get_sci_fits_file(tiledir,tilename, BAND) )
            cmd.append("-c %s" % sex_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            SExpsf_cmd[BAND] = cmd

        return SExpsf_cmd


    def __str__(self):
        return 'Creates the SExtractor call for psf'


if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
    


