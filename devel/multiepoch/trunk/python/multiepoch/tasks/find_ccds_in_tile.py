#!/usr/bin/env python

import json
import numpy
import time
import despyastro

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider
# Mutiepoch loads
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs

from despyastro import tableio
from despymisc.miscutils import elapsed_time
npadd = numpy.core.defchararray.add

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

# DEFAULT PARAMETER VALUES -- Change from felipe.XXXXX --> XXXXX
# -----------------------------------------------------------------------------
SELECT_EXTRAS = "felipe.extraZEROPOINT.MAG_ZERO,"
FROM_EXTRAS   = "felipe.extraZEROPOINT"
AND_EXTRAS    = "felipe.extraZEROPOINT.FILENAME = image.FILENAME" 
# -----------------------------------------------------------------------------

# The QUERY template used to get the the CCDs
QUERY_CCDS = ''' 
     SELECT
         {select_extras}
         file_archive_info.FILENAME,file_archive_info.COMPRESSION,
         file_archive_info.PATH,
         image.BAND,
         image.RAC1,  image.RAC2,  image.RAC3,  image.RAC4,
         image.DECC1, image.DECC2, image.DECC3, image.DECC4
     FROM
         file_archive_info, wgb, image, ops_proctag,
         {from_extras} 
     WHERE
         file_archive_info.FILENAME  = image.FILENAME AND
         file_archive_info.FILENAME  = wgb.FILENAME  AND
         image.FILETYPE  = 'red' AND
         wgb.FILETYPE    = 'red' AND
         wgb.EXEC_NAME   = '{exec_name}' AND
         wgb.REQNUM      = ops_proctag.REQNUM AND
         wgb.UNITNAME    = ops_proctag.UNITNAME AND
         wgb.ATTNUM      = ops_proctag.ATTNUM AND
         ops_proctag.TAG = '{tagname}' AND
         {and_extras} 
     '''


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
        tileinfo = Dict(None, help="The tileinfo dictionary.",
                        argparse=False)
        tilename = Unicode(None, help="The name of the tile.",
                           argparse=False)

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

        tile_geom_input_file = CUnicode('',
                help='The json file with the tile information',
                # declare this variable as input_file, this leads the content
                # of the file to be loaded into the ctx at initialization
                input_file=True,
                # set argtype=positional !! to make this a required positional
                # argument when using the parser
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
        assoc_file    = CUnicode("", help= ("Name of the output ASCII association file where we will store the cccds information for coadd"))
        assoc_json    = CUnicode("", help=("Name of the output JSON association file where we will store the cccds information for coadd"))
        plot_outname  = CUnicode('', help=("Output file name for plot, in case we want to plot"))
        local_archive = CUnicode('', help=("The local filepath where the input fits files (will) live"))

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        def _validate_conditional(self):

            #  We will need one or ther other, but not both.
            # Check for valid output assoc_json
            if self.mojo_execution_mode == 'job as script' and self.assoc_json == "":
                mess = 'If job is run standalone assoc_json cannot be ""'
                raise IO_ValidationError(mess)

            # Check for valid output assoc_file
            if self.mojo_execution_mode == 'job as script' and self.assoc_file == "":
                mess = 'If job is run standalone assoc_file cannot be ""'
                raise IO_ValidationError(mess)


    def run(self):

        # Check for the db_handle
        self.ctx = utils.check_dbh(self.ctx, logger=self.logger)

        # Create the tile_edges tuple structure and query the database
        tile_edges = self.get_tile_edges(self.ctx.tileinfo)

        self.ctx.CCDS = Job.get_CCDS_from_db(self.ctx.dbh,
                                             tile_edges,
                                             logger=self.logger,
                                             **self.input.as_dict())

        # Get the cosmology archive root path in case there is no local_archive
        # path defined
        if self.ctx.local_archive == '':
            self.ctx.local_archive = self.get_root_archive(self.ctx.dbh,
                    logger=self.logger, archive_name=self.input.archive_name)

        self.ctx.root_https   = self.get_root_https(self.ctx.dbh,
                logger=self.logger, archive_name=self.input.archive_name)
        # In case we want root_http (DESDM framework)
        #self.ctx.root_https   = self.get_root_http(self.ctx.dbh, archive_name=self.input.archive_name)

        # Now we get the locations, ie the association information
        self.ctx.assoc = self.get_fitsfile_locations(
                self.ctx.CCDS, self.ctx.local_archive, self.ctx.root_https,
                logger=self.logger)

        # TODO : keep CCDS as list of dicts -> like that we could keep CCDS in
        # a dumped ctx
        #self.ctx.CCDS = [dict(zip(CCDS.dtype.names, ccd)) for ccd  in CCDS]

#       if self.ctx.filepath_local:
#           names=['FILEPATH_LOCAL','FILENAME','BAND','MAG_ZERO']
#       else:
#           names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO',]
#       
#       self.ctx.assoc = { name: assoc[name].tolist() for name in names }


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

        # TODO : CLEANUP : is FILEPATH needed some other place or could it be
        # removed?
        assoc['FILEPATH']    = npadd(CCDS['PATH'], "/")
        assoc['FILEPATH']    = npadd(assoc['FILEPATH'], CCDS['FILENAME'])
        assoc['FILEPATH']    = npadd(assoc['FILEPATH'], assoc['COMPRESSION'])

        # 2. Create the archive locations for each file
        path = [local_archive+"/"]*Nimages
        assoc['FILEPATH_ARCHIVE'] = npadd(path, assoc['FILEPATH'])

        # 3. Create the https locations for each file
        path = [root_https+"/"]*Nimages
        assoc['FILEPATH_HTTPS']   = npadd(path, assoc['FILEPATH'])

# TODO : CLEANUP : remove if indeed not needed.
#       # 4. in case we provide a filepath_local
#       if filepath_local:
#           if logger:
#               logger.info("Setting FILEPATH_LOCAL to: %s" % filepath_local)
#           path = [filepath_local+"/"]*Nimages
#           assoc['FILEPATH_LOCAL']   = npadd(path, assoc['FILEPATH'])
        
        return assoc


    # QUERY methods
    # -------------------------------------------------------------------------

    @staticmethod
    def get_root_archive(dbh, archive_name='desar2home', logger=None):
        """ Get the root-archive fron the database
        """
        cur = dbh.cursor()
        # root_archive
        query = "SELECT root FROM ops_archive WHERE name='%s'" % archive_name
        if logger:
            logger.debug("Getting the archive root name for section: %s" % archive_name)
            logger.debug("Will execute the SQL query:\n********\n** %s\n********" % query)
        cur.execute(query)
        root_archive = cur.fetchone()[0]
        if logger: logger.info("root_archive: %s" % root_archive)
        return root_archive


    @staticmethod
    def get_root_https(dbh, archive_name='desar2home', logger=None):
        """ Get the root_https fron the database
        """
        cur = dbh.cursor()
        # root_https
        # to add it:
        # insert into ops_archive_val (name, key, val) values ('prodbeta', 'root_https', 'https://desar2.cosmology.illinois.edu/DESFiles/Prodbeta/archive');
        query = "SELECT val FROM ops_archive_val WHERE name='%s' AND key='root_https'" % archive_name
        if logger:
            logger.debug("Getting root_https for section: %s" % archive_name)
            logger.debug("Will execute the SQL query:\n********\n** %s\n********" % query)
        cur.execute(query)
        root_https = cur.fetchone()[0]
        if logger: logger.info("root_https:   %s" % root_https)
        cur.close()
        return root_https


    @staticmethod
    def get_root_http(dbh, archive_name='desar2home', logger=None):
        """ Get the root_http  fron the database
        """
        cur = dbh.cursor()
        # root_http 
        query = "SELECT val FROM ops_archive_val WHERE name='%s' AND key='root_http'" % archive_name
        if logger:
            logger.debug("Getting root_https for section: %s" % archive_name)
            logger.debug("Will execute the SQL query:\n********\n** %s\n********" % query)
        cur.execute(query)
        root_http  = cur.fetchone()[0]
        if logger: logger.info("root_http:   %s" % root_http)
        cur.close()
        return root_http


    @staticmethod
    def get_CCDS_from_db(dbh, tile_edges, **kwargs): 
        '''
        Execute the database query that returns the ccds and store them in a numpy
        record array
        kwargs: exec_name, tagname, select_extras, and_extras, from_extras
        '''
        logger = kwargs.pop('logger', None)

        mess = "Running the query to find the CCDS"
        if logger: logger.info(mess)
        else: print mess

        corners_and = [
            "((image.RAC1 BETWEEN %.10f AND %.10f) AND (image.DECC1 BETWEEN %.10f AND %.10f))\n" % tile_edges,
            "((image.RAC2 BETWEEN %.10f AND %.10f) AND (image.DECC2 BETWEEN %.10f AND %.10f))\n" % tile_edges,
            "((image.RAC3 BETWEEN %.10f AND %.10f) AND (image.DECC3 BETWEEN %.10f AND %.10f))\n" % tile_edges,
            "((image.RAC4 BETWEEN %.10f AND %.10f) AND (image.DECC4 BETWEEN %.10f AND %.10f))\n" % tile_edges,
            ]
        ccd_query = QUERY_CCDS.format(
            tagname       = kwargs.get('tagname'),
            exec_name     = kwargs.get('exec_name',     'immask'),
            select_extras = kwargs.get('select_extras'),
            from_extras   = kwargs.get('from_extras'),
            and_extras    = kwargs.get('and_extras')+  ' AND\n (' + ' OR '.join(corners_and) + ')',
            )

        mess = "Will execute the query:\n%s\n" %  ccd_query
        if logger: logger.debug(mess)
        else: print mess
        
        # Get the ccd images that are part of the DESTILE
        CCDS = despyastro.genutil.query2rec(ccd_query, dbhandle=dbh)

        # Fix 'COMPRESSION' from None --> '' if present
        if 'COMPRESSION' in CCDS.dtype.names:
            CCDS['COMPRESSION'] = numpy.where(CCDS['COMPRESSION'],CCDS['COMPRESSION'],'')
        return CCDS 


    # WRITE FILES
    # -------------------------------------------------------------------------

    def write_assoc_file(self, assoc_file,
            names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO']):

        variables = []
        for name in names:
            variables.append(self.ctx.assoc[name].tolist())
            
        print "# Writing CCDS files information to: %s" % assoc_file
        N = len(names)
        header =  "# " + "%s "*N % tuple(names)
        format =  "%-s "*N 
        tableio.put_data(assoc_file,tuple(variables), header=header, format=format)
        return


    def write_assoc_json(self, assoc_jsonfile,
            names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO']):

        print "# Writing CCDS information to: %s" % assoc_jsonfile
        dict_assoc = {}
        for name in names:
            # Make them lists instead of rec arrays
            dict_assoc[name] =  self.ctx.assoc[name].tolist()
        o = open(assoc_jsonfile,"w")
        # Put in the proper container
        jsondict = {'assoc': dict_assoc}
        o.write(json.dumps(jsondict,sort_keys=False,indent=4))
        o.close()
        return


    def __str__(self):
        return 'find ccds in tile'


if __name__ == "__main__":
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)

    '''
    if ran as script we directly write association info to files and also
    directly plot from here.

    FIXME MICHAEL : NEEDS REFACTORING

    print "# Will write out the assoc file"
    # FELIPE: We need to decide whether want to write the assoc file
    # as json or space-separated ascii file.

    names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO']
    
    self.write_assoc_file(self.ctx.assoc_file,names=names)
    self.write_assoc_json(self.ctx.assoc_json,names=names)
    
    # do we plot as well?
    if self.ctx.plot_outname:
        from multiepoch.tasks.plot_ccd_corners_destile import Job as plot_job
        plot = plot_job(ctx=self.ctx)
        plot()
    '''
