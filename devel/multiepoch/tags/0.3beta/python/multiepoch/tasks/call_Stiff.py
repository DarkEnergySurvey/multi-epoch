#!/usr/bin/env python

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

import os
import sys
import subprocess
import time
from despymisc.miscutils import elapsed_time

# Multi-epoch
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs

# JOB INTERNAL CONFIGURATION
STIFF_EXE = 'stiff'
BKLINE = "\\\n"

class Job(BaseJob):


    """ Stiff call for the multi-epoch pipeline
    Inputs:
    - self.ctx.comb_sci
    Outputs:
    - self.ctx.color_tile
    """

    class Input(IO):

        """
        Stiff call for the multi-epoch pipeline
        """
        ######################
        # Required inputs
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",
                          argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",
                              input_file=True,
                              argparse={ 'argtype': 'positional', })

        # Optional Arguments
        basename              = CUnicode("",help="Base Name for coadd fits files in the shape: COADD_BASENAME_$BAND.fits")
        stiff_execution_mode  = CUnicode("tofile",help="Stiff excution mode",
                                         argparse={'choices': ('tofile','dryrun','execute')})
        stiff_parameters      = Dict({},help="A list of parameters to pass to Stiff",
                                     argparse={'nargs':'+',})
        
        def _validate_conditional(self):
            # if in job standalone mode json
            if self.mojo_execution_mode == 'job as script' and self.basename == "":
                mess = 'If job is run standalone basename cannot be ""'
                raise IO_ValidationError(mess)

        def _argparse_postproc_stiff_parameters(self, v):
            return utils.arglist2dict(v, separator='=')


    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""

        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # Get the BANDs information in the context if they are not present
        self.ctx = contextDefs.set_BANDS(self.ctx)
        # Make sure we set up the output dir
        self.ctx = contextDefs.set_tile_directory(self.ctx)
        # 1. Set up names
        # 1a. Get the BANDs information in the context if they are not present
        self.ctx = contextDefs.set_BANDS(self.ctx)
        # 1b. Get the output names for SWarp
        self.ctx = contextDefs.set_SWarp_output_names(self.ctx)
        
    def run(self):

        # 0. Prepare the context
        self.prewash()

        # 1. Set the output name of the color tiff file
        color_tile =  self.ctx.get('color_tile',"%s.tiff" %  self.ctx.basename)

        # 2. get the update stiff parameters  --
        stiff_parameters = self.ctx.get('stiff_parameters', {})
        cmd_list = self.get_stiff_cmd_list(color_tile,stiff_parameters=stiff_parameters)  
        
        # 3. check execution mode and write/print/execute commands accordingly --------------
        execution_mode = self.ctx.stiff_execution_mode

        if execution_mode == 'tofile':
            bkline  = self.ctx.get('breakline',BKLINE)
            # The file where we'll write the commands
            cmdfile = self.ctx.get('cmdfile', "%s_call_stiff.cmd" % self.ctx.basename)
            print "# Will write stiff call to: %s" % cmdfile
            with open(cmdfile, 'w') as fid:
                fid.write(bkline.join(cmd_list)+'\n')
                fid.write('\n')

        elif execution_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            print ' '.join(cmd_list)

        elif execution_mode == 'execute':
            logfile = self.ctx.get('swarp_logfile', os.path.join(self.ctx.logdir,"%s_swarp.log" % self.ctx.filepattern))
            log = open(logfile,"w")
            print "# Will proceed to run the stiff call now:"
            print "# Will write to logfile: %s" % logfile
            t0 = time.time()
            cmd  = ' '.join(cmd_list)
            print "# Executing stiff for tile:%s " % self.ctx.filepattern
            print "# %s " % cmd
            status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            if status > 0:
                sys.exit("***\nERROR while running Stiff, check logfile: %s\n***" % logfile)
            print "# Total stiff time %s" % elapsed_time(t0)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)
        return


    def get_stiff_parameter_set(self,color_tile,**kwargs):

        """
        Set the Stiff default options and have the options to
        overwrite them with kwargs to this function.
        """

        stiff_parameters = {
            "OUTFILE_NAME"     : color_tile,
            "COMPRESSION_TYPE" : "JPEG",
            }

        stiff_parameters.update(kwargs)
        return stiff_parameters

    def get_stiff_cmd_list(self,color_tile,stiff_parameters={}):

        """ Make a color tiff of the TILENAME using stiff"""

        if self.ctx.NBANDS < 3:
            print "# WARINING: Not enough filters to create color image"
            print "# WARINING: No color images will be created"
            return 
        
        # The default stiff configuration file
        stiff_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.stiff')

        # The update parameters set
        pars = self.get_stiff_parameter_set(color_tile,**stiff_parameters)
        
        # Define the color filter sets we'd like to use, by priority depending on what BANDS will be combined
        cset1 = ['i','r','g']
        cset2 = ['z','r','g']
        cset3 = ['z','i','g']
        cset4 = ['z','i','r']
        csets = (cset1,cset2,cset3,cset4)
        CSET = False
        for color_set in csets:
            if CSET: break
            inset = list( set(color_set) & set(self.ctx.BANDS))
            if len(inset) == 3:
                CSET = color_set

        if not CSET:
            print "# WARNING: Could not find a suitable filter set for color image"
            return 
        
        cmd_list = []
        cmd_list.append("%s" % STIFF_EXE)
        for BAND in CSET:
            cmd_list.append( "%s" % self.ctx.comb_sci[BAND])

        cmd_list.append("-c %s" % stiff_conf)
        for param,value in pars.items():
            cmd_list.append("-%s %s" % (param,value))
        return cmd_list

    def __str__(self):
        return 'Creates the call to Stiff'

if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)

