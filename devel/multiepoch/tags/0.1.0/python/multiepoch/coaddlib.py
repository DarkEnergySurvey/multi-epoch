#!/usr/bin/env python

import os,sys
import numpy
import time
import math

import matplotlib.pyplot as plt
from matplotlib.patches     import Polygon
from matplotlib.collections import PatchCollection

# DESDM modules
import despyastro
from despyastro import wcsutil
from despyastro import tableio

D2R = math.pi/180. # degrees to radians shorthand

class DEScoadd:

    """
    Handles DES information gathering using DESDM DB to perform
    multi-epoch co-addition of DECam CCDs.

    Felipe Menanteau, NCSA, Summer 2014
    """

    def __init__ (self,archive_name='desardata',verbose=False,outpath="."):
        
        self.verbose = verbose
        self.outpath = outpath

        # Avoid opening multiple connections
        try: 
            self.dbh
        except:
            # For now all connections will be to the old DB
            print "# setting up DES-DB connection(s)"
            self.dbh = despydb.desdbi.DesDbi(section="db-desoper")

        else:
            print "# Connection already open: %s" % self.dbh
            
        # Archive path
        self.get_archive_root(archive_name)
        
    def getDESTILENAMEinfo(self,tilename,tablename='felipe.coaddtile_new'):

        """
        Builds a dictionary/header with all of the information on the
        COADDTILE_XXXX table for a given TILENAME (input). It store
        the RAC[1,4] and DECC[1,4] corners of the TILENAME an computes and stores the:
        (TILE_RACMIN, TILE_RACMAX) and (TILE_DECCMIN,TILE_DECCMAX) for that tile.

        We will use this dictionary to make the head file later.

        -- We won't need the *.head file in the future --

        """

        # We set up the tilename in the class
        self.tilename = tilename
        
        # Simple query
        query = "SELECT * from %s where TILENAME='%s'" % (tablename,tilename)
        
        if self.verbose: print "# Getting TILENAME information for %s\n%s\n" %(tilename,query)
        cur = self.dbh.cursor()
        cur.execute(query)
        desc = [d[0] for d in cur.description] # cols description
        line = cur.fetchone()
        cur.close()
        
        # Make a dictionary/header for the all columns from COADDTILE table
        self.COADDTILE = dict(zip(desc, line))

        # Lower-case for compatibility with wcsutils
        for k, v in self.COADDTILE.items():
            self.COADDTILE[k.lower()] = v

        # The minimum values for the tilename
        ras  = numpy.array([self.COADDTILE['RAC1'], self.COADDTILE['RAC2'], self.COADDTILE['RAC3'], self.COADDTILE['RAC4'] ])
        decs = numpy.array([self.COADDTILE['DECC1'],self.COADDTILE['DECC2'],self.COADDTILE['DECC3'],self.COADDTILE['DECC4']])

        if self.COADDTILE['CROSSRAZERO'] == 'Y':
            # Maybe we substract 360?
            self.TILE_RACMIN  = ras.max()
            self.TILE_RACMAX  = ras.min()
        else:
            self.TILE_RACMIN  = ras.min()
            self.TILE_RACMAX  = ras.max()
            
        self.TILE_DECCMIN = decs.min()
        self.TILE_DECCMAX = decs.max()

        # Store the packed corners of the COADDTILES for plotting later
        self.TILE_RACS  = ras  #numpy.append(ras,ras[0])
        self.TILE_DECCS = decs #numpy.append(decs,decs[0])
        return 

    def findCCDinDESTILENAME(self,tilename,
                          and_constraints="",
                          from_constraints="",
                          select_constraints=""):
        """
        Finds all of the CCCs in the IMAGE table that fall inside the
        (RACMI,RACMAX) and (DECCMIN,DECCMAX) of a DES tile with a
        given set of extras SQL and/or constraints provided into the
        function. 
        
        Here we consider the simple case in which the CCDs are always
        smaller that the DESTILE footprint, and therefore in order to
        have overlap at least on the corners need to be inside. We'll
        consider the more general case where the any TILE size (x or
        y) is smaller that the CCDs later.
        
        In order for a CCD image to be considered inside the footprint
        of a DES tile it needs to satifies:

        either:

        (RAC1,DEC1) or (RAC2,DECC2) or (RAC3,DECC3) or (RAC4,DECC4)
        have to been inside
        (TILE_RACMIN,TILE_RACMAX) and (TILE_DECCMIN, TILE_DECCMAX),
        which are the min and max boundaries of a DES tile.

        The following driagram describes how this works:
        
 
                                                                  (RAC4,DEC4)                          (RAC3,DEC3)
                                                                   +----------------------------------+         
                                                                   |                                  |         
              Corner4                                       Corner |                                  |         
                +----------------------------------------------+   |                                  |         
                |                                              |   |     Corner2                      |         
                |                                              |   |                                  |         
                |                DES TILE                      |   |  CCD outside (all corners out)   |         
                |                                              |   +----------------------------------+         
                |                                              |  (RAC1,DEC1)                          (RAC2,DEC2)
    (RAC4,DEC4) |                        (RAC3,DEC3)           |       
       +--------|-------------------------+                    |
       |        | CCD inside              |                    |
       |        | (2 corners in)          |                    |
       |        |                         |                    |
       |        |                         |                    |
       |        |                         |                    |
       |        |                         |                    |
       +----------------------------------+                    |
   (RAC1,DEC1)  |                       (RAC2,DEC2)            |
                |                                              |
                |                                  (RAC4,DEC4) |                        (RAC3,DEC3)
                |                                     +--------|-------------------------+
                |                                     |        |                         |
                |                                     |        |                         |
                +----------------------------------------------+                         |
              Corner1                                 |     Corner2                      |
                                                      |                                  |
                                                      | CCD inside (one corner in)       |
                                                     +----------------------------------+
                                                    (RAC1,DEC1)                          (RAC2,DEC2) 

                                                    
        TILE_RACMIN  = min(RA Corners[1,4])
        TILE_RACMAX  = man(RA Conrers[1,4])
        TILE_DECCMIN = min(DEC Corners[1,4])
        TILE_DECCMAX = man(DEC Corners[1,4])

        **** Note1: We need to do something about inverting MIN,MAX when crossRAzero='Y' ****

        **** Note2: We need add the general query, when the tile is smaller than the CCDs ****

        """

        self.tilename = tilename

        ############################################################################################################
        # For simplicity, here we build the query blocks (checkC[1,4]) as string that will check if each of the CCD corners:
        # (RAC[1,4], DEC[1,4]) is inside the TILE footprint as defined by (TILE_RACMIN, TILE_RACMAX) and (TILE_DECMIN,TILE_DECMAX).
        tile_corners = (self.TILE_RACMIN,self.TILE_RACMAX,self.TILE_DECCMIN,self.TILE_DECCMAX) # A short cut
        checkC1 = "(c.RAC1 between %.10f and %.10f) and (c.DECC1 between %.10f and %.10f)" % tile_corners
        checkC2 = "(c.RAC2 between %.10f and %.10f) and (c.DECC2 between %.10f and %.10f)" % tile_corners
        checkC3 = "(c.RAC3 between %.10f and %.10f) and (c.DECC3 between %.10f and %.10f)" % tile_corners
        checkC4 = "(c.RAC4 between %.10f and %.10f) and (c.DECC4 between %.10f and %.10f)" % tile_corners

        ############################################
        # Re-format the constraints -- if defined
        if and_constraints:
            and_constraints = "and %s" % and_constraints
        if select_constraints:
            select_constraints = "%s," % select_constraints
        if from_constraints:
            from_constraints = ",%s" % from_constraints

        ######################################################################
        # Merge main query + constraints + checksC[1,4] + IMAGECORNERS table
        # For now we need to merge with the IMAGECORNERS table, until
        # the new schema is merged into the IMAGE table

        query = """SELECT %s
         filepath_desar.PATH,
         image.IMAGENAME, image.BAND, image.RUN, image.PROJECT, image.IMAGETYPE, image.ID,
         c.RA, c.RAC1, c.RAC2, c.RAC3, c.RAC4, c.DEC, c.DECC1, c.DECC2, c.DECC3, c.DECC4
         from IMAGE, felipe.IMAGECORNERS c, FILEPATH_DESAR %s
          where image.IMAGENAME=c.IMAGENAME and
                filepath_desar.ID=IMAGE.ID
           %s
           and ((%s) or
                (%s) or
                (%s) or
                (%s) ) """ % (select_constraints, from_constraints,and_constraints,checkC1,checkC2,checkC3,checkC4)
        if self.verbose: print "# Getting images within the tile: %s\n %s" % (tilename,query)
        # Get the images that are part of the DESTILE
        self.ccdimages = despyastro.genutil.query2dict_of_columns(query,dbhandle=self.dbh,array=True)

        # Get the filters we found
        self.BANDS = numpy.unique(self.ccdimages['BAND'])
        self.NBANDS = len(self.BANDS)

        # Finally, make sure the output directories exist
        self.checkTILEDIR()
        return


    def repackCCDcorners(self,filename):

        """
        Repacks the CCD corners for plotting, from the existing
        dictionary of CCD images that fall inside the tile
        """
        ccds = self.ccdimages # short-cut
        k = numpy.where(ccds['IMAGENAME'] == filename)[0][0]
        ras   = numpy.array([ccds['RAC1'][k], ccds['RAC2'][k], ccds['RAC3'][k], ccds['RAC4'][k]])
        decs  = numpy.array([ccds['DECC1'][k],ccds['DECC2'][k],ccds['DECC3'][k],ccds['DECC4'][k]])
        return ras,decs
        
    def plot_CCDcornersDESTILEsubplot(self,fignumber=1,figsize=20,show=False,**kwargs):
        
        """ Plot the CCDs overlaping the DESTILENAME using subplots"""

        # Figure out the layout depending on the number of filters
        # found in the overlapping ares
        ncols = 3
        if self.NBANDS > 3:
            nrows = 2
        else:
            nrows = 1

        # Figure out the aspects ratios for the main and sub-plots
        plot_aspect    = float(nrows)/float(ncols)
        subplot_aspect = 1/math.cos(D2R*self.COADDTILE['DEC'])

        # The main figure
        fig = plt.figure(fignumber,figsize=(figsize,figsize*plot_aspect))
        
        # Initialize limits for all plots
        x1 =  1e10
        x2 = -1e10
        y1 = x1
        y2 = x2
        
        kplot = 1
        for BAND in self.BANDS:
            plt.subplot(nrows,ncols,kplot)
            
            ax = plt.gca()
            idx = numpy.where(self.ccdimages['BAND'] == BAND)[0]
            filenames = self.ccdimages['IMAGENAME'][idx]
            NCCDs = len(filenames)
            
            print "# Found %s CCDimages for filter %s overlaping " % (NCCDs, BAND)
            for filename in filenames:
                ras,decs = self.repackCCDcorners(filename)
                P1 = Polygon( zip(ras,decs), closed=True, hatch='',lw=0.2,alpha=0.1,Fill=True,color='k')
                P2 = Polygon( zip(ras,decs), closed=True, hatch='',lw=0.2,Fill=False,color='k') 
                ax.add_patch(P1)
                ax.add_patch(P2)

                # Keep track of the limits as Polygon as trouble
                x1 = min(x1,ras.min())
                x2 = max(x2,ras.max())
                y1 = min(y1,decs.min())
                y2 = max(y2,decs.max())
                
            # Draw the TILE footprint at the end
            P = Polygon( zip(self.TILE_RACS,self.TILE_DECCS), closed=True, Fill=False, hatch='',lw=1.0, color='r') 
            ax.add_patch(P)
        
            # Fix range
            plt.xlim(x1,x2)
            plt.ylim(y1,y2)
            plt.title("%s - %s band (%s images)" % (self.tilename,BAND,NCCDs))
            plt.xlabel("RA (degrees)")
            plt.ylabel("Decl. (degrees)")
            ax.set_aspect(subplot_aspect)
            kplot = kplot + 1

        fig.set_tight_layout(True) # This avoids backend warnings.
        figname = os.path.join(self.TILEDIR,"%s_overlap.pdf" % self.tilename)
        fig.savefig(figname)
        if show: plt.show()
        print "# Wrote: %s" % figname
        return
 
    def plot_CCDcornersDESTILEsingle(self,BAND,fignumber=1,**kwargs):

        """ Plot the CCD corners of overlapping images for the tilename"""

        aspect = math.cos(D2R*self.COADDTILE['DEC'])
        plt.figure(fignumber,figsize=(10,10*aspect))
        ax = plt.gca()

        idx = numpy.where(self.ccdimages['BAND'] == BAND)[0]
        filenames = self.ccdimages['IMAGENAME'][idx]
        NCCDs = len(filenames)

        # Initialize limits
        x1 =  1e10
        x2 = -1e10
        y1 = x1
        y2 = x2

        print "# Found %s CCDimages for filter %s overlaping " % (NCCDs, BAND)
        for filename in filenames:
            ras,decs = c.repackCCDcorners(filename)
            P1 = Polygon( zip(ras,decs), closed=True, hatch='',lw=0.2,alpha=0.1,Fill=True,color='k')
            P2 = Polygon( zip(ras,decs), closed=True, hatch='',lw=0.2,Fill=False,color='k') 
            ax.add_patch(P1)
            ax.add_patch(P2)

            # Keep track of the limits as Polygon as trouble
            x1 = min(x1,ras.min())
            x2 = max(x2,ras.max())
            y1 = min(y1,decs.min())
            y2 = max(y2,decs.max())

        # Draw the TILE footprint at the end
        P = Polygon( zip(self.TILE_RACS,self.TILE_DECCS), closed=True, Fill=False, hatch='',lw=1.0, color='r') 
        ax.add_patch(P)
        
        # Fix range
        plt.xlim(x1,x2)
        plt.ylim(y1,y2)
        plt.title("%s - %s band (%s images)" % (self.tilename,BAND,NCCDs))
        plt.xlabel("RA (degrees)")
        plt.ylabel("Decl. (degrees)")

        return


    def checkTILEDIR(self):

        """
        Check that the TILEDIR exist and setup directory
        """
        self.TILEDIR = os.path.join(self.outpath,self.tilename)
        if not os.path.exists(self.TILEDIR):
            print "# Creating %s" % self.TILEDIR
            os.makedirs(self.TILEDIR)
        return

    def collectFILESforSWarp(self,**kwargs):

        """
        Collect the files and their absolute paths to start the SWarping process,
        it creates a dictionary: self.swarp_inputs[BAND] keyed to the BAND and a
        numpy str-array: self.ccdimages['FILEPATH'] that cointains the filepath for each entry.

        Useful to testing:
        
        We can change the archive_root from the default=/archive_data/Archive
        to any custom place we made. We can also add a prefix the filenames.

        """

        # Grab the KWARGS
        # in case we want to tweak the default location
        archive_root = kwargs.pop('archive_root',self.archive_root)
        # in case we want a prefix modifier -- REMOVE IN THE FUTURE
        prefix       = kwargs.pop('prefix',None)

        # in case we want to tweak the default location
        if not archive_root:
            archive_root = self.archive_root

        ###################################################
        # REMOVE LATER -- ONLY NEEDED for tweaked paths
        # Get the filename path for each file
        filepath = [] # list to store
        Nimages = len(self.ccdimages['IMAGENAME'])
        print "# Will find paths for images %s" % Nimages
        for k in range(Nimages):
            path = self.ccdimages['PATH'][k]
            # in case we want to tweak the default location
            if prefix : 
                p1,p2 = os.path.split(path)
                path = os.path.join(p1,"%s%s" % (prefix,p2))
                
            location = os.path.join(archive_root,path)
            filepath.append(location)

        # Make it a numpy array the default paths
        self.ccdimages['FILEPATH'] = numpy.array(filepath)
        #####################################################

        ###########################################
        # IN THE FUTURE IT WOULD BE ENOUGH TO DO:
        #apath = [archive_root]*Nimages
        #self.ccdimages['FILEPATH'] = numpy.core.defchararray.add(apath,self.ccdimages['PATH'])
        # and for numpy >= 1.8
        #self.ccdimages['FILEPATH'] = numpy.add(apath,self.ccdimages['PATH'])
        ###########################################

        return

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


    def setCatNames(self,**kwargs):

        """ Set the names for input/ouput for psfex/Sextractor calls"""

        print "# Setting names for SExPSF/psfex/SExDual"

        # SExPSF
        self.psfcat = {}
        self.psf    = {}
        self.SExpsf_cmd = {}
        # PsfCall
        self.psfex_cmd  = {}
        self.psfexxml     = {}
        # SExDual
        self.SExDual_cmd = {}
        self.checkimage  = {}
        self.cat = {}
        
        for BAND in self.dBANDS:
            # SExPSF
            self.psf[BAND]       = os.path.join(self.TILEDIR,"%s_%s_psfcat.psf"  %  (self.tilename, BAND))
            self.psfcat[BAND]    = os.path.join(self.TILEDIR,"%s_%s_psfcat.fits" %  (self.tilename, BAND))
            # psfex
            self.psfexxml[BAND]  = os.path.join(self.TILEDIR,"%s_%s_psfex.xml"   %  (self.tilename, BAND))
            # SExDual
            self.cat[BAND]       = os.path.join(self.TILEDIR,"%s_%s_cat.fits"    %  (self.tilename, BAND))
            self.checkimage[BAND]= os.path.join(self.TILEDIR,"%s_%s_seg.fits"    %  (self.tilename, BAND))
        print "# Done"
        return
    


    def get_archive_root(self,archive_name='desardata'):

        """ Gets the archive root -- usually /archive_data/Archive """
        query = "select archive_root from archive_sites where location_name='%s'"  % archive_name
        print "# Getting the archive root name for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur = self.dbh.cursor()
        cur.execute(query)
        self.archive_root = cur.fetchone()[0]
        cur.close()
        return

    def makeheadDESTILENAME(self,tilename=None,PROJ='TAN'):

        """
        Created the head SWarp file for the DES tilename.  This
        function reused the information store in the self.COADDTILE
        dictionary/header produced by the funtion: getDESTILENAMEinfo,
        which of course need to be called first!
        """

        if not tilename:
            tilename=self.tilename

        self.headfile = os.path.join(self.TILEDIR,"%s.head" % tilename)
        
        print "# Will create head file: %s" % self.headfile
        # Populate header for Projection
        if PROJ == 'TAN':
            self.COADDTILE['NAXIS']  = 2
            self.COADDTILE['CTYPE1'] = 'RA---TAN'
            self.COADDTILE['CTYPE2'] = 'DEC---TAN'
            self.COADDTILE['CUNIT1'] = 'deg'
            self.COADDTILE['CUNIT2'] = 'deg'
        else:
            sys.exit("# ERROR: Unknown projection %s" % PROJ)

        # These are the keys we want for the .head file
        keys = ['NAXIS', 
                'NAXIS1',
                'NAXIS2',
                'CTYPE1',
                'CTYPE2',
                'CUNIT1',
                'CUNIT2',
                'CRVAL1',
                'CRPIX1',
                'CD1_1', 
                'CD1_2', 
                'CRVAL2',
                'CRPIX2',
                'CD2_1', 
                'CD2_2'] 

        head = open(self.headfile,"w")
        for key in keys:
            if   isinstance(self.COADDTILE[key],str):
                format = "%-8s= '%s'"
            elif isinstance(self.COADDTILE[key],float):
                format = "%-8s= %.10f"
            elif isinstance(self.COADDTILE[key],int):
                format = "%-8s= %d"
            else:
                format = "%-8s= %s"
            head.write(format % (key,self.COADDTILE[key]) + "\n")
        head.close()


    def makeStiffCall(self,**kwargs):

        """ Make a color tiff of the TILENAME using stiff"""

        if self.NBANDS < 3:
            print "# Not enough filters to create color image"
            return 
        
        # The Breakline in case we want to break
        bkline  = kwargs.pop('breakline',"\\\n")
        # The file where we'll write the commands
        cmdfile = kwargs.pop('cmdfile',os.path.join(self.TILEDIR,"call_stiff_%s.cmd" % self.tilename))
        callfile = open(cmdfile, "w")
        print "# Will write stiff call to: %s" % cmdfile

        stiff_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.stiff')
        stiff_exe  = kwargs.pop('stiff_exe','stiff')
        self.color_tile =  os.path.join(self.TILEDIR,"%s.tif" % self.tilename)
        
        pars = {}
        pars['OUTFILE_NAME']     =  self.color_tile
        pars['COMPRESSION_TYPE'] =  'JPEG'
        # Now update pars with kwargs
        pars = update_pars(pars,kwargs)
        
        # Define the color filter sets we'd like to use, by priority depending on what BANDS will be combined
        cset1 = ['i','r','g']
        cset2 = ['z','r','g']
        cset3 = ['z','i','g']
        cset4 = ['z','i','r']
        csets = (cset1,cset2,cset3,cset4)
        CSET = False
        for color_set in csets:
            if CSET: break
            inset = list( set(color_set) & set(self.BANDS))
            if len(inset) == 3:
                CSET = color_set

        if not CSET:
            print "# Could not find a suitable filter set for color image"
            return 
        
        color_cmd = "%s " % stiff_exe
        for BAND in CSET:
            color_cmd = color_cmd + "%s " % self.comb_sci[BAND]
        color_cmd = color_cmd + " -c %s %s" % (stiff_conf,bkline)
        for param,value in pars.items():
            color_cmd = color_cmd + " -%s %s %s" % (param,value,bkline)
        callfile.write("%s\n" % color_cmd)
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


    # # --- DEPRECATED ----###
    # def makeSWarpDetecCall(self,**kwargs):
    #
    #     """ Make the call to make the detection image with SWarp"""
    #
    #     # Grab the KWARGS
    #     # The Breakline in case we want to break
    #     bkline  = kwargs.pop('breakline',"\\\n")
    #     # The file where we'll write the commands
    #     cmdfile = kwargs.pop('cmdfile',os.path.join(self.TILEDIR,"call_swarpDetec_%s.cmd" % self.tilename))
    #
    #
    #     callfile = open(cmdfile, "w")
    #     print "# Will write SWarp Detection call to: %s" % cmdfile
    #
    #     # Figure out which bands to use that match the detecBANDS
    #     useBANDS = list( set(self.BANDS) & set(detecBANDS) )
    #     print "# Will use %s bands for detection" % useBANDS
    #
    #     # The BAND pseudo-name, we'll store with the 'real bands' as a list to access later
    #     BAND='det%s' % "".join(useBANDS)
    #     self.detBAND = BAND
    #     self.dBANDS = list(self.BANDS) + [self.detBAND]
    #
    #     self.comb_sci[BAND] = os.path.join(self.TILEDIR,"%s_%s_sci.fits" %  (self.tilename, BAND))
    #     self.comb_wgt[BAND] = os.path.join(self.TILEDIR,"%s_%s_wgt.fits" %  (self.tilename, BAND))
    #
    #     # The Science and Weight lists matching the bands used for detection
    #     scilist = extract_from_keys(self.comb_sci, useBANDS).values()
    #     wgtlist = extract_from_keys(self.comb_wgt, useBANDS).values()
    #
    #     # Create the detection swarp call
    #     swarp_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.swarp')
    #     swarp_exe  = 'swarp'
    #
    #     # The SWarp options that stay the same for all tiles
    #     pars = {}
    #     pars["COMBINE_TYPE"]    = "CHI-MEAN"
    #     pars["WEIGHT_TYPE"]     = "MAP_WEIGHT"
    #     pars["PIXEL_SCALE"]     = "%s"  % self.COADDTILE['PIXELSCALE']
    #     pars["PIXELSCALE_TYPE"] = "MANUAL"
    #     pars["CENTER_TYPE"]     = "MANUAL"
    #     pars["IMAGE_SIZE"]      = "%s,%d" % (self.COADDTILE['NAXIS1'],self.COADDTILE['NAXIS2'])
    #     pars["CENTER"]          = "%s,%s" % (self.COADDTILE['RA'],self.COADDTILE['DEC'])
    #     pars["NTHREADS"]        = "%s" % "8" 
    #     # BAND spefic configurations
    #     pars["WEIGHT_IMAGE"]    = "%s" % " ".join(wgtlist)
    #     pars["IMAGEOUT_NAME"]   = "%s" % self.comb_sci[BAND]
    #     pars["WEIGHTOUT_NAME"]  = "%s" % self.comb_wgt[BAND]
    #     # Now update pars with kwargs
    #     pars = update_pars(pars,kwargs)
    #
    #     # Build the SWarp call
    #     self.swarp_cmd[BAND] = swarp_exe + " %s %s" % (" ".join(scilist),bkline)
    #     self.swarp_cmd[BAND] = self.swarp_cmd[BAND] + " -c %s %s" % (swarp_conf,bkline)
    #     for param,value in pars.items():
    #         self.swarp_cmd[BAND] = self.swarp_cmd[BAND] + " -%s %s %s" % (param,value,bkline)
    #     callfile.write("%s\n" % self.swarp_cmd[BAND])
    #     callfile.close()
    #     return

    def makeSExpsfCall(self,**kwargs):

        """ Build/Execute the SExtractor call for psf on the detection image"""

        # Grab the KWARGS
        # The Breakline in case we want to break
        bkline  = kwargs.pop('breakline',"\\\n")
        # The file where we'll write the commands
        cmdfile = kwargs.pop('cmdfile',os.path.join(self.TILEDIR,"call_SExpsf_%s.cmd" % self.tilename))

        callfile = open(cmdfile, "w")
        print "# Will write SEx psf call to: %s" % cmdfile

        # SEx configuration
        sex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.sex')
        sex_exe  = kwargs.pop('sex_exe','sex')
        
        pars = {}
        pars['CATALOG_TYPE']    = "FITS_LDAC"
        pars['WEIGHT_TYPE']     = 'MAP_WEIGHT'
        pars['PARAMETERS_NAME'] = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param_psfex')
        pars['FILTER_NAME']     = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.conv')
        pars['STARNNW_NAME']    = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.nnw')
        pars['SATUR_LEVEL']     = 65000
        pars['VERBOSE_TYPE']    = 'NORMAL'
        pars['DETECT_MINAREA']  = 3 # WHY??????????? son small!!!!

        # Now update pars with kwargs
        pars = update_pars(pars,kwargs)
        
        # Loop over all bands and Detection
        for BAND in self.dBANDS:
            pars['WEIGHT_IMAGE'] = self.comb_wgt[BAND]
            pars['CATALOG_NAME'] = self.psfcat[BAND]
            # Build the call
            self.SExpsf_cmd[BAND] = sex_exe + " %s %s" % (self.comb_sci[BAND],bkline)
            self.SExpsf_cmd[BAND] = self.SExpsf_cmd[BAND] + " -c %s %s" % (sex_conf,bkline)
            for param,value in pars.items():
                self.SExpsf_cmd[BAND] = self.SExpsf_cmd[BAND] + " -%s %s %s" % (param,value,bkline)
            callfile.write("%s\n" % self.SExpsf_cmd[BAND])

        callfile.close()
        return

    # Next run psfex on coadd images and detection
    def makepsfexCall(self,**kwargs):

        """ Build/Execute the psf call"""

        # Grab the KWARGS
        # The Breakline in case we want to break
        bkline  = kwargs.pop('breakline',"\\\n")
        # The file where we'll write the commands
        cmdfile = kwargs.pop('cmdfile',os.path.join(self.TILEDIR,"call_psfex_%s.cmd" % self.tilename))

        callfile = open(cmdfile, "w")
        print "# Will write psf call to: %s" % cmdfile

        # SEx configuration
        psfex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.psfex')
        psfex_exe  = kwargs.pop('psfex_exe','psfex')

        pars = {}
        pars['WRITE_XML'] = 'Y'
        pars = update_pars(pars,kwargs)

        for BAND in self.dBANDS:
            pars['XML_NAME'] = self.psfexxml[BAND]
            # Build the call
            self.psfex_cmd[BAND] = psfex_exe + " %s -c %s %s" % (self.psfcat[BAND],psfex_conf,bkline)
            for param,value in pars.items():
                self.psfex_cmd[BAND] = self.psfex_cmd[BAND] + " -%s %s %s" % (param,value,bkline)
            callfile.write("%s\n" % self.psfex_cmd[BAND])

        callfile.close()
        return


    def makeSExDualCall(self,**kwargs):

        """ Build/Execute the SEx dual-mode call """

        # Grab the KWARGS
        # The Breakline in case we want to break
        bkline  = kwargs.pop('breakline',"\\\n")
        # The file where we'll write the commands
        cmdfile = kwargs.pop('cmdfile',os.path.join(self.TILEDIR,"call_SExDual_%s.cmd" % self.tilename))

        callfile = open(cmdfile, "w")
        print "# Will write SEx Dual call to: %s" % cmdfile

        # SEx configuration
        sex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.sex')
        sex_exe  = kwargs.pop('sex_exe','sex')

        # General pars, BAND-independent
        pars = {}
        pars['CATALOG_TYPE']    = "FITS_LDAC"
        pars['FILTER_NAME']     = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.conv')
        pars['STARNNW_NAME']    = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.nnw')
        pars['CATALOG_TYPE']    =   'FITS_1.0'
        pars['WEIGHT_TYPE']     = 'MAP_WEIGHT'
        pars['MEMORY_BUFSIZE']  = 2048
        pars['CHECKIMAGE_TYPE'] = 'SEGMENTATION'
        pars['DETECT_THRESH']   = 1.5
        pars['DEBLEND_MINCONT'] = 0.001 
        #pars['PARAMETERS_NAME'] = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param')
        pars['PARAMETERS_NAME'] = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param_psfonly')
        pars['VERBOSE_TYPE']    = 'NORMAL'

        # Now update pars with kwargs
        pars = update_pars(pars,kwargs)


        dBAND = self.detBAND
        for BAND in self.BANDS:
            
            pars['MAG_ZEROPOINT']   =  30.0000 
            pars['CATALOG_NAME']    =  self.cat[BAND]           
            pars['PSF_NAME']        =  self.psf[BAND]
            pars['CHECKIMAGE_NAME'] =  self.checkimage[BAND]
            pars['WEIGHT_IMAGE']    =  "%s,%s" % (self.comb_sci[dBAND],self.comb_sci[BAND])

            self.SExDual_cmd[BAND] = sex_exe + " %s,%s %s" % (self.comb_sci[dBAND],self.comb_sci[BAND],bkline)
            self.SExDual_cmd[BAND] = self.SExDual_cmd[BAND] + " -c %s %s" % (sex_conf,bkline)
            
            for param,value in pars.items():
                self.SExDual_cmd[BAND] = self.SExDual_cmd[BAND] + " -%s %s %s" % (param,value,bkline)
            callfile.write("%s\n" % self.SExDual_cmd[BAND])

        callfile.close()
        return

# MOVE TO despymisc!!!
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
    
