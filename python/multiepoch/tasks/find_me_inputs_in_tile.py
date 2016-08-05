#!/usr/bin/env python

"""

Option A: Distance Search Method
--------------------------------

|-----------RA_SIZE_TILE ---------------|
       
     +---------------------------------------+       - 
     |                                       |       | 
     |                                       |       | 
     |                                       |       | 
     |                                       |       | 
     |                                       |       | 
     |                                       |       | 
     |      RA_CENT_TILE, DEC_CENT_TILE      |       | 
     |                                       |  DEC_SIZE_TILE
     |                  +                    |       |                   _
     |                                       |       |                   |
     |                                       |       |                   |
     |                                       |       |                   |
     |                                       |       |                   dy
     |                         +----------------------------------+      |   
     |                         |             |       |            |      |
     |                         |             |       |            |      |
     +---------------------------------------+       -            |      |
                               |                +                 |      -
                               |     RA_CENT_CCD,DEC_CENT_CCD     |
                               |                                  |
                               |                                  |
                               +----------------------------------+        
                                  
                               |<------   RA_SIZE_CCD    -------->|
 
                       
                        |<-------- dx ------->|
 
                     
 dx=abs(RA_CENT_TILE  - RA_CENT_CCD)/2
 dy=abs(DEC_CENT_TILE - DEC_CENT_CCD)/2
 
Condition to be satisfied:
 dx < (RA_SIZE_TILE  + RA_SIZE_CCD)/2 AND
 dy < (DEC_SIZE_TILE + DEC_SIZE_CCD)/2


Option B: CORNERS' Seach Method
---------------------------------

Finds all of the CCCs in the IMAGE table that fall inside the (RACMI,RACMAX)
and (DECCMIN,DECCMAX) of a DES tile with a given set of extras SQL and/or
constraints provided into the function. 

Here we consider the simple case in which the CCDs are always smaller that the
DESTILE footprint, and therefore in order to have overlap at least on the
corners need to be inside. We'll consider the more general case where the any
TILE size (x or y) is smaller that the CCDs later.

In order for a CCD image to be considered inside the footprint of a DES tile it
needs to satifies:

either:

(RAC1,DEC1) or (RAC2,DECC2) or (RAC3,DECC3) or (RAC4,DECC4) have to been inside
(TILE_RACMIN,TILE_RACMAX) and (TILE_DECCMIN, TILE_DECCMAX), which are the min
and max boundaries of a DES tile.

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
(RAC1,DEC1)  |                       (RAC2,DEC2)           |
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

Author: Felipe Menanteau, NCSA, Nov 2014.

"""

import json
import numpy
import time
import os,sys
import pandas as pd
import despyastro

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider
from mojo.utils import log as mojo_log

# Mutiepoch loads
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
import multiepoch.querylibs as querylibs
from multiepoch import file_handler as fh

# -----------------------------------------------------------------------------
# Changing default to empty strings for now
SELECT_EXTRAS = ""
FROM_EXTRAS   = ""
AND_EXTRAS    = ""
CATS_SELECT_EXTRAS = ""
CATS_FROM_EXTRAS   = ""
CATS_AND_EXTRAS    = ""
SEARCH_TYPE   = "distance"
ZP_SOURCE  = "GCM"
ZP_VERSION = "Y1A1_gruendlhack"
# -----------------------------------------------------------------------------

class Job(BaseJob):

    '''
    DESDM multi-epoch pipeline : QUERY TILEINFO JOB
    ===============================================

    Required INPUT from context (ctx):
    ``````````````````````````````````
    - tile_edges: tuple of four floats (RACMIN, RACMAX, DECCMIN, DECCMAX)

    Writes as OUTPUT to context (ctx):
    ``````````````````````````````````
    - assoc
    '''

    
    class Input(IO):


        """Find the CCDs that fall inside a tile"""

        # Required inputs to run the job (in ctx, after loading files)
        # because we set the argparse keyword to False they are not interfaced
        # to the command line parser
        tileinfo = Dict(None, help="The tileinfo dictionary.",argparse=False)
        tilename = Unicode(None, help="The name of the tile.",argparse=False)

        tile_geom_input_file = CUnicode('',help='The json file with the tile information',
                # declare this variable as input_file, this leads the content of the file to be loaded into the ctx at initialization
                input_file=True,
                # set argtype=positional !! to make this a required positional argument when using the parser
                argparse={ 'argtype': 'positional', })

        # Optional inputs, also when interfaced to argparse
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        db_section    = CUnicode("db-destest",help="DataBase Section to connect", 
                                 argparse={'choices': ('db-desoper','db-destest', )} )
        # Blacklist and zeropoint
        no_blacklist  = Bool(False, help=("Do not Black list images"))
        no_zeropoint  = Bool(False, help=("Do not get ZP for images"))
        zp_source     = CUnicode(ZP_SOURCE,help="ZEROPOINT.SOURCE",)
        zp_version    = CUnicode(ZP_VERSION,help="ZEROPOINT.VERSION",)
        zp_force      = CFloat(32.2,help="Force ZP value if MAG_ZEROP not present [default=32.2]",)

        select_extras = CUnicode(SELECT_EXTRAS,help="string with extra SELECT for query",)
        and_extras    = CUnicode(AND_EXTRAS,help="string with extra AND for query",)
        from_extras   = CUnicode(FROM_EXTRAS,help="string with extra FROM for query",)
        search_type   = CUnicode(SEARCH_TYPE,help="Search type to perform (distances or corners)",
                                 argparse={'choices': ('distance','corners')} )
        ccdnum        = CInt(0,help="Query image table for CCDNUM",)
        tagname       = CUnicode('Y2T9_FINALCUT_V2',help="TAGNAME for images in the database",)
        assoc_file    = CUnicode("", help=("Name of the output ASCII association file where we will store the cccds information for coadd"))
        cats_file     = CUnicode("", help=("Name of the output ASCII catalog list storing the information for scamp"))
        segs_file     = CUnicode("", help=("Name of the output ASCII catalog list storing the SEG files for meds "))
        bkgs_file     = CUnicode("", help=("Name of the output ASCII catalog list storing the BKG files for meds "))
        super_align   = Bool(False, help=("Run super-aligment of tile using scamp"))
        use_scampcats = Bool(False, help=("Use finalcut scampcats for super-alignment"))
        
        # extras for the cats' queries -- separated from the CCDs as we do not want to match those for scamp 
        cats_select_extras = CUnicode(CATS_SELECT_EXTRAS,help="string with extra SELECT for query",)
        cats_and_extras    = CUnicode(CATS_AND_EXTRAS,help="string with extra AND for query",)
        cats_from_extras   = CUnicode(CATS_FROM_EXTRAS,help="string with extra FROM for query",)

        # SN Tag
        sn_proctag         = CUnicode("", help=("SN PROCTAG.TAG (Optional for SN coadds)"))

        plot_outname  = CUnicode("", help=("Output file name for plot, in case we want to plot"))
        local_archive = CUnicode("", help="The local filepath where the input fits files (will) live")
        dump_assoc    = Bool(False, help=("Dump the assoc file?"))
        dump_cats     = Bool(False, help=("Dump the catlist?"))
        dump_assoc_meds = Bool(False, help=("Dump the assocs file for MEDS?"))

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        def _validate_conditional(self):

            # Get logger
            logger = mojo_log.get_logger({})

            # Check for valid output assoc_file
            if self.mojo_execution_mode == 'job as script' and self.assoc_file == "":
                mess = 'If job is run standalone assoc_file cannot be ""'
                raise IO_ValidationError(mess)

            if self.cats_file != "" and not self.super_align:
                logger.info("Updating super_align value to True")
                self.super_align = True
            
            if self.mojo_execution_mode == 'job as script' and self.cats_file == "" and self.super_align:
                mess = 'If job is run standalone and super_align is True then cats_file cannot be "" (empty string)'
                raise IO_ValidationError(mess)

            # Check for valid local_archive if not in the NCSA cosmology cluster
            if not utils.inDESARcluster(logger=logger) and self.local_archive == "": 
                mess = 'If not in cosmology cluster local_archive cannot be empty [""]'
                raise IO_ValidationError(mess)

            # Make sure that some of the extras statements have the proper endingh
            if len(self.select_extras) != 0:
                if self.select_extras[-1] !=",": self.select_extras = self.select_extras+","
            if len(self.from_extras) != 0:
                if self.from_extras[-1] !=",": self.from_extras = self.from_extras+","

            if len(self.cats_from_extras) != 0:
                if self.cats_from_extras[-1] !=",": self.cats_from_extras = self.cats_from_extras+","
            if len(self.cats_select_extras) != 0:
                if self.cats_select_extras[-1] !=",": self.cats_select_extras = self.cats_select_extras+","

            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

            # if SN PROCTAG is present we overide the query information to reflect the tag we want
            if self.sn_proctag != '':
                self.and_extras  = self.and_extras + "PROCTAG.pfw_attempt_id=me.pfw_attempt_id AND PROCTAG.TAG='%s' AND" % self.sn_proctag
                self.from_extras = self.from_extras + "PROCTAG,"
                self.cats_from_extras = self.from_extras
                self.cats_and_extras = self.and_extras

    def run(self):

        # Get the archive name
        self.ctx = utils.check_archive_name(self.ctx, logger=self.logger)
        
        # Check for the db_handle
        self.ctx = utils.check_dbh(self.ctx, logger=self.logger)
        
        # sort-cuts
        LOG = self.logger
        DBH = self.ctx.dbh

        # Put the inputs as kwargs
        input_kw = self.input.as_dict()

        # Update tileinfo RA values -- does nothing if we do not cross RA=0
        self.ctx.tileinfo = utils.update_tileinfo_RAZERO(self.ctx.tileinfo)
        
        # SQL Template version general method
        self.ctx.CCDS = querylibs.get_CCDS_from_db_general_sql(DBH, logger=LOG,**input_kw)
        if self.ctx.CCDS is False:
            self.logger.info("ERROR: No CCDS found to overlap tile:%s for the query" % self.ctx.tilename)
            self.logger.info("Exiting...Bye")
            exit(1)

        # Optional: Get the input catalogs if we want to run scamp for super-alignment
        if self.ctx.super_align:
            self.ctx.CATS = querylibs.get_CATS_from_db_general_sql(DBH, logger=LOG,**input_kw)
            # Make sure that we find the same number of exposure/unitnames
            UNITNAMES_CCDS = numpy.unique(self.ctx.CCDS['UNITNAME'])
            UNITNAMES_CATS = numpy.unique(self.ctx.CATS['UNITNAME'])

            if len(UNITNAMES_CCDS) != len(UNITNAMES_CATS):
                self.logger.info("WARNING: Number of UNITNAMES do not match between CCDS and CATS")

        # Get the finalcut large exposure-based scamp cats.
        if self.ctx.use_scampcats and self.ctx.super_align:
            self.ctx.SCAMPCATS = querylibs.get_SCAMPCATS_from_db_general_sql(DBH, logger=LOG,**input_kw)
                
        # Get root_https from from the DB with a query
        self.ctx.root_https   = querylibs.get_root_https(DBH,logger=LOG, archive_name=self.ctx.archive_name)
        self.ctx.root_archive = querylibs.get_root_archive(DBH,logger=LOG, archive_name=self.ctx.archive_name)
            
        # If in the cosmology archive local_archive=root and local_archive will be ignored
        if utils.inDESARcluster(logger=LOG) and self.ctx.local_archive == '':
            self.logger.info("In cosmology cluster -- setting local_archive=%s" % self.ctx.root_archive)
            self.ctx.local_archive = self.ctx.root_archive
        else:
            self.logger.info("Setting local_archive=%s" % self.ctx.local_archive)

        # Now we get the locations, ie the association information for images
        self.ctx.assoc = self.get_fitsfile_locations(self.ctx.CCDS,
                                                     self.ctx.local_archive,
                                                     self.ctx.root_https,
                                                     zp_force=self.ctx.zp_force,
                                                     logger=self.logger)

        # Get the SEG map list
        self.ctx.assoc_seg = self.get_fitsfile_locations(self.ctx.CCDS,
                                                         self.ctx.local_archive,
                                                         self.ctx.root_https,
                                                         zp_force=self.ctx.zp_force,
                                                         logger=self.logger, imagetype='seg')
        # Get the BKG list
        self.ctx.assoc_bkg = self.get_fitsfile_locations(self.ctx.CCDS,
                                                         self.ctx.local_archive,
                                                         self.ctx.root_https,
                                                         zp_force=self.ctx.zp_force,
                                                         logger=self.logger, imagetype='bkg')
        
        # Now we get the locations, ie the association information for catalogs
        if self.ctx.super_align:
            self.ctx.catlist = self.get_catalogs_locations(self.ctx.CATS,
                                                           self.ctx.local_archive,
                                                           self.ctx.root_https,
                                                           logger=self.logger)

        if self.ctx.super_align and self.ctx.use_scampcats:
            self.ctx.scampcatlist = self.get_catalogs_locations(self.ctx.SCAMPCATS,
                                                                self.ctx.local_archive,
                                                                self.ctx.root_https,
                                                                logger=self.logger,
                                                                filename_key='FILENAME_SCAMPCAT')

            self.ctx.scampheadlist = self.get_catalogs_locations(self.ctx.SCAMPCATS,
                                                                self.ctx.local_archive,
                                                                self.ctx.root_https,
                                                                logger=self.logger,
                                                                filename_key='FILENAME_SCAMPHEAD')
        # do we want to plot as well
        if self.input.plot_outname !="":
            from multiepoch.tasks.plot_ccd_corners_destile import Job as plot_job
            plot = plot_job(ctx=self.ctx)
            plot()

        # We might want to spit out the assoc file anyways
        if self.input.dump_assoc:
            assoc_default_name = fh.get_default_assoc_file(self.ctx.tiledir, self.ctx.tilename_fh)
            self.logger.info("Dumping assoc file to:%s" % assoc_default_name)
            self.write_dict2pandas(self.ctx.assoc,assoc_default_name,names=['FILEPATH_LOCAL','BAND','MAG_ZERO'],logger=self.logger)

        # and catlist
        if self.input.dump_cats and self.ctx.super_align:
            cats_default_name = fh.get_default_cats_file(self.ctx.tiledir, self.ctx.tilename_fh)
            self.logger.info("Dumping catlist file to:%s" % cats_default_name)
            self.write_dict2pandas(self.ctx.catlist,cats_default_name,names=['FILEPATH_LOCAL','BAND','UNITNAME'],logger=self.logger)

        # and scampcatlist
        if self.input.dump_cats and self.ctx.super_align and self.ctx.use_scampcats:
            cats_default_name = fh.get_default_scampcats_file(self.ctx.tiledir, self.ctx.tilename_fh)
            self.logger.info("Dumping catlist file to:%s" % cats_default_name)
            self.write_dict2pandas(self.ctx.scampcatlist,cats_default_name,names=['FILEPATH_LOCAL','BAND','UNITNAME'],logger=self.logger)

        # and scampheadlist
        if self.input.dump_cats and self.ctx.super_align and self.ctx.use_scampcats:
            cats_default_name = fh.get_default_scampheads_file(self.ctx.tiledir, self.ctx.tilename_fh)
            self.logger.info("Dumping catlist file to:%s" % cats_default_name)
            self.write_dict2pandas(self.ctx.scampheadlist,cats_default_name,names=['FILEPATH_LOCAL','BAND','UNITNAME'],logger=self.logger)

        # and meds inputs (BKG,SEG)
        if self.input.dump_assoc_meds:
            assoc_default_name = fh.get_default_assoc_file(self.ctx.tiledir, self.ctx.tilename_fh,imagetype='bkg')
            self.logger.info("Dumping assoc file to:%s" % assoc_default_name)
            self.write_dict2pandas(self.ctx.assoc_bkg,assoc_default_name,names=['FILEPATH_LOCAL','BAND'],logger=self.logger)
            assoc_default_name = fh.get_default_assoc_file(self.ctx.tiledir, self.ctx.tilename_fh,imagetype='seg')
            self.logger.info("Dumping assoc file to:%s" % assoc_default_name)
            self.write_dict2pandas(self.ctx.assoc_seg,assoc_default_name,names=['FILEPATH_LOCAL','BAND'],logger=self.logger)

    @staticmethod
    def get_fitsfile_locations(CCDS, local_archive, root_https, logger=None, zp_force=None,imagetype=None):
        """ Find the location of the files in the des archive and https urls
        """

        # TODO : to construct regular paths use os.path utils join, normpath ..

        # Number of images/filenames
        Nimages = len(CCDS['FILENAME'])

        # 1. Construct an new dictionary that will store the
        # information required to associate files for co-addition
        assoc = {}

        # Generic construction to get other image types
        if imagetype:
            FTYPE = "_%s" % imagetype.upper()
        else:
            FTYPE = ''

        assoc['BAND']        = CCDS['BAND']
        assoc['FILENAME']    = CCDS['FILENAME%s' % FTYPE]
        assoc['COMPRESSION'] = CCDS['COMPRESSION%s' % FTYPE]
        assoc['PATH']        = CCDS['PATH%s' % FTYPE]

        if 'MAG_ZERO'in CCDS.dtype.names:
            assoc['MAG_ZERO']    = CCDS['MAG_ZERO']
        else:
            assoc['MAG_ZERO']    = numpy.zeros(Nimages) + zp_force

        # In line loop creation
        filepaths = [os.path.join(assoc['PATH'][k],assoc['FILENAME'][k]+assoc['COMPRESSION'][k]) for k in range(Nimages)]

        # 2. Create the archive locations for each file
        assoc['FILEPATH_LOCAL'] = numpy.array([os.path.join(local_archive,filepath) for filepath in filepaths])
        
        # 3. Create the https locations for each file
        assoc['FILEPATH_HTTPS'] = numpy.array([os.path.join(root_https,filepath) for filepath in filepaths])

        return assoc


    @staticmethod
    def get_catalogs_locations(CATS, local_archive, root_https, logger=None,filename_key='FILENAME'):
        """ Find the location of the catalog files in the des archive and https urls
        """

        # Number of images/filenames
        Nimages = len(CATS[filename_key])

        # 1. Construct an new dictionary that will store the
        # information required to associate files for co-addition
        catlist = {}
        catlist['BAND']        = CATS['BAND']
        catlist['FILENAME']    = CATS[filename_key]
        catlist['UNITNAME']    = CATS['UNITNAME']

        # In line loop creation
        filepaths = [os.path.join(CATS['PATH'][k],CATS[filename_key][k]) for k in range(Nimages)]

        # 2. Create the archive locations for each file
        catlist['FILEPATH_LOCAL'] = numpy.array([os.path.join(local_archive,filepath) for filepath in filepaths])
        
        # 3. Create the https locations for each file
        catlist['FILEPATH_HTTPS'] = numpy.array([os.path.join(root_https,filepath) for filepath in filepaths])
        return catlist


    # -------------------------------------------------------------------------
    # WRITE FILES
    # -------------------------------------------------------------------------
    def write_assoc_pandas(self, assoc_file,names=['FILEPATH_LOCAL','BAND','MAG_ZERO'],sep=' '):
        self.logger.info("Writing CCDS files information to: %s" % assoc_file)
        variables = [self.ctx.assoc[name] for name in names]
        df = pd.DataFrame(zip(*variables), columns=names)
        df.to_csv(assoc_file,index=False,sep=sep)
        return

    @staticmethod
    def write_dict2pandas(mydict, file, names=['FILEPATH_LOCAL','BAND','UNITNAME'],sep=' ', logger=None):

        utils.pass_logger_info("Writing information to: %s" % file,logger=logger)
        variables = [mydict[name] for name in names]
        df = pd.DataFrame(zip(*variables), columns=names)
        df.to_csv(file,index=False,sep=sep)
        return

    def write_assoc_json(self, assoc_jsonfile,names=['FILEPATH_LOCAL','BAND','MAG_ZERO']):

        self.logger.info("Writing CCDS information to: %s" % assoc_jsonfile)
        dict_assoc = { name: self.ctx.assoc[name].tolist() for name in names }                         
        jsondict = {'assoc': dict_assoc}
	with open(assoc_jsonfile,"w") as jfi:
	    jfi.write(json.dumps(jsondict,sort_keys=False,indent=4))
        return

    def __str__(self):
        return 'find ccds in tile'


if __name__ == "__main__":
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)

    """
    if ran as script we directly write association info to files and also
    directly plot from here.
    """
    
    # For now we'll try to use plain asscii that can be read/write with pandas
    job.write_dict2pandas(job.ctx.assoc,job.ctx.assoc_file,names=['FILEPATH_LOCAL','BAND','MAG_ZERO'])
    if job.ctx.super_align:
        job.write_dict2pandas(job.ctx.catlist,job.ctx.cats_file,names=['FILEPATH_LOCAL','BAND','UNITNAME'])
        

