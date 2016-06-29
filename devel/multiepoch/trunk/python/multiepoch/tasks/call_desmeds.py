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
from despyastro import tableio
import numpy

# Multi-epoch
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
import multiepoch.querylibs as querylibs
from multiepoch import file_handler as fh

# JOB INTERNAL CONFIGURATION
DESMEDS_EXE = 'run_desmeds'
BKLINE = "\\\n"
MAGBASE = 30.0

class Job(BaseJob):

    """
    MEDS call for the multi-epoch pipeline
    """

    class Input(IO):

        """
        MES files creation call call for the multi-epoch pipeline
        """
        ######################
        # Positional Arguments
        # 1. Tilename amd association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })
        assoc_bkg = Dict(None,help="The Dictionary containing the BKG assoc",argparse=False)
        bkg_file  = CUnicode('',help="Input association file with BKG information",input_file=True,
                              argparse={ 'argtype': 'positional', })
        assoc_seg = Dict(None,help="The Dictionary containing the SEG assoc",argparse=False)
        seg_file  = CUnicode('',help="Input association file with BKGinformation",input_file=True,
                              argparse={ 'argtype': 'positional', })
        tilename     = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir     = Unicode(None, help='The output directory for this tile')
        tileid      = CInt(-1,    help="The COADDTILE_ID for the Tile Name")
        magbase     = CFloat(MAGBASE, help="Zero point magnitude base for SWarp, default=%s." % MAGBASE)


        execution_mode_meds  = CUnicode("tofile",help="MEDS excution mode",
                                         argparse={'choices': ('tofile','dryrun','execute')})
        medsconf = CUnicode(help="Optional MEDS configuration file (yaml)")
        MP_meds  = CInt(1,help="run using multi-process, 0=automatic, 1=single-process [default]")

        doBANDS  = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        # Function to read ASCII/panda framework file (instead of json)
        def _read_assoc_file(self):
            mydict = {}
            df = pd.read_csv(self.assoc_file,sep=' ')
            mydict['assoc'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _read_bkg_file(self):
            mydict = {}
            df = pd.read_csv(self.bkg_file,sep=' ')
            mydict['assoc_bkg'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _read_seg_file(self):
            mydict = {}
            df = pd.read_csv(self.seg_file,sep=' ')
            mydict['assoc_seg'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

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

        execution_mode = self.input.execution_mode_meds

        # 0. Prepare the context
        self.prewash()

        # 1. Get the configuration file
        if self.ctx.medsconf == '':
            self.ctx.medsconf = fh.get_configfile('meds-desdm-Y3A1v1',ext='yaml')
            self.logger.info("Will use MEDS default configuration file: %s" % self.ctx.medsconf)

        # 2. Write the lists
        self.write_meds_input_list_files()

        # 4. Get the command list
        cmd_list = self.get_meds_cmd_list()

        # 5. execute cmd_list according to execution_mode 
        if execution_mode == 'tofile':
            self.writeCall(cmd_list)

        elif execution_mode == 'dryrun':
            self.logger.info("For now we only print the commands (dry-run)")
            [self.logger.info(' '.join(cmd_list[band])) for band in self.ctx.doBANDS]

        elif execution_mode == 'execute':
            MP = self.ctx.MP_meds # MP or single Processs
            self.runMEDS(cmd_list,MP=MP)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)

        return


    def write_meds_input_list_files(self):

        """ Write the input file list for MEDS"""
        self.logger.info('Writing MEDS input files')

        for BAND in self.ctx.doBANDS:
            
            idx = numpy.where(self.ctx.assoc['BAND'] == BAND)[0]
            nwg_inputs  = self.ctx.assoc['FILEPATH_LOCAL'][idx]
            bkg_inputs  = self.ctx.assoc_bkg['FILEPATH_LOCAL'][idx]
            seg_inputs  = self.ctx.assoc_seg['FILEPATH_LOCAL'][idx]
            magzero     = self.ctx.assoc['MAG_ZERO'][idx]
            
            # Now let's sort them by filename
            isort = numpy.argsort(nwg_inputs)
            nwg_inputs = nwg_inputs[isort]
            bkg_inputs = bkg_inputs[isort]
            seg_inputs = seg_inputs[isort]
            magzero    = magzero[isort]
            
            # writing the lists to files using tableio.put_data()
            tableio.put_data(fh.get_meds_list_nwg(self.ctx.tiledir, self.ctx.tilename_fh, BAND),(nwg_inputs,magzero),  format='%s %s')
            tableio.put_data(fh.get_meds_list_seg(self.ctx.tiledir, self.ctx.tilename_fh, BAND),(seg_inputs,),  format='%s')
            tableio.put_data(fh.get_meds_list_bkg(self.ctx.tiledir, self.ctx.tilename_fh, BAND),(bkg_inputs,),  format='%s')

        return

    def get_meds_cmd_list(self):

        """
        Get the meds command list

        Example call:

        run_desmeds
        --band r \
        --coadd_cat   coadd/DES2246-4457_r_cat.fits \
        --coadd_image coadd/DES2246-4457_r.fits \
        --coadd_seg   coadd/DES2246-4457_r_seg.fits \
        --nwg_flist   list/DES2246-4457_r_ngwint.list \
        --seg_flist   list/DES2246-4457_r_seg.list \
        --bkg_flist   list/DES2246-4457_r_bkg.list \
        --meds_output DES2246-4457-r-meds-Y3A1.fits \
        --tileconf    DES2246-4457-r-fileconf.yml\
        --medsconf    meds-desdm-Y3A1.yaml\
        --dryrun
        """

        tiledir     = self.input.tiledir
        tileid      = self.input.tileid
        tilename    = self.input.tilename
        tilename_fh = self.input.tilename_fh

        self.logger.info("assembling commands for meds call")

        cmd_list = {}
        for BAND in self.ctx.doBANDS:

            cmd = []
            cmd.append("%s" % DESMEDS_EXE)
            cmd.append("--band %s" % BAND)
            cmd.append("--coadd_cat %s"    % fh.get_cat_file(tiledir, tilename_fh, BAND))
            cmd.append("--coadd_image %s"  % fh.get_mef_file(tiledir, tilename_fh, BAND))
            cmd.append("--coadd_seg %s"    % fh.get_seg_file(tiledir, tilename_fh, BAND))
            cmd.append("--coadd_magzp %s"  % self.ctx.magbase)
            cmd.append("--coadd_image_id %s" %  self.ctx.tileid) ## REVISE!!!!
            cmd.append("--nwg_flist %s" % fh.get_meds_list_nwg(tiledir, tilename_fh, BAND))
            cmd.append("--seg_flist %s"    % fh.get_meds_list_seg(tiledir, tilename_fh, BAND))
            cmd.append("--bkg_flist %s"    % fh.get_meds_list_bkg(tiledir, tilename_fh, BAND))
            cmd.append("--meds_output %s"  % fh.get_meds_output(tiledir, tilename_fh, BAND))
            cmd.append("--tileconf %s"     % fh.get_meds_tileconf(tiledir, tilename_fh, BAND))
            cmd.append("--medsconf %s"     % self.ctx.medsconf)
            cmd_list[BAND] = cmd

        return cmd_list

    def writeCall(self,cmd_list,mode='w'):

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_meds_cmd_file(self.input.tiledir, self.input.tilename_fh)
        self.logger.info("Will write MEDS call to: %s" % cmdfile)
        with open(cmdfile, mode) as fid:
            for band in self.ctx.doBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n\n')
        return

    def runMEDS(self,cmd_list,MP):

        self.logger.info("Will proceed to run the MEDS call now:")
        t0 = time.time()
        NP = utils.get_NP(MP) # Figure out NP to use, 0=automatic
        
        # Case A -- NP=1
        if NP == 1:
            logfile = fh.get_meds_log_file(self.input.tiledir, self.input.tilename_fh)
            log = open(logfile,"w")
            self.logger.info("Will write MEDS to logfile: %s" % logfile)

            for band in self.ctx.doBANDS:
                t1 = time.time()
                cmd  = ' '.join(cmd_list[band])
                self.logger.info("Executing MEDS for BAND:%s" % band)
                self.logger.info("%s " % cmd)
                status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
                if status != 0:
                    raise RuntimeError("\n***\nERROR while running MEDS, check logfile: %s\n***" % logfile)
                self.logger.info("Done band %s in %s" % (band,elapsed_time(t1)))
            
        # Case B -- multi-process in case NP > 1
        else:
            self.logger.info("Will Use %s processors" % NP)
            cmds = []
            logs = []
            for band in self.ctx.doBANDS:
                cmds.append(' '.join(cmd_list[band]))
                logfile = fh.get_meds_log_file(self.input.tiledir, self.input.tilename_fh,band)
                logs.append(logfile)
                self.logger.info("Will write to logfile: %s" % logfile)

            sys.stdout.flush()
            pool = multiprocessing.Pool(processes=NP)
            pool.map(utils.work_subprocess_logging, zip(cmds,logs))

        self.logger.info("Total MEDS time %s" % elapsed_time(t0))
        return


    def __str__(self):
        return 'Creates the call to MEDS on multiepoch pipeline'

if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)

