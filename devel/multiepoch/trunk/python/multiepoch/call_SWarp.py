




    def setSWarpNames(self,**kwargs):

        """
        This centralized place to define and keep track of the names
        for the output products for SWArp.
        """

        print "# Setting names for SWarp calls"

        # The band to consider for the detection image
        detecBANDS = kwargs.pop('detecBANDS',['r','i','z'])
        # The magbase for flxscale, FLXSCALE = 10**(0.4*(magbase-zp))
        magbase      = kwargs.pop('magbase',30.0)
    
        # SWarp input per filter
        self.swarp_inputs  = {} # Array with all filenames
        self.swarp_scilist = {} # File with list of science names fits[0]
        self.swarp_wgtlist = {} # File with list of weight  names fits[2]
        self.swarp_magzero = {} # Array with all fluxscales in BAND
        self.swarp_flxlist = {} # File with list of fluxscales
        # SWarp outputs per filer
        self.comb_sci  = {} # SWarp coadded science image
        self.comb_wgt  = {} # SWarp coadded weight image
        self.swarp_cmd = {} # SWarp cmdline line to be executed

        # Loop over filters
        for BAND in self.BANDS:

            # Relevant indices per band
            idx = numpy.where(self.ccdimages['BAND'] == BAND)[0]

            # 1. SWarp inputs
            self.swarp_inputs[BAND]  = self.ccdimages['FILEPATH'][idx]
            self.swarp_magzero[BAND] = self.ccdimages['MAG_ZERO'][idx]
            
            ######################################
            # REMOVE LATER
            # Check that all the files actually exits
            idx_erase = []
            for k in range(len(self.swarp_inputs[BAND])):
                if not os.path.exists(self.swarp_inputs[BAND][k]):
                    print "# Skipping %s, file does not exists" % self.swarp_inputs[BAND][k]
                    idx_erase.append(k)
            tmp = numpy.delete(self.swarp_inputs[BAND],idx_erase)
            self.swarp_inputs[BAND] = tmp
            # REMOVE LATER
            ####################################

            self.swarp_scilist[BAND] = os.path.join(self.TILEDIR,"%s_%s_sci.list" % (self.tilename,BAND))
            self.swarp_wgtlist[BAND] = os.path.join(self.TILEDIR,"%s_%s_wgt.list" % (self.tilename,BAND))
            self.swarp_flxlist[BAND] = os.path.join(self.TILEDIR,"%s_%s_flx.list" % (self.tilename,BAND))

            print "# Writing science files for tile:%s band:%s on:%s" % (self.tilename,BAND,self.swarp_scilist[BAND])
            tableio.put_data(self.swarp_scilist[BAND],(self.swarp_inputs[BAND],),format="%s[0]")

            print "# Writing weight files for tile: %s band:%s on:%s" % (self.tilename,BAND,self.swarp_wgtlist[BAND])
            tableio.put_data(self.swarp_wgtlist[BAND],(self.swarp_inputs[BAND],),format="%s[2]")

            flxscale = 10.0**(0.4*(magbase - self.swarp_magzero[BAND]))
            print "# Writing fluxscale values for tile: %s band:%s on:%s" % (self.tilename,BAND,self.swarp_flxlist[BAND])
            tableio.put_data(self.swarp_flxlist[BAND],(flxscale,),format="%s")

            # 2. SWarp outputs names
            self.comb_sci[BAND] = os.path.join(self.TILEDIR,"%s_%s_sci.fits" %  (self.tilename, BAND))
            self.comb_wgt[BAND] = os.path.join(self.TILEDIR,"%s_%s_wgt.fits" %  (self.tilename, BAND))


        # The SWarp-combined detection image input and ouputs
        # Figure out which bands to use that match the detecBANDS
        useBANDS = list( set(self.BANDS) & set(detecBANDS) )
        print "# Will use %s bands for detection" % useBANDS

        # The BAND pseudo-name, we'll store with the 'real bands' as a list to access later
        self.detBAND ='det%s' % "".join(useBANDS)
        self.dBANDS = list(self.BANDS) + [self.detBAND]

        # Names and lists
        BAND = self.detBAND  # Short-cut
        self.comb_sci[BAND] = os.path.join(self.TILEDIR,"%s_%s_sci.fits" %  (self.tilename, BAND))
        self.comb_wgt[BAND] = os.path.join(self.TILEDIR,"%s_%s_wgt.fits" %  (self.tilename, BAND))
        
        # The Science and Weight lists matching the bands used for detection
        self.swarp_scilist[BAND] = extract_from_keys(self.comb_sci, useBANDS).values()
        self.swarp_wgtlist[BAND] = extract_from_keys(self.comb_wgt, useBANDS).values()

        print "# Names for SWarp input/output are set"
        return


    def makeSWarpCall(self,**kwargs):

        """ Make the SWarp call for a given TILENAME"""

        # Grab the KWARGS
        # The Breakline in case we want to break
        bkline  = kwargs.pop('breakline',"\\\n")
        # The file where we'll write the commands
        cmdfile = kwargs.pop('cmdfile',os.path.join(self.TILEDIR,"call_swarp_%s.cmd" % self.tilename))
        # The band to consider for the detection image
        detecBANDS = kwargs.pop('detecBANDS',['r','i','z'])
        # The COMBINE_TYPE for the detection image
        DETEC_COMBINE_TYPE = kwargs.pop('DETEC_COMBINE_TYPE',"CHI-MEAN")

        # Set up the names
        self.setSWarpNames(detecBANDS=detecBANDS)

        callfile = open(cmdfile, "w")
        print "# Will write SWarp call (filters+detection) to: %s" % cmdfile

        swarp_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.swarp')
        swarp_exe  = kwargs.pop('swarp_exe','swarp')

        # The SWarp options that stay the same for all tiles
        pars = {}
        pars["COMBINE_TYPE"]    = "MEDIAN"
        pars["WEIGHT_TYPE"]     = "MAP_WEIGHT"
        pars["PIXEL_SCALE"]     = "%s"  % self.COADDTILE['PIXELSCALE']
        pars["PIXELSCALE_TYPE"] = "MANUAL"
        pars["CENTER_TYPE"]     = "MANUAL"
        pars["IMAGE_SIZE"]      = "%s,%d" % (self.COADDTILE['NAXIS1'],self.COADDTILE['NAXIS2'])
        pars["CENTER"]          = "%s,%s" % (self.COADDTILE['RA'],self.COADDTILE['DEC'])
        pars["WRITE_XML"]       = "N"

        # Now update pars with kwargs
        pars = update_pars(pars,kwargs)

        # Loop over all filters
        for BAND in self.BANDS:

            # BAND speficit configurations
            pars["WEIGHT_IMAGE"]   = "@%s" % (self.swarp_wgtlist[BAND])
            pars["IMAGEOUT_NAME"]  = "%s" % self.comb_sci[BAND]
            pars["WEIGHTOUT_NAME"] = "%s" % self.comb_wgt[BAND]
            pars["FSCALE_DEFAULT"] = "@%s" % (self.swarp_flxlist[BAND])
            # Construct the call
            self.swarp_cmd[BAND] = swarp_exe + " @%s %s" % (self.swarp_scilist[BAND],bkline)
            self.swarp_cmd[BAND] = self.swarp_cmd[BAND] + " -c %s %s" % (swarp_conf,bkline)
            for param,value in pars.items():
                self.swarp_cmd[BAND] = self.swarp_cmd[BAND] + " -%s %s %s" % (param,value,bkline)
            callfile.write("%s\n" % self.swarp_cmd[BAND])

        ## Prepare the detection call now
        print "# Will write SWarp Detection call to: %s" % cmdfile
        BAND = self.detBAND
        if "FSCALE_DEFAULT" in pars : del pars["FSCALE_DEFAULT"]
        pars["COMBINE_TYPE"]   = DETEC_COMBINE_TYPE
        pars["WEIGHT_IMAGE"]   = "%s" % " ".join(self.swarp_wgtlist[BAND])
        pars["IMAGEOUT_NAME"]  = "%s" % self.comb_sci[BAND]
        pars["WEIGHTOUT_NAME"] = "%s" % self.comb_wgt[BAND]

        self.swarp_cmd[BAND] = swarp_exe + " %s %s" % (" ".join(self.swarp_scilist[BAND]),bkline)
        self.swarp_cmd[BAND] = self.swarp_cmd[BAND] + " -c %s %s" % (swarp_conf,bkline)
        for param,value in pars.items():
            self.swarp_cmd[BAND] = self.swarp_cmd[BAND] + " -%s %s %s" % (param,value,bkline)
        callfile.write("%s\n" % self.swarp_cmd[BAND])


        callfile.close()
        return

# MOVE TO utils!!!
def extract_from_keys(dictionary, keys):
    """ Returns a dictionary of a subset of keys """
    return dict((k, dictionary[k]) for k in keys if k in dictionary)

def update_pars(pars,kwargs):
    """ Simple function to update pars dictionary with kwargs in function"""
    for key, value in kwargs.iteritems():
        if key in pars.keys():
            print "# -- updating %s=%s" % (key,value)
        else:
            print "# -- adding   %s=%s" % (key,value)
        pars[key] = value
    return pars
    
