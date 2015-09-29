#!/usr/bin/env python

"""
Prepare the finalcut input to be used in multi-epoch for coaddition.
"""

# TODO:
# Find a better name than "me-prepare"

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError

import os,sys
import numpy
import time
import multiprocessing
import subprocess

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
import pandas as pd
from multiepoch import file_handler as fh
from despymisc.miscutils import elapsed_time


ROWINTERP_NULLWEIGHT_EXE = 'rowinterp_nullweight_me'
ROWINTERP_NULLWEIGHT_OPTIONS = "--interp_mask TRAIL --invalid_mask EDGE --max_cols 50 --null_mask BADAMP,EDGEBLEED,EDGE,CRAY,SSXTALK,STREAK,TRAIL -v --me_prepare"
BKLINE = "\\\n"

class Job(BaseJob):

    class Input(IO):
        """
        Prepare finalCut single-epoch inputs for co-addition
        """
    
        # Required inputs to run the job (in ctx, after loading files)
        # because we set the argparse keyword to False they are not interfaced
        # to the command line parser
        assoc      = Dict(None,help="The Dictionary containing the association information.",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })
        tilename   = Unicode(None, help="The Name of the Tile Name to query",
                             argparse={ 'argtype': 'positional', })
        # Optional Arguments
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir     = Unicode(None, help='The output directory for this tile.')

        # Optional inputs -- postional arguments
        clobber_me   = Bool(False, help="Cloober the existing me-ready files.")
        extension_me = CUnicode('_me', help=("Weight extension to add to custom weight file names."))
        MP_me        = CInt(1, help = ("Run using multi-process, 0=automatic, 1=single-process [default]"))
        
        local_archive     = Unicode(None, help=("The local filepath where the input fits files (will) live"))
        local_archive_me  = Unicode(None, help=('The path to the me prepared files archive.'))
        me_execution_mode = CUnicode("dryrun",help="me_prepare excution mode",
                                           argparse={'choices': ('tofile','dryrun','execute')})
        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        # TODO:
        # Add method to pass row_interp_nullweight options
        # Copy .head file if available

        # Function to read ASCII/panda framework file (instead of json)
        def _read_assoc_file(self):
            mydict = {}
            df = pd.read_csv(self.assoc_file,sep=' ')
            mydict['assoc'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""
        
        # Define weight names using function in contextDefs
        self.logger.info("Constructing assoc[FILEPATH_LOCAL_ME] from assoc[FILEPATH_LOCAL]")
        self.ctx.assoc['FILEPATH_LOCAL_ME'] = contextDefs.define_me_names(self.ctx)

        # now make sure all paths exist
        for path in self.ctx.assoc['FILEPATH_LOCAL_ME']:
            if not os.path.exists(os.path.split(path)[0]):
                os.makedirs(os.path.split(path)[0])

    def run(self):

        # 0. Prepare the context
        self.prewash()
        
        # 1. Get the comand-line list
        cmd_list = self.get_me_prepare_cmd_list()

        # 2. check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.get('me_execution_mode', 'tofile')

        if execution_mode == 'tofile':
            self.writeCall(cmd_list)
        elif execution_mode == 'dryrun':
            self.logger.info("For now we only print the commands (dry-run)")
            for cmd in cmd_list:
                self.logger.info(' '.join(cmd))
        elif execution_mode == 'execute':
            self.run_me_prepare(cmd_list,MP=self.ctx.MP_me)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)

        # we prefer it to be a numpy array
        self.ctx.assoc['FILEPATH_LOCAL_ME'] = numpy.array(self.ctx.assoc['FILEPATH_LOCAL_ME'])

        return

    def writeCall(self,cmd_list):

        """ Write the rowinterp_nullweights call to a file """

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_me_prepare_cmd_file(self.input.tiledir, self.input.tilename_fh)
        self.logger.info("Will write me_prepare call to: %s" % cmdfile)
        with open(cmdfile, 'w') as fid:
            for cmd in cmd_list:
                fid.write(bkline.join(cmd)+'\n')
                fid.write('\n')
        return

    
    def get_me_prepare_cmd_list(self):
        
        # Figure out which ME files need to be created
        cmd_list = []
        for idx, me_file in enumerate(self.ctx.assoc['FILEPATH_LOCAL_ME']):
            if os.path.exists(me_file) and not self.input.clobber_me:
                self.logger.debug('Skipping creation of %s, exists already.' % me_file)
            else:
                cmd = [ROWINTERP_NULLWEIGHT_EXE]
                cmd.append("-i %s" % self.ctx.assoc['FILEPATH_LOCAL'][idx])
                cmd.append("-o %s" % me_file)
                cmd.append("%s" % ROWINTERP_NULLWEIGHT_OPTIONS)
                cmd_list.append(cmd)

        return cmd_list
        

    def run_me_prepare(self, cmd_list, MP):
        """ Create the me input files"""
        
        self.logger.info("Will proceed to run: %s" % ROWINTERP_NULLWEIGHT_EXE)
        t0 = time.time()
        NP = utils.get_NP(MP)  # Figure out NP to use, 0=automatic
        logfile = fh.get_me_prepare_log_file(self.input.tiledir, self.input.tilename_fh)

        # Get ready to run if applicable
        N = len(cmd_list)

        # Case A -- NP=1
        if NP == 1 and N>0:
            log = open(logfile,"w")
            self.logger.info("Will write to logfile: %s" % logfile)

            # Loop over all input files
            for k,cmd in range(N):
                t1 = time.time()
                cmd  = ' '.join(cmd_list[k])
                self.logger.info("Preparing:  %s (%s/%s)" % (cmd_list[k][1].split()[1],k+1,N))
                status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
                if status > 0:
                    raise RuntimeError("\n***\nERROR while running me_prepare, check logfile: %s\n***" % logfile)
                self.logger.info("Done in %s" % elapsed_time(t1))

        # Case B -- multi-process in case NP > 1
        elif N > 0:

            # Prepare the cmd -- no logging of each call for now.
            cmds = [' '.join(cmd) for cmd in cmd_list]
            self.logger.info("Will Use %s processors" % NP)
            pool = multiprocessing.Pool(processes=NP)
            pool.map(utils.work_subprocess, cmds) 
            # ----------------------------------------------
            # In case we want to run with async
            # pool.map_async(utils.work_subprocess, cmds)
            # pool.close()
            # pool.join()
            # ---------------------------------------------
        else:
            self.logger.info("No me-prepare file to be created")

        self.logger.info("Total me-prepare time %s" % elapsed_time(t0))
        return

    def __str__(self):
        return 'Prepare input for me processing'


if __name__ == "__main__":
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
