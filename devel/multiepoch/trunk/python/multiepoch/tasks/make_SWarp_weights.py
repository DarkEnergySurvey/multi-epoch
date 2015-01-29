"""

Create the custom weights, with interpolated weight values for the SWarp input weights

INPUTS:

 REQUIRED:
  - self.ctx.assoc['FILEPATH_LOCAL'] (if in cosmology cluster self.ctx.FILEPATH_LOCAL =  self.ctx.FILEPATH_ARCHIVE)

 OPTIONAL:
  - clobber_weights       : Defined local clobber (to self.ctx.clobber_weights)
  - MP_weight             : Run the process in MP

OUTPUTS:
  - self.ctx.assoc['FILEPATH_LOCAL_WGT'] (contains the weight for SWarp science combination)

  FELIPE: we might want to re-define it later depending on the wgt extension that we decided to use?

"""

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

import os,sys
from despymisc.miscutils import elapsed_time
import numpy
import time
import multiprocessing
# Multi-epoch loads
import multiepoch.utils as utils
from multiepoch.DESfits import DESFITS

class Job(BaseJob):


    class Input(IO):

        """Create Custom inputs Weights for SWarp"""
    
        # Required inputs to run the job (in ctx, after loading files)
        # because we set the argparse keyword to False they are not interfaced
        # to the command line parser
        FILEPATH_LOCAL = List(None,help="The List of images to be processed",
                              argparse=False)

        # Required inputs when run as script
        #
        # variables with keyword input_file=True are loaded into the ctx
        # automatically when intializing the Job class if provided, Input
        # validation happens only thereafter
        # you can implement a implement a custom reader for you input file in
        # your input class, let's say for a variable like
        #
        # my_input_file = CUnicode('',
        #        help='My input file.',
        #        argparse={'argtype': 'positional'})
        #
        # def _read_my_input_file(self):
        #     do the stuff
        #     return a dict

        assoc_file = CUnicode('',help="Input association file with CCDs information",
                # declare this variable as input_file, this leads the content
                # of the file to be loaded into the ctx at initialization
                input_file=True,
                # set argtype=positional !! to make this a required positional
                # argument when using the parser
                argparse={ 'argtype': 'positional', })
                
        # Optional inputs
        clobber_weights  = Bool(False, help="Cloober the existing custom weight files")
        weight_extension = CUnicode('_wgt',help="Weight extension to add to custom weight files")
        MP_weight        = Bool(False,help="run in MP")

    def run(self):

        t0 = time.time()

        # Get the relevant context variables
        clobber = self.ctx.clobber_weights
        wgt_ext = self.ctx.weight_extension
        MP      = self.ctx.MP_weight

        # ONLY if NOT in script mode
        # Create the directory -- if it doesn't exist.
        utils.create_local_archive(self.ctx.local_archive)

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

            # REDO the naming convention based on FILEPATH_LOCAL instead of FILEPATH
            # FILEPATH_LOCAL should be read from the assoc file in case it is run as script
            # FILEPATH_LOCAL is defined in get_fitsfiles, but can be override by the assoc file.
            
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
            NP = utils.get_NP(MP)

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



if __name__ == "__main__":

    # 0. take care of the sys arguments
    import sys
    args = sys.argv[1:]
    # 1. read the input arguments into the job input
    inp = Job.Input()
    # 2. load a context
    ctx = ContextProvider.create_ctx(**inp.parse_arguments(args))
    # 3. set the pipeline execution mode
    ctx['mojo_execution_mode'] = 'job as script'
    # 4. create an empty JobOperator with context
    from mojo import job_operator
    jo = job_operator.JobOperator(**ctx)
    # 5. run the job
    job_instance = jo.run_job(Job)
    # 6. dump the context if json_dump
    if jo.ctx.get('json_dump_file', ''):
        jo.json_dump_ctx()
