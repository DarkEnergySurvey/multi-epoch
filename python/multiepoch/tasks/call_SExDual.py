#!/usr/bin/env python

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

import os
import sys
import subprocess
import multiprocessing
import time
from despymisc.miscutils import elapsed_time

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh


# JOB INTERNAL CONFIGURATION
SEX_EXE = 'sex'
BKLINE = "\\\n"

class Job(BaseJob):

    """
    SExtractor call for catalog creation
    """

    class Input(IO):

        """
        SExtractor call for catalog creation
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

        SExDual_execution_mode = CUnicode("tofile",help="SExtractor Dual excution mode",
                                          argparse={'choices': ('tofile','dryrun','execute')})
        SExDual_parameters     = Dict({},help="A list of parameters to pass to SExtractor", argparse={'nargs':'+',})
        MP_SEx        = CInt(1,help="run using multi-process, 0=automatic, 1=single-process [default]")

        def _validate_conditional(self):
            # if in job standalone mode json
            #if self.mojo_execution_mode == 'job as script' and self.basename == "":
            #    mess = 'If job is run standalone basename cannot be ""'
            #    raise IO_ValidationError(mess)
            pass

        def _argparse_postproc_SExDual_parameters(self, v):
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
        cmd_list = self.get_SExDual_cmd_list()

        # 2. check execution mode and write/print/execute commands accordingly --------------
        executione_mode = self.ctx.get('SExDual_execution_mode', 'tofile')
        if executione_mode == 'tofile':
            self.writeCall(cmd_list)
            
        elif executione_mode == 'dryrun':
            self.logger.info("# For now we only print the commands (dry-run)")
            for band in self.ctx.BANDS:
                self.logger.info(' '.join(cmd_list[band]))

        elif executione_mode == 'execute':
            MP = self.ctx.MP_SEx # MP or single Processs
            self.runSExDual(cmd_list,MP=MP)
        else:
            raise ValueError('Execution mode %s not implemented.' % executione_mode)
        return

    def writeCall(self,cmd_list):

        """ Write the SEx psf call to a file """

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_SExdual_cmd_file(self.input.tiledir, self.input.tilename)
        self.logger.info("# Will write SExDual call to: %s" % cmdfile)
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.BANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return

    def runSExDual(self,cmd_list,MP):

        self.logger.info("# Will proceed to run the SEx Dual call now:")
        t0 = time.time()
        NP = utils.get_NP(MP) # Figure out NP to use, 0=automatic
        
        # Case A -- NP=1
        if NP == 1:
            logfile = fh.get_SExdual_log_file(self.input.tiledir, self.input.tilename)
            log = open(logfile,"w")
            self.logger.info("# Will write to logfile: %s" % logfile)

            for band in self.ctx.BANDS:
                t1 = time.time()
                cmd  = ' '.join(cmd_list[band])
                self.logger.info("# Executing SExDual for BAND:%s" % band)
                self.logger.info("# %s " % cmd)
                status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
                if status > 0:
                    raise RuntimeError("\n***\nERROR while running SExDual, check logfile: %s\n***" % logfile)
                self.logger.info("# Done band %s in %s\n" % (band,elapsed_time(t1)))
            
        # Case B -- multi-process in case NP > 1
        else:
            self.logger.info("# Will Use %s processors" % NP)
            cmds = []
            logs = []
            for band in self.ctx.BANDS:
                cmds.append(' '.join(cmd_list[band]))
                logfile = fh.get_SExdual_log_file(self.input.tiledir, self.input.tilename,band)
                logs.append(logfile)
                self.logger.info("# Will write to logfile: %s" % logfile)
                
            pool = multiprocessing.Pool(processes=NP)
            pool.map(utils.work_subprocess_logging, zip(cmds,logs))

        self.logger.info("# Total SExtractor Dual time %s" % elapsed_time(t0))
        return


    def get_SExDual_parameter_set(self,**kwargs):

        """
        Set the SEx default options for dual SExtrator run and have the
        options to overwrite them with kwargs to this function.
        """

        # General pars, BAND-independent
        SExDual_parameters = {
            'CATALOG_TYPE'    : "FITS_LDAC",
            'FILTER_NAME'     : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.conv'),
            'STARNNW_NAME'    : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.nnw'),
            'CATALOG_TYPE'    :   'FITS_1.0',
            'WEIGHT_TYPE'     : 'MAP_WEIGHT',
            'MEMORY_BUFSIZE'  : 2048,
            'CHECKIMAGE_TYPE' : 'SEGMENTATION',
            'DETECT_THRESH'   : 1.5,
            'DEBLEND_MINCONT' : 0.001, 
            #'PARAMETERS_NAME' : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param'),
            'PARAMETERS_NAME' : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param_psfonly'), # Faster!!!
            'VERBOSE_TYPE'    : 'NORMAL',
            }

        # Now update pars with kwargs
        SExDual_parameters.update(kwargs)
        return SExDual_parameters

    def get_SExDual_cmd_list(self):
        
        """ Build/Execute the SExtractor call for psf on the detection image"""

        # Sortcuts for less typing
        tiledir  = self.input.tiledir
        tilename = self.input.tilename
        self.logger.info('# assembling commands for SEx Dual call')

        # SEx default configuration
        sex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.sex')

        # The updated parameters set for SEx
        pars = self.get_SExDual_parameter_set(**self.input.SExDual_parameters)

        SExDual_cmd = {}
        # Loop over all bands 
        dBAND = self.ctx.get('detBAND','det') 
        for BAND in self.ctx.BANDS:

            # Spell out input and output names
            # From SEx/psfex
            sexcat = fh.get_cat_file(tiledir, tilename, BAND)
            psf    = fh.get_psf_file(tiledir, tilename, BAND)
            seg    = fh.get_seg_file(tiledir, tilename, BAND)
            # Combined images and weights
            sci_comb     = fh.get_sci_fits_file(tiledir, tilename, BAND)
            sci_comb_det = fh.get_sci_fits_file(tiledir, tilename, dBAND)
            wgt_comb     = fh.get_wgt_fits_file(tiledir, tilename, BAND)
            wgt_comb_det = fh.get_wgt_fits_file(tiledir, tilename, dBAND)
            
            pars['MAG_ZEROPOINT']   =  30.0000 
            pars['CATALOG_NAME']    =  sexcat           
            pars['PSF_NAME']        =  psf
            pars['CHECKIMAGE_NAME'] =  seg
            pars['WEIGHT_IMAGE']    =  "%s,%s" % (wgt_comb_det, wgt_comb)
            
            # Build the call for the band, using dband as detection image
            cmd = []
            cmd.append("%s" % SEX_EXE)
            cmd.append("%s,%s" % (sci_comb_det,sci_comb))
            cmd.append("-c %s" % sex_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            SExDual_cmd[BAND] = cmd

        return SExDual_cmd


    def __str__(self):
        return 'Creates the SExtractor call for dual detection'


if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
