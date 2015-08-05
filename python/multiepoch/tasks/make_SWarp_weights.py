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
        """ Create Custom inputs Weights for SWarp
        """
    
        # Required inputs to run the job (in ctx, after loading files)
        # because we set the argparse keyword to False they are not interfaced
        # to the command line parser
        assoc = Dict(None,help="The Dictionary containing the association information.",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })

        # TODO : see below
        # MICHAEL: This does not work
        #def _read_assoc_file(self):
        #
        #    print "# Hello"
        #    print self.input.assoc_file
        #    my_dict = read_ascii_to_dict(self.input.assoc_file)
        #    print "Done"
        #    return mydict

        # Optional inputs -- postional arguments
        clobber_weights  = Bool(False, help="Cloober the existing custom weight files.")
        weight_extension = CUnicode('_wgt', help=("Weight extension to add to custom weight file names."))
        MP_weight        = CInt(1, help = ("Run using multi-process, 0=automatic, 1=single-process [default]"))
        
        local_archive        = CUnicode('', help=("The local filepath where the input fits files (will) live"))
        local_weight_archive = CUnicode('', help='The path to the weights archive.')
        weights_execution_mode  = CUnicode("dryrun",help="Weights excution mode",
                                           argparse={'choices': ('tofile','dryrun','execute')})
        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        def _validate_conditional(self):
            pass


    def run(self):

        # 1. Define weight names using function in contextDefs
        self.logger.info("Constructing assoc[FILEPATH_LOCAL_WTG] from assoc[FILEPATH_LOCAL]")
        self.ctx.assoc['FILEPATH_LOCAL_WGT'] = contextDefs.define_weight_names(self.ctx)

        # now make sure all paths exist
        for wgt_path in self.ctx.assoc['FILEPATH_LOCAL_WGT']:
            if not os.path.exists(os.path.split(wgt_path)[0]):
                os.makedirs(os.path.split(wgt_path)[0])

        # 2. FIGURE OUT WHICH WEIGHT FILES NEED TO BE CREATED
        to_create = []
        for idx, weight_file in enumerate(self.ctx.assoc['FILEPATH_LOCAL_WGT']):
            if os.path.exists(weight_file) and not self.input.clobber_weights:
                self.logger.debug('Skipping creation of %s, exists already.' % weight_file)
            else:
                t = (self.ctx.assoc['FILEPATH_LOCAL'][idx],weight_file,self.input.clobber_weights)
                to_create.append(t)

        # 3. CREATE THE NECESSARY WEIGHT FILES
        self.create_weights_for_SWarp(to_create, MP=self.input.MP_weight)

        # we prefer it to be a numpy array
        self.ctx.assoc['FILEPATH_LOCAL_WGT'] = numpy.array(self.ctx.assoc['FILEPATH_LOCAL_WGT'])


    def create_weights_for_SWarp(self, to_create, MP=1):
        """ Create the custom weight files
        """
        execution_mode = self.input.weights_execution_mode
        
        # Figure out NP to use, 0=automatic
        NP = utils.get_NP(MP) 

        # Get ready to run if applicable
        N = len(to_create)
        if N > 0 and NP!=1 and execution_mode == 'execute':
            self.logger.info("Will create weights multi-process using %s processor(s)" % NP)
            pool = multiprocessing.Pool(processes=NP)
            pool.map(modify_weight, to_create)
            
        elif N > 0 and execution_mode == 'execute':
            self.logger.info("Will create weights single-process")
            for k in range(N):
                self.logger.info("Making Weight:  %s (%s/%s)" % (to_create[k][1],k+1,N))
                modify_weight(to_create[k])
        else:
            self.logger.info("No Weights to be created")


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
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
