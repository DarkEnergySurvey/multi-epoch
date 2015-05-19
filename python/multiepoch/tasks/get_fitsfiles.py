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

from traitlets import Bool, Dict, Unicode
from mojo.jobs import base_job
import multiepoch.utils as utils


class Job(base_job.BaseJob):

    class Input(base_job.IO):

        assoc = Dict(None, help='The dictionary with the file association info.')

        clobber = Bool(False, help='clobber?') 
        local_archive = Unicode('', help='The path to the local des archive.')
        http_section = Unicode('http-desarchive',
                help='The according section in the .desservices.ini file.')


    def run(self):

        # if we have local files, then we'll skip the rest
        if utils.inDESARcluster():
            self.ctx.assoc['FILEPATH_LOCAL'] = self.input.assoc['FILEPATH_ARCHIVE']
            self.logger.info("Inside DESAR cluster, files assumed to be locally available.")
            return

        # Create the list of local names -- gets self.ctx.FILEPATH_LOCAL
        self.define_localnames(self.input.local_archive)

        # Create the directory -- if it doesn't exist.
        utils.create_local_archive(self.input.local_archive)

        # Transfer the files
        self.transfer_files(self.input.clobber, section=self.input.http_section)

    
    def define_localnames(self, local_archive):

        Nfiles = len(self.ctx.assoc['FILEPATH_HTTPS'])
        self.ctx.assoc['FILEPATH_LOCAL'] = []
        for k in range(Nfiles):
            # Get the remote and local names
            url       = self.input.assoc['FILEPATH_HTTPS'][k]
            localfile = os.path.join(local_archive, self.input.assoc['FILEPATH'][k])
            self.ctx.assoc['FILEPATH_LOCAL'].append(localfile)


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
                    
                sys.stdout.write("\r# Getting:  %s (%s/%s)" % (url,k+1,Nfiles))
                sys.stdout.flush()
                # Get a file using the $HOME/.desservices.ini credentials
                http_requests.download_file_des(url,localfile,section)
            else:
                self.logger.debug("\rSkipping: %s (%s/%s) -- file exists" % (url,k+1,Nfiles))

        # Make it a np-char array
        self.ctx.assoc['FILEPATH_LOCAL'] = numpy.array(self.ctx.assoc['FILEPATH_LOCAL'])
            

    def __str__(self):
        return 'Transfer the fits files'



