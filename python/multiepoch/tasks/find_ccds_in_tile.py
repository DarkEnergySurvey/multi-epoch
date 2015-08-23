#!/usr/bin/env python

"""
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

Author: Felipe Menanteau, NCSA, Nov 2014.

"""

import json
import numpy
import time
import os
import pandas as pd
import despyastro

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider
from mojo.utils import log

# Mutiepoch loads
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
import multiepoch.querylibs as querylibs

# DEFAULT PARAMETER VALUES -- Change from felipe.XXXXX --> XXXXX
# -----------------------------------------------------------------------------
SELECT_EXTRAS = "felipe.extraZEROPOINT.MAG_ZERO,"
FROM_EXTRAS   = "felipe.extraZEROPOINT"
AND_EXTRAS    = "felipe.extraZEROPOINT.FILENAME = image.FILENAME" 
# -----------------------------------------------------------------------------

#import mojo
#import mojo.utils 
#mojo.utils.log.DEFAULT_CONSOLELOGLEVEL = 'INFO'
#mojo.utils.log.DEFAULT_FILELOGLEVEL    = 'INFO'
#print mojo.utils.log.DEFAULT_CONSOLELOGLEVEL 

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

        # Required inputs when run as script
        #
        # variables with keyword input_file=True are loaded into the ctx
        # automatically when intializing the Job class if provided, Input
        # validation happens only thereafter
        # you can implement a implement a custom reader for you input file in
        # your input class, let's say for a variable like
        #
        # def _read_my_input_file(self):
        #     do the stuff
        #     return a dict

        tile_geom_input_file = CUnicode('',help='The json file with the tile information',
                # declare this variable as input_file, this leads the content of the file to be loaded into the ctx at initialization
                input_file=True,
                # set argtype=positional !! to make this a required positional argument when using the parser
                argparse={ 'argtype': 'positional', })

        # Optional inputs, also when interfaced to argparse
        db_section    = CUnicode("db-destest",help="DataBase Section to connect", 
                                 argparse={'choices': ('db-desoper','db-destest', )} )
        archive_name  = CUnicode("prodbeta",help="DataBase Archive Name section",
                                 argparse={'choices': ('prodbeta','desar2home')} )
        select_extras = CUnicode(SELECT_EXTRAS,help="string with extra SELECT for query",)
        and_extras    = CUnicode(AND_EXTRAS,help="string with extra AND for query",)
        from_extras   = CUnicode(FROM_EXTRAS,help="string with extra FROM for query",)
        tagname       = CUnicode('Y2T_FIRSTCUT',help="TAGNAME for images in the database",)
        exec_name     = CUnicode('immask', help=("EXEC_NAME for images in the database"))
        assoc_file    = CUnicode("", help=("Name of the output ASCII association file where we will store the cccds information for coadd"))
        assoc_json    = CUnicode("", help=("Name of the output JSON association file where we will store the cccds information for coadd"))
        plot_outname  = CUnicode("", help=("Output file name for plot, in case we want to plot"))
        local_archive = CUnicode("", help=("The local filepath where the input fits files (will) live"))

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        def _validate_conditional(self):

            #  We will need one or ther other, but not both.
            # Check for valid output assoc_json
            #if self.mojo_execution_mode == 'job as script' and self.assoc_json == "":
            #    mess = 'If job is run standalone assoc_json cannot be ""'
            #    raise IO_ValidationError(mess)

            # Check for valid output assoc_file
            if self.mojo_execution_mode == 'job as script' and self.assoc_file == "":
                mess = 'If job is run standalone assoc_file cannot be ""'
                raise IO_ValidationError(mess)

            # Check for valid local_archive if not in the NCSA cosmology cluster
            if not utils.inDESARcluster(logger=log.get_logger({})) and not self.local_archive: 
                mess = 'If not in cosmology cluster local_archive cannot be empty [""]'
                raise IO_ValidationError(mess)

            #########################################
            # REMOVE LATER
            # FOR TESTING ON SHORTER DATASETS
            self.from_extras = FROM_EXTRAS+", felipe.TAGS"
            self.and_extras  = AND_EXTRAS +" and\nfelipe.TAGS.FILENAME = image.FILENAME and felipe.TAGS.TAG = '%s_RAN_EXP'" % self.tilename
            #########################################

    def run(self):
        
        # Check for the db_handle
        self.ctx = utils.check_dbh(self.ctx, logger=self.logger)
        
        # sort-cuts
        LOG = self.logger
        DBH = self.ctx.dbh

        # Create the tile_edges tuple structure and query the database
        tile_edges = self.get_tile_edges(self.ctx.tileinfo)
        self.ctx.CCDS = querylibs.get_CCDS_from_db(DBH, tile_edges,logger=LOG,**self.input.as_dict())

        # Get root_https from from the DB with a query
        self.ctx.root_https   = querylibs.get_root_https(DBH,logger=LOG, archive_name=self.input.archive_name)
        self.ctx.root_archive = querylibs.get_root_archive(DBH,logger=LOG, archive_name=self.input.archive_name)
        # In case we want root_http (DESDM framework)
        #self.ctx.root_https  = querylibs.get_root_http(self.ctx.dbh, archive_name=self.input.archive_name)
            
        # If in the cosmology archive local_archive=root path and local_archive not defined
        if utils.inDESARcluster(logger=LOG) and self.ctx.local_archive == '':
            self.logger.info("In cosmology cluster -- setting local_archive=%s" % self.ctx.root_archive)
            self.ctx.local_archive = self.ctx.root_archive
        else:
            self.logger.info("Not in cosmology cluster -- setting local_archive=%s" % self.ctx.local_archive)


        # Now we get the locations, ie the association information
        self.ctx.assoc = self.get_fitsfile_locations(self.ctx.CCDS,
                                                     self.ctx.local_archive,
                                                     self.ctx.root_https,
                                                     logger=self.logger)

        if self.input.assoc_json != "":
            self.write_assoc_json(self.input.assoc_json)


    # -------------------------------------------------------------------------

    @staticmethod
    def get_tile_edges(tileinfo):
        tile_edges = (tileinfo['RACMIN'], tileinfo['RACMAX'],
                tileinfo['DECCMIN'], tileinfo['DECCMAX'])
        return tile_edges


    @staticmethod
    def get_fitsfile_locations(CCDS, local_archive, root_https, logger=None):
        """ Find the location of the files in the des archive and https urls
        """

        # TODO : to construct regular paths use os.path utils join, normpath ..

        # Number of images/filenames
        Nimages = len(CCDS['FILENAME'])

        # 1. Construct an new dictionary that will store the
        # information required to associate files for co-addition
        assoc = {}
        assoc['MAG_ZERO']    = CCDS['MAG_ZERO']
        assoc['BAND']        = CCDS['BAND']
        assoc['FILENAME']    = CCDS['FILENAME']
        assoc['COMPRESSION'] = CCDS['COMPRESSION']

        # Filename with fz if exists
        #assoc['FILENAME'] =  [CCDS['FILENAME'][k]+CCDS['COMPRESSION'][k] for k in range(Nimages)]

        # In line loop creation
        filepaths = [os.path.join(CCDS['PATH'][k],CCDS['FILENAME'][k]+CCDS['COMPRESSION'][k]) for k in range(Nimages)]

        # 2. Create the archive locations for each file
        assoc['FILEPATH_LOCAL'] = numpy.array([os.path.join(local_archive,filepath) for filepath in filepaths])
        
        # 3. Create the https locations for each file
        assoc['FILEPATH_HTTPS'] = numpy.array([os.path.join(root_https,filepath) for filepath in filepaths])

        return assoc


    # -------------------------------------------------------------------------
    # WRITE FILES
    # -------------------------------------------------------------------------
    def write_assoc_pandas(self, assoc_file,names=['FILEPATH_LOCAL','BAND','MAG_ZERO'],sep=' '):

        self.logger.info("Writing CCDS files information to: %s" % assoc_file)
        variables = [self.ctx.assoc[name] for name in names]
        df = pd.DataFrame(zip(*variables), columns=names)
        df.to_csv(assoc_file,index=False,sep=sep)
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
    
    names=['FILEPATH_LOCAL','BAND','MAG_ZERO']
    # For now we'll try to use plain asscii that can be read/write with pandas
    job.write_assoc_pandas(job.ctx.assoc_file,names=names)
    #job.write_assoc_json(job.ctx.assoc_json,names=names)


