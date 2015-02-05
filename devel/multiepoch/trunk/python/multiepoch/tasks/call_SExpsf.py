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
import multiprocessing
from despymisc.miscutils import elapsed_time

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs


# JOB INTERNAL CONFIGURATION
SEX_EXE = 'sex'
BKLINE = "\\\n"

class Job(BaseJob):

    """
    SExtractor call for psf creation

    Inputs:
    - self.ctx.comb_sci
    Outputs:
    - self.ctx.psfcat
    """

    class Input(IO):

        """ SExtractor call to build inputs for psfex"""

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
        SExpsf_execution_mode  = CUnicode("tofile",help="SEx for psfex excution mode",
                                          argparse={'choices': ('tofile','dryrun','execute')})
        SExpsf_parameters       = Dict({},help="A list of parameters to pass to SExtractor",
                                       argparse={'nargs':'+',})
        MP_SEx        = CInt(1,help="run using multi-process, 0=automatic, 1=single-process [default]")

        def _validate_conditional(self):
            # if in job standalone mode json
            if self.mojo_execution_mode == 'job as script' and self.basename == "":
                mess = 'If job is run standalone basename cannot be ""'
                raise IO_ValidationError(mess)

        def _argparse_postproc_SExpsf_parameters(self, v):
            return utils.arglist2dict(v, separator='=')


    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""
        
        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # Make sure we set up the output dir
        self.ctx = contextDefs.set_tile_directory(self.ctx)
        # 1. set up names 
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
        SExpsf_parameters = self.ctx.get('SExpsf_parameters', {})

        # 2. Build the list of command line for SEx for psf
        cmd_list = self.get_SExpsf_cmd_list(SExpsf_parameters=SExpsf_parameters)

        # 3. check execution mode and write/print/execute commands accordingly --------------
        executione_mode = self.ctx.get('SExpsf_execution_mode', 'tofile')
        if executione_mode == 'tofile':
            self.writeCall(cmd_list)

        elif executione_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            for band in self.ctx.dBANDS:
                print ' '.join(cmd_list[band])

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
        cmdfile = self.ctx.get('cmdfile', "%s_call_SExpsf.cmd" % self.ctx.basename)
        print "# Will write SExpsf call to: %s" % cmdfile
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return


    def runSExpsf(self,cmd_list,MP):

        print "# Will proceed to run the SEx psf call now:"
        t0 = time.time()
        NP = utils.get_NP(MP) # Figure out NP to use, 0=automatic
        
        # Case A -- NP=1
        if NP == 1:
            logfile = self.ctx.get('SExpsf_logfile',  os.path.join(self.ctx.logdir,"%s_SExpsf.log" % self.ctx.filepattern))
            log = open(logfile,"w")
            print "# Will write to logfile: %s" % logfile
            for band in self.ctx.dBANDS:
                t1 = time.time()
                cmd  = ' '.join(cmd_list[band])
                print "# Executing SEx/psf for BAND:%s" % band
                print "# %s " % cmd
                status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
                if status > 0:
                    raise RuntimeError("\n***\nERROR while running SExpsf, check logfile: %s\n***" % logfile)
                print "# Done band %s in %s\n" % (band,elapsed_time(t1))
            
        # Case B -- multi-process in case NP > 1
        else:
            print "# Will Use %s processors" % NP
            cmds = []
            logs = []
            for band in self.ctx.dBANDS:
                cmds.append(' '.join(cmd_list[band]))
                logfile = os.path.join(self.ctx.logdir,"%s_%s_SExpsf.log" % (self.ctx.filepattern,band))
                logs.append(logfile)
                
            pool = multiprocessing.Pool(processes=NP)
            pool.map(utils.work_subprocess_logging, zip(cmds,logs))

        print "# Total SEx psf time %s" % elapsed_time(t0)
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

    def get_SExpsf_cmd_list(self,SExpsf_parameters={}):
        
        """ Build/Execute the SExtractor call for psf on the detection image"""

        # SEx default configuration
        sex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.sex')
        
        # The updated parameters set for SEx
        pars = self.get_SExpsf_parameter_set(**SExpsf_parameters)

        SExpsf_cmd = {}
        # Loop over all bands and Detection
        for BAND in self.ctx.dBANDS:
            pars['WEIGHT_IMAGE'] = self.ctx.comb_wgt[BAND]
            pars['CATALOG_NAME'] = self.ctx.psfcat[BAND]
            # Build the call
            
            cmd = []
            cmd.append("%s" % SEX_EXE)
            cmd.append("%s" % self.ctx.comb_sci[BAND])
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
    


