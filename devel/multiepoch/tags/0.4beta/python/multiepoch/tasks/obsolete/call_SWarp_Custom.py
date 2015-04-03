from mojo.jobs.base_job import BaseJob
from despyastro import tableio
import os
import sys
import numpy
import subprocess
import time
from despymisc.miscutils import elapsed_time


class Job(BaseJob):

    """ SWARP call for the multiepoch pipeline"""

    # JOB INTERNAL CONFIGURATION
    SWARP_EXE = 'swarp'
    DETEC_BANDS_DEFAULT = ['r', 'i', 'z']
    BKLINE = "\\\n"

    def __call__(self):

        # 1. set up names -----------------------------------------------------
        # The band to consider for the detection image
        detecBANDS = self.ctx.get('detecBANDS', self.DETEC_BANDS_DEFAULT)

        # Gets swarp_scilist, swarp_wgtlist, swarp_flxlist, comb_sci, comb_wgt and  comb_sci/tmps
        (swarp_scilist, swarp_wgtlist, swarp_swglist, swarp_flxlist,
        comb_sci, comb_wgt, comb_sci_tmp, comb_wgt_tmp) = self.get_swarp_names(detecBANDS=detecBANDS)
        
        # Make some objects visible to the context that will be needed
        # down the line for SEx, psfex, etc.
        self.ctx.comb_sci = comb_sci
        self.ctx.comb_wgt = comb_wgt

        # 2. get the update swarp parameters and the full command list of SWarp calls --
        swarp_parameters = self.ctx.get('swarp_parameters', {})
        cmd_list_sci, cmd_list_wgt = self.get_swarp_cmd_list(swarp_scilist, swarp_wgtlist, swarp_swglist, swarp_flxlist,
                                                             comb_sci, comb_wgt, comb_sci_tmp, comb_wgt_tmp,
                                                             swarp_parameters=swarp_parameters)

        # 3. check execution mode and write/print/execute commands accordingly --------------
        executione_mode = self.ctx.get('swarp_execution_mode', 'tofile')
        if executione_mode == 'tofile':
            bkline  = self.ctx.get('breakline',self.BKLINE)
            # The file where we'll write the commands
            cmdfile = self.ctx.get('cmdfile', os.path.join(self.ctx.tiledir,"call_swarp_%s.cmd" % self.ctx.tilename))
            print "# Will write SWarp Detection call to: %s" % cmdfile
            with open(cmdfile, 'w') as fid:
                for band in self.ctx.dBANDS:
                    fid.write(bkline.join(cmd_list_sci[band])+'\n')
                    fid.write(bkline.join(cmd_list_wgt[band])+'\n')
                    fid.write('\n')
                    
        elif executione_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            for band in self.ctx.dBANDS:
                print ' '.join(cmd_list_sci[band])
                print ' '.join(cmd_list_wgt[band])

        elif executione_mode == 'execute':
            logfile = self.ctx.get('logfile', os.path.join(self.ctx.tiledir,"swarp_%s.log" % self.ctx.tilename))
            log = open(logfile,"w")
            print "# Will proceed to run the SWarp calls now:"
            print "# Will write to logfile: %s" % logfile
            t0 = time.time()
            for band in self.ctx.dBANDS:
                t1 = time.time()
                cmd  = ' '.join(cmd_list_sci[band])
                print "# Executing SWarp for tile:%s, BAND:%s" % (self.ctx.tilename,band)
                print "# %s " % cmd
                subprocess.call(cmd,shell=True,stdout=log, stderr=log)
                print "# Done in %s\n" % elapsed_time(t1)

                t2 = time.time()
                cmd  = ' '.join(cmd_list_wgt[band])
                print "# Executing SWarp for tile:%s, BAND:%s" % (self.ctx.tilename,band)
                print "# %s " % cmd
                subprocess.call(cmd,shell=True,stdout=log, stderr=log)
                print "# Done in %s\n" % elapsed_time(t2)

            print "# Total SWarp time %s" % elapsed_time(t0)
        else:
            raise ValueError('Execution mode %s not implemented.' % executione_mode)


    def get_swarp_cmd_list(self, swarp_scilist, swarp_wgtlist, swarp_swglist,
                           swarp_flxlist, comb_sci, comb_wgt, comb_sci_tmp, comb_wgt_tmp, swarp_parameters={}):

        """ Make the SWarp call for a given TILENAME"""

        # The SWarp options that stay the same for all tiles
        pars = self.get_swarp_parameter_set(**swarp_parameters)

        # The default swarp configuration file
        swarp_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.swarp')

        swarp_cmd_sci = {} # To create the science image we'll keep
        swarp_cmd_wgt = {} # To create the weight image we'll keep

        # Loop over all filters
        for BAND in self.ctx.BANDS:

            # General - BAND specific configurations
            pars["FSCALE_DEFAULT"] = "@%s" % (swarp_flxlist[BAND])

            # 1. The call to get the interpolated science image
            cmd = []
            pars["WEIGHT_IMAGE"]   = "@%s" % (swarp_swglist[BAND]) # custom weights
            pars["IMAGEOUT_NAME"]  = "%s" % comb_sci[BAND]
            pars["WEIGHTOUT_NAME"] = "%s" % comb_wgt_tmp[BAND]     # We won't keep this sci
            # Construct the call
            cmd.append(self.SWARP_EXE)
            cmd.append("@%s" % swarp_scilist[BAND])
            cmd.append("-c %s" % swarp_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            swarp_cmd_sci[BAND] = cmd

            # 2. The call to get the real combined weigth
            cmd = []
            pars["WEIGHT_IMAGE"]   = "@%s" % (swarp_wgtlist[BAND]) # The real weights
            pars["IMAGEOUT_NAME"]  = "%s" % comb_sci_tmp[BAND]     # We won't keep this weight
            pars["WEIGHTOUT_NAME"] = "%s" % comb_wgt[BAND]
            # Construct the call
            cmd.append(self.SWARP_EXE)
            cmd.append("@%s" % swarp_scilist[BAND])
            cmd.append("-c %s" % swarp_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            swarp_cmd_wgt[BAND] = cmd

        ## Prepare the detection SWarp call now
        BAND = self.ctx.detBAND # short cut
        if "FSCALE_DEFAULT" in pars : del pars["FSCALE_DEFAULT"]

        # 1. The call to get the interpolated science image
        pars["WEIGHT_IMAGE"]   = "%s" % " ".join(swarp_swglist[BAND])
        pars["IMAGEOUT_NAME"]  = "%s" % comb_sci[BAND]
        pars["WEIGHTOUT_NAME"] = "%s" % comb_wgt_tmp[BAND]

        swarp_cmd_sci[BAND] = [self.SWARP_EXE, ]
        swarp_cmd_sci[BAND].append("%s" % " ".join(swarp_scilist[BAND]))
        swarp_cmd_sci[BAND].append("-c %s" % swarp_conf)
        for param,value in pars.items():
            swarp_cmd_sci[BAND].append("-%s %s" % (param,value))

        # The alt call
        pars["WEIGHT_IMAGE"]   = "%s" % " ".join(swarp_wgtlist[BAND])
        pars["IMAGEOUT_NAME"]  = "%s" % comb_sci_tmp[BAND]
        pars["WEIGHTOUT_NAME"] = "%s" % comb_wgt[BAND]
        swarp_cmd_wgt[BAND] = [self.SWARP_EXE, ]
        swarp_cmd_wgt[BAND].append("%s" % " ".join(swarp_scilist[BAND]))
        swarp_cmd_wgt[BAND].append("-c %s" % swarp_conf)
        for param,value in pars.items():
            swarp_cmd_wgt[BAND].append("-%s %s" % (param,value))


        return swarp_cmd_sci, swarp_cmd_wgt

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


    def get_swarp_names(self, **kwargs):

        """ Defines the input/output file names for SWarp,

        Inputs: (active can be passed as kwargs)
        - detectBANDS, from kwrgs or: ['r','i','z']
        - tilename: self.ctx.tilename
        - magbase: from kwargs (or 30)

        Inputs (passive passed as ctx)
        - self.ctx.CCDS['MAGZERO']
        - self.ctx.FILEPATH_LOCAL

        Ouputs:
         - swarp_scilist, swarp_wgtlist, swarp_flxlist needed for SWarp
         - comb_sci and comb_wgt
         - comb_sci_tmp and comb_wgt_tmp
        """

        print "# Setting names for SWarp calls"

        # Extract the relevant kwargs
        detecBANDS = kwargs.get('detecBANDS',['r','i','z']) # The band to consider for the detection image
        magbase    = kwargs.get('magbase',30.0) # The magbase for flxscale, FLXSCALE = 10**(0.4*(magbase-zp))
        tilename   = kwargs.get('tilename',self.ctx.tilename)

        # output variables
        swarp_scilist = {} # File with list of science names -- fits[0]
        swarp_wgtlist = {} # File with list of weight names -- fits[2]
        swarp_flxlist = {} # File with list of fluxscales from zeropoints
        swarp_swglist = {} # File with list of swarp weight names -- _wgt.fits files

        # SWarp outputs per filer
        comb_sci  = {} # SWarp coadded science images
        comb_wgt  = {} # SWarp coadded weight images

        comb_sci_tmp  = {} # SWarp coadded science images -- to be removed later
        comb_wgt_tmp  = {} # SWarp coadded custom weight images -- to be removed

        # function local variables -- short handles
        swarp_inputs  = {} # Array with all filenames
        swarp_magzero = {} # Array with all fluxscales in BAND
        swarp_weights = {} # Array with custom weights for SWarp

        # Loop over filters
        for BAND in self.ctx.BANDS:

            print "# Examining BAND: %s" % BAND

            # Relevant indices per band
            idx = numpy.where(self.ctx.CCDS['BAND'] == BAND)[0]

            # 1. SWarp inputs
            swarp_inputs[BAND]  = self.ctx.FILEPATH_LOCAL[idx]
            swarp_weights[BAND] = self.ctx.FILEPATH_LOCAL_WGT[idx]
            swarp_magzero[BAND] = self.ctx.CCDS['MAG_ZERO'][idx]
            
            swarp_scilist[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_sci.list" % (self.ctx.tilename,BAND))
            swarp_wgtlist[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_wgt.list" % (self.ctx.tilename,BAND))
            swarp_swglist[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_swg.list" % (self.ctx.tilename,BAND))
            swarp_flxlist[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_flx.list" % (self.ctx.tilename,BAND))

            print "# Writing science files for tile:%s band:%s on:%s" % (self.ctx.tilename,BAND,swarp_scilist[BAND])
            tableio.put_data(swarp_scilist[BAND],(swarp_inputs[BAND],),format="%s[0]")

            print "# Writing weight files for tile: %s band:%s on:%s" % (self.ctx.tilename,BAND,swarp_wgtlist[BAND])
            tableio.put_data(swarp_wgtlist[BAND],(swarp_inputs[BAND],),format="%s[2]")

            print "# Writing SWarp weight files for tile: %s band:%s on:%s" % (self.ctx.tilename,BAND,swarp_swglist[BAND])
            tableio.put_data(swarp_swglist[BAND],(swarp_weights[BAND],),format="%s")

            flxscale = 10.0**(0.4*(magbase - swarp_magzero[BAND]))
            print "# Writing fluxscale values for tile: %s band:%s on:%s" % (self.ctx.tilename,BAND,swarp_flxlist[BAND])
            tableio.put_data(swarp_flxlist[BAND],(flxscale,),format="%s")

            # 2. SWarp outputs names
            comb_sci[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_sci.fits" %  (self.ctx.tilename, BAND))
            comb_wgt[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_wgt.fits" %  (self.ctx.tilename, BAND))

            # temporary files to be removed
            comb_sci_tmp[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_sci_tmp.fits" %  (self.ctx.tilename, BAND))
            comb_wgt_tmp[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_wgt_tmp.fits" %  (self.ctx.tilename, BAND))


        # The SWarp-combined detection image input and ouputs
        # Figure out which bands to use that match the detecBANDS
        useBANDS = list( set(self.ctx.BANDS) & set(detecBANDS) )
        print "# Will use %s bands for detection" % useBANDS

        # The BAND pseudo-name, we'll store with the 'real bands' as a list to access later
        self.ctx.detBAND ='det%s' % "".join(useBANDS)
        self.ctx.dBANDS = list(self.ctx.BANDS) + [self.ctx.detBAND]

        # Names and lists
        BAND = self.ctx.detBAND  # Short-cut
        comb_sci[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_sci.fits" %  (self.ctx.tilename, BAND))
        comb_wgt[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_wgt.fits" %  (self.ctx.tilename, BAND))
        comb_sci_tmp[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_sci_tmp.fits" %  (self.ctx.tilename, BAND))
        comb_wgt_tmp[BAND] = os.path.join(self.ctx.tiledir,"%s_%s_wgt_tmp.fits" %  (self.ctx.tilename, BAND))
        
        # The Science and Weight lists matching the bands used for detection
        swarp_scilist[BAND] = [comb_sci[band] for band in useBANDS] # old: extract_from_keys(comb_sci, useBANDS).values()
        swarp_wgtlist[BAND] = [comb_wgt[band] for band in useBANDS] # old: extract_from_keys(comb_wgt, useBANDS).values()
        swarp_swglist[BAND] = [comb_wgt_tmp[band] for band in useBANDS] # old: extract_from_keys(comb_wgt, useBANDS).values()

        print "# Names for SWarp input/output are set"
        return swarp_scilist, swarp_wgtlist, swarp_swglist, swarp_flxlist, comb_sci, comb_wgt, comb_sci_tmp, comb_wgt_tmp


    def __str__(self):
        return 'Creates the Custom call to SWarp'






