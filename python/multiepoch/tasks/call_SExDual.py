#!/usr/bin/env python

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError

import os
import sys
import subprocess
import multiprocessing
import time
import pandas as pd
from despymisc.miscutils import elapsed_time

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh


# JOB INTERNAL CONFIGURATION
SEX_EXE = 'sex'
DETNAME = 'det'
MAGBASE = 30.0
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
        tilename    = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir  = Unicode(None, help='The output directory for this tile.')

        execution_mode_SExDual = CUnicode("tofile",help="SExtractor Dual excution mode",
                                          argparse={'choices': ('tofile','dryrun','execute')})
        SExDual_parameters = Dict({},help="A list of parameters to pass to SExtractor", argparse={'nargs':'+',})
        SExDual_conf       = CUnicode(help="Optional SExtractor Dual mode configuration file")

        MP_SEx        = CInt(1,help="run using multi-process, 0=automatic, 1=single-process [default]")
        doBANDS       = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})
        detname       = CUnicode(DETNAME,help="File label for detection image, default=%s." % DETNAME)
        magbase       = CFloat(MAGBASE, help="Zero point magnitude base for SWarp, default=%s." % MAGBASE)
        cleanupPSFcats = Bool(False, help="Clean-up PSFcat.fits files")
        
        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

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

        def _argparse_postproc_SExDual_parameters(self, v):
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

        # 1. Build the list of command line for SEx for psf
        cmd_list = self.get_SExDual_cmd_list()

        # 2. check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.execution_mode_SExDual
        if execution_mode == 'tofile':
            self.writeCall(cmd_list)
            
        elif execution_mode == 'dryrun':
            self.logger.info("For now we only print the commands (dry-run)")
            for band in self.ctx.dBANDS:
                self.logger.info(' '.join(cmd_list[band]))

        elif execution_mode == 'execute':
            MP = self.ctx.MP_SEx # MP or single Processs
            self.runSExDual(cmd_list,MP=MP)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)

        # 3. Clean up psfcat files
        if self.input.cleanupPSFcats and execution_mode == 'execute':
            self.cleanup_PSFcats(execute=True)

        return

    def writeCall(self,cmd_list):

        """ Write the SEx psf call to a file """

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_SExdual_cmd_file(self.input.tiledir, self.input.tilename_fh)
        self.logger.info("Will write SExDual call to: %s" % cmdfile)
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return

    def runSExDual(self,cmd_list,MP):

        self.logger.info("Will proceed to run the SEx Dual call now:")
        t0 = time.time()
        NP = utils.get_NP(MP) # Figure out NP to use, 0=automatic
        
        # Case A -- NP=1
        if NP == 1:
            logfile = fh.get_SExdual_log_file(self.input.tiledir, self.input.tilename_fh)
            log = open(logfile,"w")
            self.logger.info("Will write to logfile: %s" % logfile)

            for band in self.ctx.dBANDS:
                t1 = time.time()
                cmd  = ' '.join(cmd_list[band])
                self.logger.info("Executing SExDual for BAND:%s" % band)
                self.logger.info("%s " % cmd)
                status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
                if status != 0:
                    raise RuntimeError("\n***\nERROR while running SExDual, check logfile: %s\n***" % logfile)
                self.logger.info("Done band %s in %s" % (band,elapsed_time(t1)))
            
        # Case B -- multi-process in case NP > 1
        else:
            self.logger.info("Will Use %s processors" % NP)
            cmds = []
            logs = []
            for band in self.ctx.dBANDS:
                cmds.append(' '.join(cmd_list[band]))
                logfile = fh.get_SExdual_log_file(self.input.tiledir, self.input.tilename_fh,band)
                logs.append(logfile)
                self.logger.info("Will write to logfile: %s" % logfile)
                
            pool = multiprocessing.Pool(processes=NP)
            pool.map(utils.work_subprocess_logging, zip(cmds,logs))

        self.logger.info("Total SExtractor Dual time %s" % elapsed_time(t0))
        return


    def get_SExDual_parameter_set(self,**kwargs):

        """
        Set the SEx default options for dual SExtrator run and have the
        options to overwrite them with kwargs to this function.
        """
        # Short cuts
        MULTIEPOCH_DIR = os.environ['MULTIEPOCH_DIR']
        CONFIG_DATE    = os.environ['MULTIEPOCH_CONFIG_DATE']

        # General pars, BAND-independent
        SExDual_parameters = {
            'DEBLEND_MINCONT' : 0.001,
            'MAG_ZEROPOINT'   : self.ctx.magbase,
            'CHECKIMAGE_TYPE' : 'SEGMENTATION',
            'FILTER_NAME'     : os.path.join(,'etc','gauss_3.0_7x7.conv'),
            'STARNNW_NAME'    : os.path.join(MULTIEPOCH_DIR,'etc','sex.nnw'),
            'PARAMETERS_NAME' : os.path.join(MULTIEPOCH_DIR,'etc',CONFIG_DATE+'_sex.param_diskonly'), # disk-only psf
            #'PARAMETERS_NAME' : os.path.join(MULTIEPOCH_DIR,'etc',CONFIG_DATE+'_sex.param_nomodel'), # Way Faster -- no model for tesing!!!
            }

        # Now update pars with kwargs -- with override the above definitions
        SExDual_parameters.update(kwargs)
        return SExDual_parameters

    def get_SExDual_cmd_list(self):
        
        """ Build/Execute the SExtractor call for psf on the detection image"""

        # Sortcuts for less typing
        tiledir  = self.input.tiledir
        tilename_fh = self.input.tilename_fh
        self.logger.info('assembling commands for SEx Dual call')

        # SEx default configuration
        if self.input.SExDual_conf == '':
            self.ctx.SExDual_conf = fh.get_configfile('sex')
            self.logger.info("Will use SEx Dual default configuration file: %s" % self.ctx.SExDual_conf)

        # The updated parameters set for SEx
        pars = self.get_SExDual_parameter_set(**self.input.SExDual_parameters)

        SExDual_cmd = {}
        # Loop over all bands 
        dBAND = self.ctx.detBAND
        for BAND in self.ctx.dBANDS:

            # Spell out input and output names
            # From SEx/psfex
            sexcat  = fh.get_cat_file(tiledir, tilename_fh, BAND)
	    psf_det = fh.get_psf_file(tiledir, tilename_fh, dBAND)
            psf     = fh.get_psf_file(tiledir, tilename_fh, BAND)
            seg     = fh.get_seg_file(tiledir, tilename_fh, BAND)

            # Combined images and weights
            sci_comb     = "%s'[%s]'" % (fh.get_mef_file(tiledir, tilename_fh, BAND),  utils.SCI_HDU)
            sci_comb_det = "%s'[%s]'" % (fh.get_mef_file(tiledir, tilename_fh, dBAND), utils.SCI_HDU)
            wgt_comb     = "%s'[%s]'" % (fh.get_mef_file(tiledir, tilename_fh, BAND),  utils.WGT_HDU)
            wgt_comb_det = "%s'[%s]'" % (fh.get_mef_file(tiledir, tilename_fh, dBAND), utils.WGT_HDU)
            msk_comb     = "%s'[%s]'" % (fh.get_mef_file(tiledir, tilename_fh, BAND),  utils.MSK_HDU)
            
            pars['CATALOG_NAME']    =  sexcat           
            pars['PSF_NAME']        =  "%s,%s" % (psf_det,psf)
            pars['CHECKIMAGE_NAME'] =  seg
            pars['FLAG_IMAGE']      =  msk_comb
            pars['WEIGHT_IMAGE']    =  "%s,%s" % (wgt_comb_det, wgt_comb)
            
            # Build the call for the band, using dband as detection image
            cmd = []
            cmd.append("%s" % SEX_EXE)
            cmd.append("%s,%s" % (sci_comb_det,sci_comb))
            cmd.append("-c %s" % self.ctx.SExDual_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            SExDual_cmd[BAND] = cmd

        return SExDual_cmd

    def cleanup_PSFcats(self,execute=False):
        """ Clean up the *_psfcat.fits files"""

        for BAND in self.ctx.dBANDS:
            psfcat = fh.get_psfcat_file(self.ctx.tiledir, self.ctx.tilename_fh, BAND)
            self.logger.info("Cleaning up %s" % psfcat)
            if execute:
                try:
                    os.remove(psfcat)
                except:
                    self.logger.info("Warning: cannot remove %s" % psfcat)
        return

    def __str__(self):
        return 'Creates the SExtractor call for dual detection'


if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
