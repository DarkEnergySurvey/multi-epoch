#!/usr/bin/env python

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
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

import os,sys
from despymisc.miscutils import elapsed_time
import numpy
import time
import multiprocessing
# Multi-epoch loads
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch.DESfits import DESFITS
npadd = numpy.core.defchararray.add

class Job(BaseJob):


    class Input(IO):

        """Create Custom inputs Weights for SWarp"""
    
        # Required inputs to run the job (in ctx, after loading files)
        # because we set the argparse keyword to False they are not interfaced
        # to the command line parser
        assoc = Dict(None,help="The Dictionary containing the association file",
                     argparse=False)

        assoc_file = CUnicode('',help="Input association file with CCDs information",
                # declare this variable as input_file, this leads the content
                # of the file to be loaded into the ctx at initialization
                input_file=True,
                # set argtype=positional !! to make this a required positional
                # argument when using the parser
                argparse={ 'argtype': 'positional', })

        # MICHAEL: This does not work
        #def _read_assoc_file(self):
        #
        #    print "# Hello"
        #    print self.input.assoc_file
        #    my_dict = read_ascii_to_dict(self.input.assoc_file)
        #    print "Done"
        #    return mydict

        # Optional inputs -- postional arguments
        clobber_weights  = Bool(False, help="Cloober the existing custom weight files")
        weight_extension = CUnicode('_wgt',help="Weight extension to add to custom weight files")
        MP_weight        = CInt(1,help="run using multi-process, 0=automatic, 1=single-process [default]")
        weights_execution_mode  = CUnicode("tofile",help="Weights excution mode",
                                           argparse={'choices': ('tofile','dryrun','execute')})

    def run(self):

        t0 = time.time()

        # Get the relevant context variables
        clobber = self.input.clobber_weights
        wgt_ext = self.input.weight_extension
        MP      = self.input.MP_weight

        # ONLY if NOT in script mode we create the directory -- if it doesn't exist, 
        if self.ctx.mojo_execution_mode != "job as script":
            contextDefs.create_local_archive(self.ctx.local_archive)
        else:
            print "# Skipping local_archive creation: mode:%s" % self.ctx.mojo_execution_mode

        # Create the weights
        self.create_weights_for_SWarp(clobber,wgt_ext, MP)

        print "# Weights created on: %s" % elapsed_time(t0)
        return


    # MODIFY to use utils. get_local_weight_names(filepath_local,wgt_ext) instead
    def set_weight_names_and_args(self,wgt_ext,clobber):

        """ Set up the names for the weights and the args for the
        calls, we need the weight names to be defined when running as
        a library. When run as a script, they will be re-constructed
        from the wgt_ext used.
        """

        Nfiles = len(self.ctx.assoc['FILEPATH_LOCAL'])

        # Figure out if in the cosmology.illinois.edu cluster
        self.ctx.LOCALFILES = utils.inDESARcluster()

        # Get the weight names
        if self.ctx.LOCALFILES:
            filepaths = npadd(self.ctx.local_archive+"/",self.ctx.assoc['FILENAME'])
            self.ctx.assoc['FILEPATH_LOCAL_WGT'] = contextDefs.get_local_weight_names(filepaths,wgt_ext)
        else:
            self.ctx.assoc['FILEPATH_LOCAL_WGT'] = contextDefs.get_local_weight_names(self.ctx.assoc['FILEPATH_LOCAL'],wgt_ext)

        # A shortcut
        filepath_local = self.ctx.assoc['FILEPATH_LOCAL']

        # Define the wgt local filename
        args = []
        for k in range(Nfiles):
            basename  = filepath_local[k].split(".fits")[0] 
            extension = filepath_local[k].split(".fits")[1:]
            local_wgt = self.ctx.assoc['FILEPATH_LOCAL_WGT'][k]
            local_sci = self.ctx.assoc['FILEPATH_LOCAL'][k]
            self.ctx.assoc['FILEPATH_LOCAL_WGT'].append(local_wgt) 

            # Make sure the file does not already exists exits
            if not os.path.exists(local_wgt) or clobber:
                # Create the inputs args for later call
                args.append( (local_sci,local_wgt,clobber))
            else:
                sys.stdout.write("\r# Skipping: %s (%s/%s) -- file exists" % (local_wgt,k+1,Nfiles))
                sys.stdout.flush()
        print "\n#"

        # Pass them up as arrays instead of lists
        self.ctx.assoc['FILEPATH_LOCAL_WGT'] = numpy.array(self.ctx.assoc['FILEPATH_LOCAL_WGT'])
        return args

    def create_weights_for_SWarp(self,clobber,wgt_ext, MP=1):

        """ Run the custom weight files"""

        # Set up the names and get the args for the actual call
        args = self.set_weight_names_and_args(wgt_ext,clobber)

        execute_mode = self.input.weight_execution_mode


        # Figure out NP to use, 0=automatic
        NP = utils.get_NP(MP) 

        # Get ready to run if applicable
        N = len(args)
        if N > 0 and NP!=1 and execute_mode == 'execute':
            print "# Will create weights multi-process using %s processor(s)" % NP
            pool = multiprocessing.Pool(processes=NP)
            pool.map(modify_weight, args)
            
        elif N > 0 and execute_mode == 'execute':
            print "# Will create weights single-process"
            for k in range(N):
                sys.stdout.write("\r# Making Weight:  %s (%s/%s)\n" % (args[k][1],k+1,N))
                modify_weight(args[k])
        else:
            print "# No Weights to be created"

        print "\n#\n"

        ## Pass them up as array as an np-char array
        #self.ctx.assoc['FILEPATH_LOCAL_WGT'] = numpy.array(self.ctx.assoc['FILEPATH_LOCAL_WGT'])
        return
        
    def __str__(self):
        return 'Create Custom inputs Weights for SWarp'



def modify_weight(args):

    """
    Simple call to modify weight image, based on a bitmask that
    can be called from multiprocess
    """
    fileName,outName,clobber = args
    # Get the start time
    t0 = time.time()
    desfits = DESFITS(fileName,outName,clobber=clobber)
    # Replace bleed trails 2^6 = 64
    desfits.replace_weightBit(maskbit=64)
    desfits.write_weight()
    print "# Done in %s\n" % elapsed_time(t0)
    return


def read_ascii_to_dict(filename,sep=' '):

    from despyastro import tableio

    print "# Reading file: %s " % filename

    mydict = {}
    # Get the header
    header = tableio.get_header(filename)
    header = header[1:].strip()
    keys   = header.split(sep)
    # Read filename coluns a tuple of lists of strings
    mytuple = tableio.get_str(filename,cols=range(len(keys)))

    # Repack as a dictionary
    for index, key in enumerate(keys):
        # Convert to float if possible
        try:
            if isinstance(float(mytuple[index][0]), float):
                mydict[key] = map(float,mytuple[index])
        except:
            mydict[key] = mytuple[index]
    return mydict



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
