#!/usr/bin/env python

"""

If in a remote machine, retrieve the fits file needed for coaddition using https

INPUTS:

 - local_archive : Place where we will put the files (i.e.: $HOME/LOCAL_DESAR)
 - clobber_inputs : Defined local clobber (to self.ctx.clobber_local)
 - self.ctx.FILEPATH : The relative path of the fits files
 - self.ctx.FILEPATH_ARCHIVE : The full path in the cosmology archive
 - self.ctx.FILEPATH_HTTPS:    The full URL for download

"""

import os
import sys
import re
import numpy
import pandas as pd

from despymisc import http_requests
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider
from mojo.utils import log as mojo_log

class Job(BaseJob):

    class Input(IO):

        assoc      = Dict(None,help="The Dictionary containing the association information.",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })

        clobber_inputs = Bool(False, help='clobber input files and retrieve again?') 
        local_archive  = Unicode('', help='The path to the local des archive.')
        archive_name   = CUnicode("prodbeta",help="DataBase Archive Name section",
                                 argparse={'choices': ('prodbeta','desar2home')} )
        http_section = Unicode('http-desarchive',help='The corresponding section in the .desservices.ini file.')
        db_section   = CUnicode("db-destest",help="DataBase Section to connect", 
                                argparse={'choices': ('db-desoper','db-destest', )} )

        # Super-alignment options
        cats_file    = CUnicode('',help="Name of the output ASCII catalog list storing the information for scamp", input_file=True)
        catlist      = Dict({},help="The Dictionary containing input CCD-level catalog list ",argparse=False)
        super_align  = Bool(False, help=("Run super-aligment of tile using scamp"))

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

        def _read_cats_file(self):
            mydict = {}
            df = pd.read_csv(self.cats_file,sep=' ')
            mydict['catlist'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):
            
            # Get logger
            logger = mojo_log.get_logger({})

            if self.cats_file != "" and not self.super_align:
                logger.info("Updating super_align value to True")
                self.super_align = True

            if self.catlist and not self.super_align:
                logger.info("Updating super_align value to True")
                self.super_align = True
            
            # Check for valid local_archive if not in the NCSA cosmology cluster
            if not utils.inDESARcluster() and self.local_archive == '': 
                mess = 'If not in cosmology cluster local_archive canot be empty [""]'
                raise IO_ValidationError(mess)


    def run(self):

        # if we have local files, then we'll skip the rest
        if utils.inDESARcluster():
            self.logger.info("Inside DESAR cluster, files assumed to be locally available.")
            return
        else:
            self.logger.info("Not in DESAR cluster, will try to fetch files to: %s" % self.input.local_archive)

        # Re-construct the names for https location in case not present
        if 'FILEPATH_HTTPS' not in self.ctx.assoc.keys():
            self.logger.info("# Re-consrtuncting FILEPATH_HTTPS to ctx.assoc")
            self.ctx.assoc['FILEPATH_HTTPS'] = contextDefs.define_https_by_name(self.ctx,name='assoc',logger=self.logger)

        if self.ctx.super_align and 'FILEPATH_HTTPS' not in self.ctx.catlist.keys():
            self.logger.info("# Re-consrtuncting FILEPATH_HTTPS to ctx.catlist")
            self.ctx.catlist['FILEPATH_HTTPS'] = contextDefs.define_https_by_name(self.ctx,name='catlist',logger=self.logger)

        # Create the directory -- if it doesn't exist.
        utils.create_local_archive(self.ctx.local_archive)

        # Transfer the files images and catalogs (optional)
        self.transfer_input_files(self.ctx.assoc, clobber=self.ctx.clobber_inputs, section=self.ctx.http_section, logger=self.logger)
        if self.ctx.super_align:
            self.transfer_input_files(self.ctx.catlist, clobber=self.ctx.clobber_inputs, section=self.ctx.http_section, logger=self.logger)

        # Make FILEPATH_LOCAL a np-char array to pass on
        self.ctx.assoc['FILEPATH_LOCAL'] = numpy.array(self.ctx.assoc['FILEPATH_LOCAL'])
        if self.ctx.super_align:
            self.ctx.catlist['FILEPATH_LOCAL'] = numpy.array(self.ctx.catlist['FILEPATH_LOCAL'])

    @staticmethod
    def transfer_input_files(infodict, clobber, section, logger=None):

        """ Transfer the files contained in an info dictionary"""
        
        # Now get the files via http
        Nfiles = len(infodict['FILEPATH_HTTPS'])
        for k in range(Nfiles):
            
            url       = infodict['FILEPATH_HTTPS'][k]
            localfile = infodict['FILEPATH_LOCAL'][k]

            # Make sure the file does not already exists exits
            if not os.path.exists(localfile) or clobber:
                
                dirname   = os.path.dirname(localfile)
                if not os.path.exists(dirname):
                    os.makedirs(dirname)
                    
                logger.info("Getting:  %s (%s/%s)" % (url,k+1,Nfiles))
                sys.stdout.flush()
                # Get a file using the $HOME/.desservices.ini credentials
                http_requests.download_file_des(url,localfile,section)
            else:
                logger.info("Skipping: %s (%s/%s) -- file exists" % (url,k+1,Nfiles))

    def __str__(self):
        return 'Transfer the fits files'

if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
