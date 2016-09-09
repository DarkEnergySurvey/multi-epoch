#!/usr/bin/env python

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

from mojo.jobs.base_job import BaseJob
import os,sys
import subprocess

from despymisc.miscutils import elapsed_time
import time
import pandas as pd

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh
from despyfits import coadd_assemble 

DETNAME = 'det'
XCORRNAME = 'xcorr'
EXTINCTION_EXE = 'compute_SFD98_coadd_catalog'
BKLINE = "\\\n"

class Job(BaseJob):

    """
    Make the call for the extinction correction
    """

    class Input(IO):
        
        """
        Make the call for the extinction correction
        """

        #####################
        # Required arguments 
        tilename     = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh  = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir      = Unicode(None, help="The output directory for this tile")
        execution_mode_extinction  = CUnicode("tofile",help="excution mode",
                                              argparse={'choices': ('tofile','dryrun','execute')})
        detname       = CUnicode(DETNAME,help="File label for detection image, default=%s." % DETNAME)
        xcorrname     = CUnicode(XCORRNAME,help="File label for xcorr, default=%s." % XCORRNAME)
        columns       = CUnicode("NUMBER,ALPHAWIN_J2000,DELTAWIN_J2000",
                                 help="The name of the 3 columns used separated by comas, \
                                 e.g.: NUMBER,RA,DEC, default=NUMBER,ALPHAWIN_J2000,DELTAWIN_J2000")
        
        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        def _validate_conditional(self):
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

    def run(self):

        t0 = time.time()


        # check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.execution_mode_extinction

        bkline  = self.ctx.get('breakline',BKLINE)

        # Get the cmd
        cmd_list = self.get_extinction_cmd()

        if execution_mode == 'execute':
            logfile = fh.get_extinction_log_file(self.input.tiledir, self.input.tilename_fh)
            log = open(logfile,"w")
            self.logger.info("Will proceed to run the extinction call now:")
            self.logger.info("Will write to logfile: %s" % logfile)
            t0 = time.time()
            cmd  = ' '.join(cmd_list)
            self.logger.info("Executing extinction for tile:%s " % self.input.tilename_fh)
            self.logger.info("%s " % cmd)
            status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            if status != 0:
                raise ValueError(" ERROR while running Extinction, check logfile: %s " % logfile)
            self.logger.info("Total extinction time %s" % elapsed_time(t0))

        elif execution_mode == 'dryrun':
            self.logger.info("For now we only print the commands (dry-run)")
            self.logger.info(' '.join(cmd_list))
                
        elif execution_mode == 'tofile':
            bkline  = self.ctx.get('breakline',BKLINE)
            # The file where we'll write the commands
            cmdfile = fh.get_extinction_cmd_file(self.input.tiledir, self.input.tilename_fh)
            self.logger.info("Will write extinction call to: %s" % cmdfile)
            with open(cmdfile, 'w') as fid:
                fid.write(bkline.join(cmd_list)+'\n')
                fid.write('\n')
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)
            
        self.logger.info("Extinction Correction Total time: %s" % elapsed_time(t0))
        return

    def get_extinction_cmd(self):

        """ Get the extinction call command"""
        
        cmd = []
        cmd.append(EXTINCTION_EXE)
        cmd.append("-i %s" % fh.get_cat_file(self.ctx.tiledir, self.ctx.tilename_fh, self.ctx.detname))
        cmd.append("-o %s" % fh.get_mef_file(self.ctx.tiledir, self.ctx.tilename_fh, self.ctx.xcorrname))
        cmd.append("--columns %s" % self.ctx.columns)
        return cmd

    def __str__(self):
        return 'Make the call for the extinction correction'
 
if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)




