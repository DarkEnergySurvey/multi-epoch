#!/usr/bin/env python

from despyastro import tableio
import os
import sys
import numpy
import time
import pandas as pd
import multiprocessing as mp

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
DETNAME = 'det'
BKLINE = "\\\n"
class Job(BaseJob):

    class Input(IO):

        """ Scamp preparation call for the multiepoch pipeline"""

        ######################
        # Required inputs
        # 1. Association file and assoc dictionary
        #assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        #assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
        #                      argparse={ 'argtype': 'positional', })

        # Super-alignment options
        catlist      = Dict(None,help="The Dictionary containing input CCD-level catalog list ",argparse=False)
        cats_file    = CUnicode('',help="Name of the output ASCII catalog list storing the information for scamp", input_file=True,
                                argparse={ 'argtype': 'positional', })
        #super_align  = Bool(False, help=("Run super-aligment of tile using scamp"))

        execution_mode_scamp_prep  = CUnicode("tofile",help="Scamp prepare files excution mode",
                                              argparse={'choices': ('tofile','dryrun','execute')})
        MP_cats      = CInt(1, help = ("Run using multi-process, 0=automatic, 1=single-process [default]"))
        
        # 2. Geometry and tilename
        #tileinfo    = Dict(None, help="The json file with the tile information",argparse=False)
        #tile_geom_input_file = CUnicode('',help='The json file with the tile information',input_file=True,
        #                                argparse={ 'argtype': 'positional', })

        ######################
        # Optional arguments
        tilename    = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir     = Unicode(None, help="The output directory for this tile")

        local_archive     = Unicode(None, help="The local filepath where the input fits files (will) live")
        #local_archive_me  = Unicode(None, help=('The path to the me prepared files archive.'))
        
        #detecBANDS       = List(DETEC_BANDS_DEFAULT, help="List of bands used to build the Detection Image, default=%s." % DETEC_BANDS_DEFAULT,
        #                        argparse={'nargs':'+',})
        #magbase          = CFloat(MAGBASE, help="Zero point magnitude base for SWarp, default=%s." % MAGBASE)
        #extension_me     = CUnicode('_me', help=(" extension to add to me-prepared file names."))
        #execution_mode_swarp  = CUnicode("tofile",help="SWarp excution mode",
        #                                  argparse={'choices': ('tofile','dryrun','execute')})
        #swarp_parameters = Dict({},help="A list of parameters to pass to SWarp",
        #                        argparse={'nargs':'+',})

        doBANDS          = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})
        detname          = CUnicode(DETNAME,help="File label for detection image, default=%s." % DETNAME)

        # We want to make these options visible as command-line arguments. The full suite of options can be passed as
        # swarp_parameters which will override these defaults
        #COMBINE_TYPE_detec = CUnicode('AVERAGE',  help="COMBINE type for detection coadd image")
        #COMBINE_TYPE       = CUnicode('CHI-MEAN', help="COMBINE type for band coadd image")
        #nthreads           = CInt(1,help="Number of threads to use in stiff/psfex/swarp")

        # Weight for mask
        #weight_for_mask  = Bool(False, help="Create coadded weight for mask creation")

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        # Function to read ASCII/panda framework file (instead of json)
        # Comment if you want to use json files
        #def _read_assoc_file(self):
        #    mydict = {}
        #    df = pd.read_csv(self.assoc_file,sep=' ')
        #    mydict['assoc'] = {col: df[col].values.tolist() for col in df.columns}
        #    return mydict

        def _read_cats_file(self):
            mydict = {}
            df = pd.read_csv(self.cats_file,sep=' ')
            mydict['catlist'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):

            # Get logger
            logger = mojo_log.get_logger({})
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename
            if self.stdoutloglevel == 'DEBUG':
                os.environ['FITSUTILS_DEBUG'] = "4"

            #if self.catlist and not self.super_align:
            #    logger.info("Updating super_align value to True")
            #    self.super_align = True

        # To also accept comma-separeted input lists
        def _argparse_postproc_doBANDS(self, v):
            return utils.parse_comma_separated_list(v)

    def prewash_scamp(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""

        # Re-cast the ctx.assoc/ctx.catlist as dictionary of arrays instead of lists
        #self.ctx.assoc    = utils.dict2arrays(self.ctx.assoc)
        self.ctx.catlist  = utils.dict2arrays(self.ctx.catlist)

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
        
        execution_mode = self.input.execution_mode_scamp_prep

        # 0. Prepare the context
        self.prewash_scamp()

        # 1. Get the unitnames for the doBANDS selection
        self.ctx.unitnames = contextDefs.get_scamp_unitnames(self.ctx)

        # 2. Get the list of command lines
        cmdlist = self.get_combine_cats_cmd_list(execution_mode)
        
        # 3. Execute a cmdlist according to execution_mode 
        # --> the input list files are written to the AUX directory
        self.logger.info("Combining CCD catalogs for scamp")
        if execution_mode == 'tofile':
            self.writeCall_combine_cats(cmdlist)
        elif execution_mode == 'dryrun':
            [self.logger.info(' '.join(cmdlist[key])) for key in cmdlist.keys()]
        elif execution_mode == 'execute':
            self.combine_cats_for_scamp(self.input.MP_cats)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)
        return

    # Create the inputs and combine the catalogs for scamp
    # -------------------------------------------------------------------------
    def combine_cats_for_scamp(self,MP):
        """
        Combine the CCD catalogs into exposure-based catalogs for scamp inputs
        """

        t0 = time.time()
        NP = utils.get_NP(MP)  # Figure out NP to use, 0=automatic
        
        if NP > 1 :
            p = mp.Pool(processes=NP)
        # Loop over all unitnames
        for UNITNAME in self.ctx.unitnames:
            self.logger.info("Combining CCD catalogs for %s" % UNITNAME)
            # The sorted CCD catlist per UNITNAME and the output name for the conbined cat as args
            args = (','.join(self.get_ccd_catlist(self.ctx.catlist,UNITNAME)),
                    fh.get_expcat_file(self.input.tiledir, self.input.tilename_fh, UNITNAME))
            kw = {}
            if NP > 1:
                p.apply_async(fitsutils.combine_cats, args, kw)
            else:
                fitsutils.combine_cats(*args)
                
        if NP > 1:
            p.close()
            p.join()

        self.logger.info("Total Catalog Combine time: %s" % elapsed_time(t0))
        return

    # Assemble the command
    # -------------------------------------------------------------------------
    def get_combine_cats_cmd_list(self,execute_mode):

        # Sortcuts for less typing
        tiledir     = self.input.tiledir
        tilename_fh = self.input.tilename_fh
        # The dictionary, keyed to UNITNAMES where we store the commands
        combine_cats_cmd = {}
        for UNITNAME in self.ctx.unitnames:
            # Get the sorted CCD catlist per UNITNAME
            ccd_catlist = self.get_ccd_catlist(self.ctx.catlist,UNITNAME)
            # Write out the file list containing the input catalogs
            if execute_mode == 'tofile' or execute_mode == 'dryrun':
                tableio.put_data(fh.get_catlist_file(tiledir, tilename_fh, UNITNAME),(ccd_catlist,),format="%s")

            # Build the cmdline string
            cmd = []
            cmd.append("combine_cats.py")
            cmd.append("--list  %s" % fh.get_catlist_file(tiledir, tilename_fh, UNITNAME))
            cmd.append("--outcat %s" % fh.get_expcat_file(tiledir, tilename_fh, UNITNAME))
            combine_cats_cmd[UNITNAME] = cmd
        return combine_cats_cmd

    # 'EXECUTION' FUNCTIONS
    # -------------------------------------------------------------------------
    def writeCall_combine_cats(self,cmd_list,mode='w'):

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_combine_cats_cmd_file(self.input.tiledir, self.input.tilename_fh)
        self.logger.info("Will write combine_cats call to: %s" % cmdfile)
        with open(cmdfile, mode) as fid:
            for key in cmd_list.keys():
                fid.write(bkline.join(cmd_list[key])+'\n')
                fid.write('\n\n')
        return

    # -------------------------------------------------------------------------

    @staticmethod
    def get_ccd_catlist(catlist,unitname):
        # TODO: Maybe move to the contextDef function
        """ Consistent method to extract ccd catlist from context per unitname"""
        ccd_catlist = catlist['FILEPATH_LOCAL'][catlist['UNITNAME'] == unitname]
        ccd_catlist.sort() # Make sure that they are sorted
        return ccd_catlist

    def __str__(self):
        return 'Prepares the file for the scamp call'

if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)



