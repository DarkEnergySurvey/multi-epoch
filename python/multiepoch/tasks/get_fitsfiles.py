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

from mojo.jobs.base_job import BaseJob
import os
import sys
import re
from despymisc  import http_requests
import numpy
import multiepoch.utils as utils

class Job(BaseJob):

    def run(self):

        # Get all of the relevant kwargs
        kwargs = self.ctx.get_kwargs_dict()
        local_archive = kwargs.get('local_archive', None)
        clobber       = kwargs.get('clobber', False)
        http_section  = kwargs.get('http_section', 'http-desarchive')
        
        # Figure out if in the cosmology.illinois.edu cluster
        self.ctx.LOCALFILES = inDESARcluster()

        # if we have local files, then we'll skip the rest
        if self.ctx.LOCALFILES:
            self.ctx.FILEPATH_LOCAL = self.ctx.FILEPATH_ARCHIVE
            print "# All files are local -- inside the DESAR cluster"
            return

        # Create the list of local names -- gets self.ctx.FILEPATH_LOCAL
        self.define_localnames(local_archive)

        # Create the directory -- if it doesn't exist.
        utils.create_local_archive(self.ctx.local_archive)

        # Transfer the files
        self.transfer_files(clobber,local_archive,section=http_section)

        return
    
    def define_localnames(self, local_archive):

        Nfiles = len(self.ctx.FILEPATH_HTTPS)
        self.ctx.FILEPATH_LOCAL = []
        for k in range(Nfiles):
            # Get the remote and local names
            url       = self.ctx.FILEPATH_HTTPS[k]
            localfile = os.path.join(local_archive,self.ctx.FILEPATH[k])
            self.ctx.FILEPATH_LOCAL.append(localfile)
        return

    def transfer_files(self,clobber,section):

        """ Transfer the files """
    
        # Now get the files via http
        Nfiles = len(self.ctx.FILEPATH_HTTPS)
        for k in range(Nfiles):
            
            url       = self.ctx.FILEPATH_HTTPS[k]
            localfile = self.ctx.FILEPATH_LOCAL[k]

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
                sys.stdout.write("\r# Skipping: %s (%s/%s) -- file exists" % (url,k+1,Nfiles))
                sys.stdout.flush()

        # Make it a np-char array
        print "\n#\n"
        self.ctx.FILEPATH_LOCAL = numpy.array(self.ctx.FILEPATH_LOCAL)
        return
        

            

    def __str__(self):
        return 'Transfer the fits files'


def inDESARcluster(domain_name='cosmology.illinois.edu'):

    """ Figure out if we are in the cosmology.illinois.edu cluster """
    
    uname    = os.uname()[0]
    hostname = os.uname()[1]
    mach     = os.uname()[4]
    
    pattern = r"%s$" % domain_name
        
    if re.search(pattern, hostname) and uname == 'Linux':
        LOCAL = True
        print "# Found hostname: %s, running:%s" % (hostname,uname)
        print "# In %s cluster -- will NOT transfer files" % domain_name
    else:
        LOCAL = False
        print "# Found hostname: %s, running:%s" % (hostname,uname)
        print "# NOT in %s cluster -- will try to transfer files" % domain_name

    return LOCAL

#def create_local_archive(local_archive):
#    """ Creates the local cache for the desar archive """
#    if not os.path.exists(local_archive):
#        print "# Will create LOCAL ARCHIVE at %s" % local_archive
#        os.mkdir(local_archive)
#    return
