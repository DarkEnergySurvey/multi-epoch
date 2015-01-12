"""

Create the custom weights, with interpolated weight values for the SWarp input weights

INPUTS:

 - self.ctx.FILEPATH_LOCAL (if in cosmology cluster self.ctx.FILEPATH_LOCAL =  self.ctx.FILEPATH_ARCHIVE)
 - clobber_weights       : Defined local clobber (to self.ctx.clobber_weights)
 - MP_weight             : Run the process in MP

OUTPUTS:
  - self.ctx.FILEPATH_LOCAL_WGT (contains the weight for SWarp science combination)

"""

from mojo.jobs.base_job import BaseJob
import os,sys
from multiepoch.DESfits import DESFITS
from despymisc.miscutils import elapsed_time
import numpy
import time
import multiprocessing

class Job(BaseJob):

    def __call__(self):

        t0 = time.time()

        # Get all of the relevant kwargs
        kwargs = self.ctx.get_kwargs_dict()
        clobber = kwargs.get('clobber_weights', False)
        wgt_ext = kwargs.get('weight_extension','_wgt')
        MP      = kwargs.get('MP_weight',False)

        # Create the directory -- if it doesn't exist.
        create_local_archive(self.ctx.local_archive)

        # Create the weights
        self.create_weights_for_SWarp(clobber,wgt_ext, MP)
        print "# Weights created on: %s" % elapsed_time(t0)
        return

    def create_weights_for_SWarp(self,clobber,wgt_ext, MP):

        """ Transfer the files """

        local_archive = self.ctx.local_archive

        # Now get the files via http
        Nfiles = len(self.ctx.FILEPATH_LOCAL)
        self.ctx.FILEPATH_LOCAL_WGT = []

        args = []
        for k in range(Nfiles):
            
            # Define the local filename
            basename  = self.ctx.FILEPATH[k].split(".fits")[0]
            extension = self.ctx.FILEPATH[k].split(".fits")[1:]
            local_wgtfile = os.path.join(local_archive,"%s%s.fits" % (basename,wgt_ext))
            self.ctx.FILEPATH_LOCAL_WGT.append(local_wgtfile)

            # Make sure the file does not already exists exits
            if not os.path.exists(local_wgtfile) or clobber:
                
                dirname   = os.path.dirname(local_wgtfile)
                if not os.path.exists(dirname):
                    os.makedirs(dirname)
                # Create the inputs for later
                args.append((self.ctx.FILEPATH_LOCAL[k],self.ctx.FILEPATH_LOCAL_WGT[k],clobber))
            else:
                sys.stdout.write("\r# Skipping: %s (%s/%s) -- file exists" % (local_wgtfile,k+1,Nfiles))
                sys.stdout.flush()

        # Get ready to run if applicable
        N = len(args)
        if N > 0 and MP:
            NP = get_NP(MP)

            print "# Will create weights multi-process using %s processor" % NP
            pool = multiprocessing.Pool(processes=NP)
            pool.map(modify_weight, args)
            
        elif N > 0:
            print "# Will create weights single-process"
            for k in range(N):
                sys.stdout.write("\r# Making Weight:  %s (%s/%s)\n" % (args[k][1],k+1,N))
                modify_weight(args[k])
        else:
            print "# No Weights to be created"

        print "\n#\n"

        # Pass them up as array as an np-char array
        self.ctx.FILEPATH_LOCAL_WGT = numpy.array(self.ctx.FILEPATH_LOCAL_WGT)
        return
        


    def __str__(self):
        return 'Create Custom inputs Weights for SWarp'


def get_NP(MP):

    if type(MP) is bool:
        NP = multiprocessing.cpu_count()
    elif type(MP) is int:
        NP = MP
    else:
        raise ValueError('MP is wrong type: %s, must be bool or integer type' % MP)
    return NP


def modify_weight(args):

    fileName,outName,clobber = args

    """
    Simple call to modify weight image, based on a bitmask that
    can be called from multiprocess
    """

    # Get the start time
    t0 = time.time()
    desfits = DESFITS(fileName,outName,clobber=clobber)
    # Replace bleed trails 2^6 = 64
    desfits.replace_weightBit(maskbit=64)
    desfits.write_weight()
    print "# Done in %s\n" % elapsed_time(t0)
    return

def create_local_archive(local_archive):
    """ Creates the local cache for the desar archive """
    if not os.path.exists(local_archive):
        print "# Will create LOCAL ARCHIVE at %s" % local_archive
        os.mkdir(local_archive)
    return

