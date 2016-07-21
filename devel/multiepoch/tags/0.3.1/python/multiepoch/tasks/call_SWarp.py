#!/usr/bin/env python


from despyastro import tableio
import os
import sys
import numpy
import subprocess
import time
import pandas as pd

from despymisc.miscutils import elapsed_time
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
from multiepoch import file_handler as fh

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError

# JOB INTERNAL CONFIGURATION
SWARP_EXE = 'swarp'
DETEC_BANDS_DEFAULT = ['r', 'i', 'z']
MAGBASE = 30.0
DETNAME = 'det'
BKLINE = "\\\n"
class Job(BaseJob):

    class Input(IO):

        """ SWARP call for the multiepoch pipeline"""

        ######################
        # Required inputs
        # 1. Association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })

        # 2. Geometry and tilename
        tileinfo    = Dict(None, help="The json file with the tile information",argparse=False)
        tile_geom_input_file = CUnicode('',help='The json file with the tile information',input_file=True,
                                        argparse={ 'argtype': 'positional', })

        ######################
        # Optional arguments
        tilename    = Unicode(None, help="The Name of the Tile Name to query",argparse=False)
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir     = Unicode(None, help="The output directory for this tile")

        
        detecBANDS       = List(DETEC_BANDS_DEFAULT, help="List of bands used to build the Detection Image, default=%s." % DETEC_BANDS_DEFAULT,
                                argparse={'nargs':'+',})
        magbase          = CFloat(MAGBASE, help="Zero point magnitude base for SWarp, default=%s." % MAGBASE)
        extension_me     = CUnicode('me', help=(" extension to add to me-prepared file names."))
        execution_mode_swarp  = CUnicode("tofile",help="SWarp excution mode",
                                          argparse={'choices': ('tofile','dryrun','execute')})
        swarp_parameters = Dict({},help="A list of parameters to pass to SWarp",
                                argparse={'nargs':'+',})
        swarp_conf = CUnicode(help="Optional SWarp configuration file")

        local_archive = CUnicode("", help="The local filepath where the input fits files (will) live")
        doBANDS       = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})
        detname       = CUnicode(DETNAME,help="File label for detection image, default=%s." % DETNAME)

        # We want to make these options visible as command-line arguments. The full suite of options can be passed as
        # swarp_parameters which will override these defaults
        COMBINE_TYPE_detec = CUnicode('WEIGHTED', help="COMBINE type for detection coadd image")
        COMBINE_TYPE       = CUnicode('CHI-MEAN', help="COMBINE type for band coadd image")
        nthreads           = CInt(1,help="Number of threads to use in stiff/psfex/swarp/scamp")

        # Weight for mask
        weight_for_mask  = Bool(False, help="Create coadded weight for mask creation")

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        # Function to read ASCII/panda framework file (instead of json)
        # Comment if you want to use json files
        def _read_assoc_file(self):
            mydict = {}
            df = pd.read_csv(self.assoc_file,sep=' ')
            mydict['assoc'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

        # To also accept comma-separeted input lists
        def _argparse_postproc_doBANDS(self, v):
            return utils.parse_comma_separated_list(v)

        def _argparse_postproc_swarp_parameters(self, v):
            return utils.arglist2dict(v, separator='=')


    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""

        # Re-construct the names in case not present
        if 'FILEPATH_NWG' not in self.ctx.assoc.keys():
            self.logger.info("(Re)-constructing assoc[FILEPATH_NWG] from assoc[FILEPATH_LOCAL]")
            self.ctx.assoc['FILEPATH_NWG'] = contextDefs.define_red_names(self.ctx)

        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # Get the BANDs information in the context if they are not present
        if self.ctx.get('gotBANDS'):
            self.logger.info("BANDs already defined in context -- skipping")
        else:
            self.ctx.update(contextDefs.get_BANDS(self.ctx.assoc, detname=self.ctx.detname,logger=self.logger,doBANDS=self.ctx.doBANDS))

        # Check info OK
        self.logger.info("BANDS:   %s" % self.ctx.BANDS)
        self.logger.info("doBANDS: %s" % self.ctx.doBANDS)
        self.logger.info("dBANDS:  %s" % self.ctx.dBANDS)
        
    def run(self):

        # 0. Prepare the context
        self.prewash()
        
        # 1. write the swarp input lists
        # ---------------------------------------------------------------------
        # we construct the file names by function and can easily reconstruct
        # them on the fly and do not need to return them here ..
        # --> the input list files are written to the AUX directory
        self.write_swarp_input_list_files()

        # 2. get the command list for the two cases
        # ---------------------------------------------------------------------
        if self.input.weight_for_mask:
            cmd_list_sci = self.get_swarp_cmd_list(type='sci')
            cmd_list_msk = self.get_swarp_cmd_list(type='msk')
        else:
            cmd_list = self.get_swarp_cmd_list(type='sci')

        
        # 3. execute cmd_list according to execution_mode 
        # ---------------------------------------------------------------------
        execution_mode = self.input.execution_mode_swarp
        if execution_mode == 'tofile':
            if self.input.weight_for_mask:
                self.writeCall(cmd_list_sci,mode='w')
                self.writeCall(cmd_list_msk,mode='a')
            else:
                self.writeCall(cmd_list)
                    
        elif execution_mode == 'dryrun':
            self.logger.info("For now we only print the commands (dry-run)")
            for band in self.ctx.dBANDS:
                if self.input.weight_for_mask:
                    self.logger.info(' '.join(cmd_list_sci[band]))
                    self.logger.info(' '.join(cmd_list_msk[band]))
                else:
                    self.logger.info(' '.join(cmd_list[band]))

        elif execution_mode == 'execute':
            if self.input.weight_for_mask:
                self.runSWarp(cmd_list_sci)
                self.runSWarp(cmd_list_msk)
            else:
                self.runSWarp(cmd_list)
        else:
            raise ValueError('Execution mode %s not implemented.' % execution_mode)


    # WRITE THE INPUT FILES
    # -------------------------------------------------------------------------
    def write_swarp_input_list_files(self):
        """
        Write the input file list for SWarp
        """
        self.logger.info('Writing swarp input files')

        for BAND in self.ctx.doBANDS:

            # extracting the list
            #self.ctx.assoc['BAND'] == BAND

            idx = numpy.where(self.ctx.assoc['BAND'] == BAND)[0]
            magzero       = self.ctx.assoc['MAG_ZERO'][idx]
            swarp_inputs  = self.ctx.assoc['FILEPATH_NWG'][idx]
            flxscale      = 10.0**(0.4*(self.input.magbase - magzero))

            # Now let's sort them by filename
            isort = numpy.argsort(swarp_inputs)
            # Test with shuffle
            #numpy.random.shuffle(isort)
            swarp_inputs = swarp_inputs[isort]
            flxscale = flxscale[isort]
            
            # writing the lists to files using tableio.put_data()
            tableio.put_data(fh.get_sci_list_file(self.input.tiledir, self.input.tilename_fh, BAND),(swarp_inputs,),  format='%s[0]')
            tableio.put_data(fh.get_wgt_list_file(self.input.tiledir, self.input.tilename_fh, BAND),(swarp_inputs,),  format='%s[2]')
            tableio.put_data(fh.get_flx_list_file(self.input.tiledir, self.input.tilename_fh, BAND),(flxscale,), format='%s')
            if self.input.weight_for_mask:
                tableio.put_data(fh.get_msk_list_file(self.input.tiledir, self.input.tilename_fh, BAND),(swarp_inputs,),  format='%s[1]')
                
        return

    # ASSEMBLE THE COMMANDS
    # -------------------------------------------------------------------------
    def get_swarp_cmd_list(self,type='sci'):
        """
        Build the SWarp call for a given tile.
        """

        # Sortcuts for less typing
        tiledir     = self.input.tiledir
        tilename_fh = self.input.tilename_fh

        self.logger.info('Assembling commands for SWarp call')

        # Update and Set the SWarp options 
        pars = self.get_swarp_parameter_set(**self.input.swarp_parameters)

        # The SWarp configuration file
        if self.input.swarp_conf == '':
            self.ctx.swarp_conf = fh.get_configfile('swarp')
            self.logger.info("Will use SWarp default configuration file: %s" % self.ctx.swarp_conf)
            
        swarp_cmd = {} # To create the science image we'll keep

        # Loop over all filters
        for BAND in self.ctx.doBANDS:
            
            # FSCALE_DEFAULT is the same for two swarp calls
            pars["FSCALE_DEFAULT"] = "@%s" % fh.get_flx_list_file(tiledir,tilename_fh, BAND)

            # 1. The First SWarp call for the Science image -- depending on the mode
            if type=='sci': # Create science image using non-zero center of starr
                pars["IMAGEOUT_NAME"]  = "%s"  % fh.get_sci_fits_file(tiledir,tilename_fh, BAND) # sci.fits -- keep
                pars["WEIGHTOUT_NAME"] = "%s"  % fh.get_wgt_fits_file(tiledir,tilename_fh, BAND) # wgt.fits -- keep
                pars["WEIGHT_IMAGE"]   = "@%s" % fh.get_wgt_list_file(tiledir,tilename_fh, BAND) # list of weights
            elif type=='msk': # Create coadd of weight with starts null to create a msk plane
                pars["IMAGEOUT_NAME"]  = "%s"  % fh.get_gen_fits_file(tiledir,tilename_fh, BAND, type='tmp_sci') # -- discard
                pars["WEIGHTOUT_NAME"] = "%s"  % fh.get_msk_fits_file(tiledir,tilename_fh, BAND) # msk.fits -- keep
                pars["WEIGHT_IMAGE"]   = "@%s" % fh.get_msk_list_file(tiledir,tilename_fh, BAND) # list of msk/weights
            else:
                self.logger.info("ERROR: Need to define either sci/msk on call")

            # Construct the call
            cmd = []
            cmd.append(SWARP_EXE)
            cmd.append("@%s" % fh.get_sci_list_file(tiledir,tilename_fh, BAND))
            cmd.append("-c %s" % self.ctx.swarp_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            swarp_cmd[BAND] = cmd
            
        ## Prepare the detection SWarp call now
        BAND = self.ctx.detBAND # short cut
        if "FSCALE_DEFAULT" in pars : del pars["FSCALE_DEFAULT"]

        # The Science and Weight lists matching the bands used for detection
        useBANDS = list( set(self.ctx.BANDS) & set(self.ctx.detecBANDS) )

        if type=='sci': 
            det_scilists = [ fh.get_sci_fits_file(tiledir,tilename_fh, band) for band in useBANDS ]
            det_wgtlists = [ fh.get_wgt_fits_file(tiledir,tilename_fh, band) for band in useBANDS ]
            pars["IMAGEOUT_NAME"]  = "%s" % fh.get_sci_fits_file(tiledir,tilename_fh, BAND) # det_sci.fits -- keep
            pars["WEIGHTOUT_NAME"] = "%s" % fh.get_wgt_fits_file(tiledir,tilename_fh, BAND) # det_wgt.fits -- keep
            pars["WEIGHT_IMAGE"]   = "%s" % ",".join(det_wgtlists)
        elif type=='msk':
            det_scilists = [ fh.get_sci_fits_file(tiledir,tilename_fh, band) for band in useBANDS ]
            det_wgtlists = [ fh.get_msk_fits_file(tiledir,tilename_fh, band) for band in useBANDS ]
            pars["IMAGEOUT_NAME"]  = "%s" % fh.get_gen_fits_file(tiledir,tilename_fh, BAND, type='tmp_sci') # tmp_sci.fits -- discard
            pars["WEIGHTOUT_NAME"] = "%s" % fh.get_msk_fits_file(tiledir,tilename_fh, BAND)
            pars["WEIGHT_IMAGE"]   = "%s" % ",".join(det_wgtlists)
        else:
            print "# Need to define either sci/msk"

        # The call to get the detection image
        pars['RESAMPLE'] = 'N' # We do not need to resample them, they are on the same pixel coordinates
        self.logger.info("Will use COMBINE_TYPE=%s for detection image" % self.ctx.COMBINE_TYPE_detec)
        pars['COMBINE_TYPE']  = self.ctx.COMBINE_TYPE_detec
        pars['COPY_KEYWORDS'] = "BUNIT,TILENAME,TILEID" # Avoid FILTER and BAND

        swarp_cmd[BAND] = [SWARP_EXE, ]
        swarp_cmd[BAND].append("%s" % " ".join(det_scilists))
        swarp_cmd[BAND].append("-c %s" % self.ctx.swarp_conf)
        for param,value in pars.items():
            swarp_cmd[BAND].append("-%s %s" % (param,value))

        return swarp_cmd

    def get_swarp_parameter_set(self, **kwargs):
        """
        Set the SWarp default options for all band in a tile and overwrite them
        with kwargs to this function.
        """
        swarp_parameters = {
            "COMBINE_TYPE"    : self.ctx.COMBINE_TYPE,
            "PIXEL_SCALE"     : "%.3f"  % self.ctx.tileinfo['PIXELSCALE'],
            "NTHREADS"        : self.ctx.nthreads,
            "IMAGE_SIZE"      : "%s,%d" % (self.ctx.tileinfo['NAXIS1'],self.ctx.tileinfo['NAXIS2']),
            "CENTER"          : "%s,%s" % (self.ctx.tileinfo['RA_CENT'],self.ctx.tileinfo['DEC_CENT']),
            "RESAMPLE_DIR"    : os.path.join(self.ctx.tiledir,'coadd'),
            }
        swarp_parameters.update(kwargs)
        return swarp_parameters

    # 'EXECUTION' FUNCTIONS
    # -------------------------------------------------------------------------

    def writeCall(self,cmd_list,mode='w'):

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_swarp_cmd_file(self.input.tiledir, self.input.tilename_fh)
        self.logger.info("Will write SWarp call to: %s" % cmdfile)
        with open(cmdfile, mode) as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n\n')
        return


    def runSWarp(self, cmd_list):

        logfile = fh.get_swarp_log_file(self.input.tiledir, self.input.tilename_fh)
        log = open(logfile,"w")
        self.logger.info("Will proceed to run the SWarp calls now:")
        self.logger.info("Will write to logfile: %s" % logfile)
        t0 = time.time()

        for band in self.ctx.dBANDS:
            t1 = time.time()
            cmd  = ' '.join(cmd_list[band])
            self.logger.info("Executing SWarp SCI for tile:%s, BAND:%s" % (self.ctx.tilename_fh,band))
            self.logger.info("%s " % cmd)
            status = subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            if status != 0:
                raise RuntimeError("\n***\nERROR while running SWarp, check logfile: %s\n***" % logfile)
            self.logger.info("Done in %s" % elapsed_time(t1))

        self.logger.info("Total SWarp time: %s" % elapsed_time(t0))
        log.write("Total SWarp time: %s\n" % elapsed_time(t0))
        log.close()

    # -------------------------------------------------------------------------

    def __str__(self):
        return 'Creates the call to SWarp'

if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)



