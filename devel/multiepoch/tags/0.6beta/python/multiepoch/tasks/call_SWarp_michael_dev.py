#!/usr/bin/env python

import os
import sys
import numpy
import subprocess
import time

from traitlets import Unicode, CUnicode, CFloat, Dict, List
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError

from despyastro import tableio
from despymisc.miscutils import elapsed_time

import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh


# JOB CONFIGURATION 
# -----------------------------------------------------------------------------

SWARP_EXE = 'swarp'
DETEC_BANDS_DEFAULT = ['r', 'i', 'z']
BKLINE = "\\\n"
MAGBASE = 30.0


# JOB
# -----------------------------------------------------------------------------

class Job(BaseJob):

    class Input(IO):
        """ SWARP call for the multiepoch pipeline. """
        
        # Required inputs #####################################################

        # 1. Association file and assoc dictionary
        assoc = Dict(None,
                    help="The Dictionary containing the association file",
                    argparse=False)
        assoc_file = CUnicode('',
                help="Input association file with CCDs information",
                input_file=True, argparse={ 'argtype': 'positional', })

        # 2. Geometry and tilename
        tileinfo = Dict(None, help="The json file with the tile information",
                        argparse=False)
        tilename = Unicode(None, help="The name of the tile.",
                           argparse=False)
        tiledir = CUnicode(None, help='The output directory for this tile.')

        tile_geom_input_file = CUnicode('',
                help='The json file with the tile information',
                input_file=True, argparse={ 'argtype': 'positional', })
        
        # Optional arguments ##################################################
        detecBANDS = List(DETEC_BANDS_DEFAULT,
                help="List of bands used to build the Detection Image")
        magbase = CFloat(MAGBASE,
                help="Zero point magnitude base for SWarp, default=30.")
        swarp_parameters = Dict({},
                help="A list of parameters to pass to SWarp",
                argparse={'nargs':'+',})
        swarp_execution_mode = CUnicode("tofile",
                help="SWarp excution mode",
                argparse={'choices': ('tofile','dryrun','execute')})

        def _validate_conditional(self):
            pass

        def _argparse_postproc_swarp_parameters(self, v):
            return utils.arglist2dict(v, separator='=')


    def run(self):

        # 0. preparation
        # ---------------------------------------------------------------------
        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # add implicit information given in the assoc file directly to the ctx
        self.ctx.update(contextDefs.get_BANDS(self.ctx.assoc, detname='det',
            logger=self.logger))

        # 1. write the swarp input lists
        # ---------------------------------------------------------------------
        # we construct the file names by function and can easily reconstruct
        # them on the fly and do not need to return them here ..
        # --> the input list files are written to the AUX directory
        self.write_input_list_files()

        # 2. get the command list 
        # ---------------------------------------------------------------------
        cmd_list = self.get_swarp_cmd_list()

        # 3. execute cmd_list according to execution_mode 
        # ---------------------------------------------------------------------
        execution_mode = self.ctx.swarp_execution_mode
        if execution_mode == 'tofile':
            self.writeCall(cmd_list)
                    
        elif execution_mode == 'dryrun':
            self.logger.info("# For now we only print the commands (dry-run)")
            for band in self.ctx.dBANDS:
                self.logger.info(' '.join(cmd_list[band]))

        elif execution_mode == 'execute':
            self.runSWarp(cmd_list)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)


    # WRITE THE INPUT FILES
    # -------------------------------------------------------------------------

    def write_input_list_files(self):
        '''
        '''
        self.logger.info('# writing swarp input files')

        for BAND in self.ctx.BANDS:
            # extracting the list
            idx = numpy.where(self.ctx.assoc['BAND'] == BAND)[0]
            swarp_inputs  = self.ctx.assoc['FILEPATH_LOCAL'][idx]
            swarp_magzero = self.ctx.assoc['MAG_ZERO'][idx]
            # writing the lists to files
            tableio.put_data(
                fh.get_sci_list_file(self.input.tiledir, self.input.tilename, BAND),
                (swarp_inputs,), format='%s[0]')
            tableio.put_data(
                fh.get_wgt_list_file(self.input.tiledir, self.input.tilename, BAND),
                (swarp_inputs,), format='%s[2]')
            tableio.put_data(
                fh.get_flx_list_file(self.input.tiledir, self.input.tilename, BAND),
                (self.flxscale(self.input.magbase, swarp_magzero),), format='%s')

    @staticmethod
    def flxscale(magbase, swarp_magzero):
        return 10.0**(0.4*(magbase - swarp_magzero))


    # ASSEMBLE THE COMMANDS
    # -------------------------------------------------------------------------

    def get_swarp_cmd_list(self):
        """ Build the SWarp call for a given tile.
        """

        self.logger.info('# assembling commands for swarp call')

        # The SWarp options that stay the same for all tiles
        pars = self.get_swarp_parameter_set(**self.input.swarp_parameters)

        # The default swarp configuration file
        # FIXME : this only works in the eeups context where MULTIEPOCH_DIR is set ..
        swarp_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.swarp')

        swarp_cmd = {} # To create the science image we'll keep

        # Loop over all filters
        for BAND in self.ctx.BANDS:

            # General - BAND specific configurations
            pars["FSCALE_DEFAULT"] = "@%s" % fh.get_flx_list_file(
                    self.input.tiledir, self.input.tilename, BAND)

            # 1. The call 
            cmd = []
            pars["WEIGHT_IMAGE"]   = "@%s" % fh.get_wgt_list_file(
                            self.input.tiledir, self.input.tilename, BAND)
            pars["IMAGEOUT_NAME"]  = "%s" % fh.get_sci_fits_file(
                            self.input.tiledir, self.input.tilename, BAND) 
            pars["WEIGHTOUT_NAME"] = "%s" % fh.get_wgt_fits_file(
                            self.input.tiledir, self.input.tilename, BAND)
            # Construct the call
            cmd.append(SWARP_EXE)
            cmd.append("@%s" % fh.get_sci_list_file(
                            self.input.tiledir, self.input.tilename, BAND))
            cmd.append("-c %s" % swarp_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            swarp_cmd[BAND] = cmd


        ## Prepare the detection SWarp call now
        BAND = self.ctx.detBAND # short cut
        if "FSCALE_DEFAULT" in pars : del pars["FSCALE_DEFAULT"]

        useBANDS = list( set(self.ctx.BANDS) & set(self.input.detecBANDS) )
        det_scilists = [ fh.get_sci_fits_file(
            self.input.tiledir, self.input.tilename, band) for band in useBANDS ]
        det_wgtlists = [ fh.get_wgt_fits_file(
            self.input.tiledir, self.input.tilename, band) for band in useBANDS ]

        # The call to get the detection image
        pars["WEIGHT_IMAGE"]   = "%s" % " ".join(det_wgtlists)
        pars["IMAGEOUT_NAME"]  = "%s" % fh.get_sci_fits_file(
                self.input.tiledir, self.input.tilename, BAND) 
        pars["WEIGHTOUT_NAME"] = "%s" % fh.get_wgt_fits_file(
                self.input.tiledir, self.input.tilename, BAND) 

        swarp_cmd[BAND] = [SWARP_EXE, ]
        swarp_cmd[BAND].append("%s" % " ".join(det_scilists))
        swarp_cmd[BAND].append("-c %s" % swarp_conf)
        for param,value in pars.items():
            swarp_cmd[BAND].append("-%s %s" % (param,value))

        return swarp_cmd


    def get_swarp_parameter_set(self, **kwargs):
        """
        Set the SWarp default options for all band in a tile and overwrite them
        with kwargs to this function.
        """

        swarp_parameters = {
            "COMBINE_TYPE"    : "MEDIAN",
            "WEIGHT_TYPE"     : "MAP_WEIGHT",
            "PIXEL_SCALE"     : "%.3f"  % self.ctx.tileinfo['PIXELSCALE'],
            "PIXELSCALE_TYPE" : "MANUAL",
            "NTHREADS"        : 1,
            "CENTER_TYPE"     : "MANUAL",
            "IMAGE_SIZE"      : "%s,%d" % (self.ctx.tileinfo['NAXIS1'],self.ctx.tileinfo['NAXIS2']),
            "CENTER"          : "%s,%s" % (self.ctx.tileinfo['RA'],self.ctx.tileinfo['DEC']),
            "WRITE_XML"       : "N",
            }
        swarp_parameters.update(kwargs)

        return swarp_parameters


    # 'EXECUTION' FUNCTIONS
    # -------------------------------------------------------------------------

    def writeCall(self, cmd_list):
        bkline  = self.ctx.get('breakline', BKLINE)
        cmdfile = fh.get_swarp_cmd_file(self.input.tiledir, self.input.tilename)
        self.logger.info("# Will write SWarp call to: %s" % cmdfile)
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n\n')


    def runSWarp(self, cmd_list):

        # FIXME :: logfile path via dh !!
        logfile = fh.get_swarp_log_file(self.input.tiledir, self.input.tilename)
        log = open(logfile,"w")
        self.logger.info("# Will proceed to run the SWarp calls now:")
        self.logger.info("# Will write to logfile: %s" % logfile)
        t0 = time.time()

        for band in self.ctx.dBANDS:
            t1 = time.time()
            cmd  = ' '.join(cmd_list[band])
            self.logger.info("# Executing SWarp SCI for tile:%s, BAND:%s" % (self.ctx.tilename,band))
            self.logger.info("# %s " % cmd)
            status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            if status > 0:
                raise RuntimeError("\n***\nERROR while running SWarp, check logfile: %s\n***" % logfile)
            self.logger.info("# Done in %s\n" % elapsed_time(t1))

        self.logger.info("# Total SWarp time %s" % elapsed_time(t0))
        log.write("# Total SWarp time %s\n" % elapsed_time(t0))
        log.close()

    # -------------------------------------------------------------------------

    def __str__(self):
        return '"call SWarp"'



if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
