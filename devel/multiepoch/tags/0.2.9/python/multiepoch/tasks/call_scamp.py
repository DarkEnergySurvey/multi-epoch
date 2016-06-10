#!/usr/bin/env python

from despyastro import tableio
import os
import sys
import numpy
import time
import pandas as pd
import subprocess


from despymisc.miscutils import elapsed_time
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh
import despyfitsutils.fitsutils as fitsutils

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.utils import log as mojo_log

# JOB INTERNAL CONFIGURATION
SCAMP_EXE = 'scamp'
DETNAME = 'det'
BKLINE = " \\\n"
class Job(BaseJob):

    class Input(IO):

        """ Scamp preparation call for the multiepoch pipeline"""

        ######################
        # Required inputs
        # Super-alignment options
        catlist      = Dict(None,help="The Dictionary containing input CCD-level catalog list ",argparse=False)
        cats_file    = CUnicode('',help="Name of the output ASCII catalog list storing the information for scamp", input_file=True,
                                argparse={ 'argtype': 'positional', })

        scampcats_file  = CUnicode('',help="Name of the output ASCII catalog list storing the information for scampcats", input_file=True)
        scampcatlist    = Dict({},help="The Dictionary containing scampcat catalog list ",argparse=False)

        scampheads_file = CUnicode('',help="Name of the output ASCII catalog list storing the information for scampheads", input_file=True)
        scampheadlist   = Dict({},help="The Dictionary containing scamphead catalog list ",argparse=False)


        ######################
        # Optional arguments
        tilename    = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir     = Unicode(None, help="The output directory for this tile")

        doBANDS  = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})
        detname  = CUnicode(DETNAME,help="File label for detection image, default=%s." % DETNAME)
        nthreads = CInt(1,help="Number of threads to use in stiff/psfex/swarp/scamp")
        local_archive = CUnicode("", help="The local filepath where the input fits files (will) live")

        execution_mode_scamp  = CUnicode("tofile",help="SWarp excution mode",
                                         argparse={'choices': ('tofile','dryrun','execute')})
        scamp_parameters = Dict({},help="A list of parameters to pass to scamp",
                                argparse={'nargs':'+',})
        scamp_conf = CUnicode(help="Optional scamp configuration file")

        # Use scampcats
        use_scampcats = Bool(False, help=("Use finalcut scampcats for super-alignment[Default=False]"))

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        
        def _read_cats_file(self):
            mydict = {}
            df = pd.read_csv(self.cats_file,sep=' ')
            mydict['catlist'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _read_scampcats_file(self):
            mydict = {}
            df = pd.read_csv(self.cats_file,sep=' ')
            mydict['scampcatlist'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _read_scampheads_file(self):
            mydict = {}
            df = pd.read_csv(self.cats_file,sep=' ')
            mydict['scampheadlist'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):

            # Get logger
            logger = mojo_log.get_logger({})
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

            # Check for valid input scampcats_file
            if self.mojo_execution_mode == 'job as script' and self.use_scampcats and self.scampcats_file == "":
                mess = 'If job is run standalone and use_scampcats=True, then scampcats_file cannot be ""'
                raise IO_ValidationError(mess)

            # Check for valid input scampheads_file
            if self.mojo_execution_mode == 'job as script' and self.use_scampcats and self.scampheads_file == "":
                mess = 'If job is run standalone and use_scampcats=True, then scamphead_file cannot be ""'
                raise IO_ValidationError(mess)
                

        # To also accept comma-separeted input lists
        def _argparse_postproc_doBANDS(self, v):
            return utils.parse_comma_separated_list(v)

    def prewash_scamp(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""

        # Re-cast the ctx.assoc/ctx.catlist as dictionary of arrays instead of lists
        self.ctx.catlist       = utils.dict2arrays(self.ctx.catlist)
        if self.ctx.use_scampcats:
            self.ctx.scampcatlist  = utils.dict2arrays(self.ctx.scampcatlist)
            self.ctx.scampheadlist = utils.dict2arrays(self.ctx.scampheadlist)

        # Get the BANDs information in the context if they are not present
        if self.ctx.get('gotBANDS'):
            self.logger.info("BANDs already defined in context -- skipping")
        else:
            self.ctx.update(contextDefs.get_BANDS(self.ctx.catlist, detname=self.ctx.detname,logger=self.logger,doBANDS=self.ctx.doBANDS))

        # Check info OK
        self.logger.info("BANDS:   %s" % self.ctx.BANDS)
        self.logger.info("doBANDS: %s" % self.ctx.doBANDS)
        self.logger.info("dBANDS:  %s" % self.ctx.dBANDS)
        
    def run(self):
        
        execution_mode = self.input.execution_mode_scamp

        # 0. Prepare the context
        self.prewash_scamp()

        # 1. Get the unitnames for the doBANDS selection
        self.ctx.unitnames = contextDefs.get_scamp_unitnames(self.ctx)

        # 2. Write the scamp input list of expcats
        self.write_scamp_input_list_file()

        # Soft link the scampcat file (fits and head)
        if self.ctx.use_scampcats:
            self.copy_scampcat_files()
        
        # 3. Get the list of command lines
        cmdlist = self.get_scamp_cmd_list()
        
        # 3. Execute a cmdlist according to execution_mode 
        if execution_mode == 'tofile':
            self.writeCall_scamp(cmdlist)
        elif execution_mode == 'dryrun':
            [self.logger.info(' '.join(cmdlist[key])) for key in cmdlist.keys()]
        elif execution_mode == 'execute':
            self.run_scamp(cmdlist)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)
        return


    # Create the input list for scamp
    def write_scamp_input_list_file(self):
        """
        Write the input file list for the scamp call
        """
        # Sortcuts for less typing
        tiledir     = self.input.tiledir
        tilename_fh = self.input.tilename_fh
        self.logger.info('Writing scamp input file list')
        expcats = []
        for UNITNAME in self.ctx.unitnames:
            expcats.append(fh.get_expcat_file(tiledir, tilename_fh, UNITNAME))
        tableio.put_data(fh.get_expcat_list_file(tiledir, tilename_fh),(expcats,),format="%s")
        return

    # Copy and rename the scampcat fits and head files
    def copy_scampcat_files(self,clobber=True):
        """
        Copy/rename/soft links the scampcat input files to the align directory
        """
        # Sortcuts for less typing
        tiledir     = self.input.tiledir
        tilename_fh = self.input.tilename_fh
        self.logger.info('Copying scampcat input file list to input folder')
        for UNITNAME in self.ctx.unitnames:

            # Scampcat fits catalogs
            expcat_name   = fh.get_expcat_file(tiledir, tilename_fh, UNITNAME)
            scampcat_name = self.ctx.scampcatlist['FILEPATH_LOCAL'][self.ctx.scampcatlist['UNITNAME'] == UNITNAME][0]
            self.logger.debug("Linking:%s --> %s" % (scampcat_name,expcat_name))
            utils.symlink_force(scampcat_name,expcat_name)

            # Scampcat head catalogs --> ahead catalogs
            exphead_name   = fh.get_expahead_file(tiledir, tilename_fh, UNITNAME)
            scamphead_name = self.ctx.scampheadlist['FILEPATH_LOCAL'][self.ctx.scampheadlist['UNITNAME'] == UNITNAME][0]
            self.logger.debug("Linking:%s --> %s" % (scamphead_name,exphead_name))
            utils.symlink_force(scamphead_name,exphead_name)
            
        return


    # Run scamp
    # -------------------------------------------------------------------------
    def run_scamp(self,cmd_list):
        """
        Run scamp
        """

        if self.ctx.use_scampcats:
            logfile = fh.get_scamp_log_file(self.input.tiledir, self.input.tilename_fh,suffix='scamp_scampcats')
        else:
            logfile = fh.get_scamp_log_file(self.input.tiledir, self.input.tilename_fh,suffix='scamp')

        log = open(logfile,"w")
        self.logger.info("Will proceed to run scamp now:")
        self.logger.info("Will write to logfile: %s" % logfile)
        t0 = time.time()

        current_cwd = os.getcwd()
        self.logger.info("Moving to the TILEDIR directory for execution")
        os.chdir(self.ctx.tiledir)
        

        cmd  = ' '.join(cmd_list)
        self.logger.info("Executing scamp super-alignment for tile:%s" % self.ctx.tilename_fh)
        self.logger.info("%s " % cmd)
        status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
        # Move back to current dir
        self.logger.info("Moving back to current dir")
        os.chdir(current_cwd)
        if status != 0:
            raise RuntimeError("\n***\nERROR while running scamp, check logfile: %s\n***" % logfile)
        self.logger.info("Total scamp time: %s" % elapsed_time(t0))
        log.write("Total scamp time: %s\n" % elapsed_time(t0))
        log.close()
        return

    # Assemble the command for finalcut catalogs
    # -------------------------------------------------------------------------
    def get_scamp_cmd_list(self):
        """
        Build the scamp call for a given tile.
        """
        # Sortcuts for less typing
        tiledir     = self.input.tiledir
        tilename_fh = self.input.tilename_fh

        self.logger.info('Assembling commands for scamp call')
        # Update and Set the SWarp options 
        pars = self.get_scamp_parameter_set(**self.input.scamp_parameters)
        pars["XML_NAME"] = "%s" % fh.get_scamp_xml_file(tiledir, tilename_fh)

        if self.ctx.use_scampcats:
            pars["AHEADER_SUFFIX"] = ".ahead"
        
        # The Scamp configuration file
        if self.input.scamp_conf == '':
            self.ctx.scamp_conf = fh.get_configfile('scamp')
            self.logger.info("Will use scamp default configuration file: %s" % self.ctx.scamp_conf)

        cmd = []
        cmd.append(SCAMP_EXE)
        cmd.append("@%s" % fh.get_expcat_list_file(tiledir, tilename_fh))
        cmd.append("-c %s" % self.ctx.scamp_conf)
        for param,value in pars.items():
            cmd.append("-%s %s" % (param,value))
        return cmd


    def get_scamp_parameter_set(self, **kwargs):
        """
        Set the scamp default options for all band in a tile and overwrite them
        with kwargs to this function.
        """
        # older way, most plots
        #checkplot_type = "astr_pixerror1d,astr_referror1d,fgroups,astr_referror2d,astr_refsysmap,distortion"
        # Only general ones
        checkplot_type = "astr_referror1d,astr_referror2d,fgroups"
        checkplot_name = ",".join( fh.get_scamp_plots_relative(self.input.tilename_fh, checkplot_type.split(',')))
        scamp_parameters = {
            "CHECKPLOT_TYPE"  : checkplot_type,
            "CHECKPLOT_NAME"  : checkplot_name,
            "NTHREADS"        : self.ctx.nthreads,
            }
        scamp_parameters.update(kwargs)
        return scamp_parameters


    # 'EXECUTION' FUNCTIONS
    # -------------------------------------------------------------------------
    def writeCall_scamp(self,cmd_list,mode='w'):

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_scamp_cmd_file(self.input.tiledir, self.input.tilename_fh)

        self.logger.info("Will write combine_cats call to: %s" % cmdfile)
        with open(cmdfile, mode) as fid:
            fid.write("#!/usr/bin/env bash\n\n")
            fid.write('# Save current pwd\n')
            fid.write("current=`pwd`\n")
            fid.write("cd %s\n" % self.ctx.tiledir)
            fid.write(bkline.join(cmd_list)+'\n')
            fid.write('\n\n')
            fid.write('# Get back to prev directory\n')
            fid.write('cd $current\n')
            fid.write('\n\n')

        os.chmod(cmdfile, 0755)
        return

    # -------------------------------------------------------------------------

    def __str__(self):
        return 'Creates the call to scamp'


if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)



