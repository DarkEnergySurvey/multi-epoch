#!/usr/bin/env python

"""

If in a remote machine, retrieve the fits file needed for coaddition using https

INPUTS:

 - local_archive : Place where we will put the files (i.e.: $HOME/LOCAL_DESAR)
 - clobber       : Defined local clobber (to self.ctx.clobber_local)

 - self.ctx.FILEPATH : The relative path of the fits files
 - self.ctx.FILEPATH_ARCHIVE : The full path in the cosmology archive
 - self.ctx.FILEPATH_HTTPS:    The full URL for download

OUTPUTS:

 - self.ctx.FILEPATH_LOCAL (if in cosmology cluster self.ctx.FILEPATH_LOCAL =  self.ctx.FILEPATH_ARCHIVE)

"""

import os
import sys
import re
import numpy
from despymisc import http_requests
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs


# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider


#from traitlets import Bool, Dict, Unicode
#from mojo.jobs import base_job


class Job(BaseJob):

    class Input(IO):

        assoc = Dict(None,help="The Dictionary containing the association information.",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })

        clobber = Bool(False, help='clobber?') 
        local_archive = Unicode('', help='The path to the local des archive.')
        archive_name  = CUnicode("prodbeta",help="DataBase Archive Name section",
                                 argparse={'choices': ('prodbeta','desar2home')} )
        http_section = Unicode('http-desarchive',help='The corresponding section in the .desservices.ini file.')
        db_section   = CUnicode("db-destest",help="DataBase Section to connect", 
                                argparse={'choices': ('db-desoper','db-destest', )} )

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )


        def _validate_conditional(self):

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
            self.ctx.assoc['FILEPATH_HTTPS'] = contextDefs.define_https_names(self.ctx,logger=self.logger)

        # Create the directory -- if it doesn't exist.
        utils.create_local_archive(self.input.local_archive)

        # Transfer the files
        self.transfer_files(self.input.clobber, section=self.input.http_section)

    
    def transfer_files(self, clobber, section):
        """ Transfer the files """

        # Now get the files via http
        Nfiles = len(self.input.assoc['FILEPATH_HTTPS'])
        for k in range(Nfiles):
            
            url       = self.input.assoc['FILEPATH_HTTPS'][k]
            localfile = self.ctx.assoc['FILEPATH_LOCAL'][k]

            # Make sure the file does not already exists exits
            if not os.path.exists(localfile) or clobber:
                
                dirname   = os.path.dirname(localfile)
                if not os.path.exists(dirname):
                    os.makedirs(dirname)
                    
                self.logger.info("Getting:  %s (%s/%s)" % (url,k+1,Nfiles))
                print localfile
                sys.stdout.flush()
                # Get a file using the $HOME/.desservices.ini credentials
                http_requests.download_file_des(url,localfile,section)
            else:
                self.logger.info("Skipping: %s (%s/%s) -- file exists" % (url,k+1,Nfiles))

        # Make it a np-char array
        self.ctx.assoc['FILEPATH_LOCAL'] = numpy.array(self.ctx.assoc['FILEPATH_LOCAL'])
            

    def __str__(self):
        return 'Transfer the fits files'



if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
