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
import multiepoch.querylibs as querylibs

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


class Job(BaseJob):

    '''
    DESDM multi-epoch pipeline : QUERY TILEINFO JOB
    ===============================================

    Required INPUT from context (ctx):
    ``````````````````````````````````
    - tile_edges: tuple of four floats (RACMIN, RACMAX, DECCMIN, DECCMAX)

    Writes as OUTPUT to context (ctx):
    ``````````````````````````````````
    - CCDS
    - assoc
    '''
    
    class Input(IO):

        """Find the CCDs that fall inside a tile"""
    
        # Required inputs to run the job (in ctx, after loading files)
        # because we set the argparse keyword to False they are not interfaced
        # to the command line parser
        tileinfo = Dict(None, help="The json file with the tile information",
                        argparse=False)
        tilename = Unicode(None, help="The Name of the Tile Name to query",
                           argparse=False)

        # Required inputs when run as script
        #
        # variables with keyword input_file=True are loaded into the ctx
        # automatically when intializing the Job class if provided, Input
        # validation happens only thereafter
        # you can implement a implement a custom reader for you input file in
        # your input class, let's say for a variable like
        #
        # my_input_file = CUnicode('',
        #        help='My input file.',
        #        argparse={'argtype': 'positional'})
        #
        # def _read_my_input_file(self):
        #     do the stuff
        #     return a dict

        tile_geom_input_file = CUnicode('',help='The json file with the tile information',
                # declare this variable as input_file, this leads the content
                # of the file to be loaded into the ctx at initialization
                input_file=True,
                # set argtype=positional !! to make this a required positional
                # argument when using the parser
                argparse={ 'argtype': 'positional', })

        # Optional inputs, also when interfaced to argparse
        db_section    = CUnicode("db-destest",
                                 help="DataBase Section to connect", 
                                 argparse={'choices': ('db-desoper','db-destest', )} )
        archive_name  = CUnicode("prodbeta",
                                 help="DataBase Archive Name section",
                                 argparse={'choices': ('prodbeta','desar2home')} )
        select_extras = CUnicode(SELECT_EXTRAS,
                                 help="string with extra SELECT for query",)
        and_extras    = CUnicode(AND_EXTRAS,
                                 help="string with extra AND for query",)
        from_extras   = CUnicode(FROM_EXTRAS,
                                 help="string with extra FROM for query",)
        tagname       = CUnicode('Y2T_FIRSTCUT',
                                 help="TAGNAME for images in the database",)
        exec_name     = CUnicode('immask',
                                 help="EXEC_NAME for images in the database",)
        assoc_file    = CUnicode("", 
                                 help="Name of the output ASCII association file where we will store the cccds information for coadd")
        assoc_json    = CUnicode("", 
                                 help="Name of the output JSON association file where we will store the cccds information for coadd")
        plot_outname   = CUnicode(None, help="Output file name for plot, in case we want to plot",)
        
        filepath_local = CUnicode(None,
                                  help="The local filepath where the input fits files (will) live")

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
        self.ctx = utils.check_dbh(self.ctx)

        # Create the tile_edges tuple structure and query the database
        tile_edges = self.get_tile_edges(self.ctx.tileinfo)
        self.ctx.CCDS = querylibs.get_CCDS_from_db(self.ctx.dbh, tile_edges, **self.input.as_dict())

        # Get the root paths
        self.ctx.root_archive = self.get_root_archive(self.ctx.dbh, archive_name=self.input.archive_name)
        self.ctx.root_https   = self.get_root_https(self.ctx.dbh, archive_name=self.input.archive_name)
        # In case we want root_http (DESDM framework)
        #self.ctx.root_https   = self.get_root_http(self.ctx.dbh, archive_name=self.input.archive_name)

        # Now we get the locations
        self.ctx.assoc = self.get_fitsfile_locations(filepath_local=self.input.filepath_local)

        # if Job is run as script
        if self.ctx.mojo_execution_mode == 'job as script':

            print "# Will write out the assoc file"
            # FELIPE: We need to decide whether want to write the assoc file
            # as json or space-separated ascii file.

            if self.ctx.filepath_local:
                names=['FILEPATH_LOCAL','FILENAME','BAND','MAG_ZERO']
            else:
                names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO']
            
            self.write_assoc_file(self.ctx.assoc_file,names=names)
            self.write_assoc_json(self.ctx.assoc_json,names=names)
            
            # do we plot as well?
            if self.ctx.plot_outname:
                from multiepoch.tasks.plot_ccd_corners_destile import Job as plot_job
                plot = plot_job(ctx=self.ctx)
                plot()


    @staticmethod
    def get_fitsfile_locations(ctx, filepath_local=None):

        """ Find the location of the files in the des archive and https urls"""

        # Number of images/filenames
        Nimages = len(ctx.CCDS['FILENAME'])

        # 1. Construct an new dictionary that will store the
        # information required to associate files for co-addition
        assoc = {}
        assoc['MAG_ZERO']    = ctx.CCDS['MAG_ZERO']
        assoc['BAND']        = ctx.CCDS['BAND']
        assoc['FILENAME']    = ctx.CCDS['FILENAME']
        assoc['COMPRESSION'] = ctx.CCDS['COMPRESSION']
        assoc['FILEPATH']    = npadd(ctx.CCDS['PATH'],"/")
        assoc['FILEPATH']    = npadd(assoc['FILEPATH'],ctx.CCDS['FILENAME'])
        assoc['FILEPATH']    = npadd(assoc['FILEPATH'],assoc['COMPRESSION'])

        # 2. Create the archive locations for each file
        path = [ctx.root_archive+"/"]*Nimages
        assoc['FILEPATH_ARCHIVE'] = npadd(path, assoc['FILEPATH'])

        # 3. Create the https locations for each file
        path = [ctx.root_https+"/"]*Nimages
        assoc['FILEPATH_HTTPS']   = npadd(path, assoc['FILEPATH'])

        # 4. in case we provide a filepath_local
        if filepath_local:
            print "# Setting FILEPATH_LOCAL to: %s" % filepath_local
            path = [filepath_local+"/"]*Nimages
            assoc['FILEPATH_LOCAL']   = npadd(path, assoc['FILEPATH'])
        
        return assoc

    @staticmethod
    def get_tile_edges(tileinfo):
        tile_edges = (tileinfo['RACMIN'], tileinfo['RACMAX'],
                tileinfo['DECCMIN'], tileinfo['DECCMAX'])
        return tile_edges

    @staticmethod
    def get_root_archive(dbh, archive_name='desar2home'):

        """
        Get the root-archive fron the database
        """
        cur = dbh.cursor()
        
        # root_archive
        query = "select root from ops_archive where name='%s'" % archive_name
        print "# Getting the archive root name for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur.execute(query)
        root_archive = cur.fetchone()[0]
        print "# root_archive: %s" % root_archive

        return root_archive

    @staticmethod
    def get_root_https(dbh, archive_name='desar2home'):

        """
        Get the root_https fron the database
        """
        cur = dbh.cursor()
        # root_https
        # to add it:
        # insert into ops_archive_val (name, key, val) values ('prodbeta', 'root_https', 'https://desar2.cosmology.illinois.edu/DESFiles/Prodbeta/archive');
        query = "select val from ops_archive_val where name='%s' and key='root_https'" % archive_name
        print "# Getting root_https for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur.execute(query)
        root_https = cur.fetchone()[0]
        print "# root_https:   %s" % root_https
        cur.close()
        return root_https

    @staticmethod
    def get_root_http(dbh, archive_name='desar2home'):

        """
        Get the root_http  fron the database
        """
        cur = dbh.cursor()
        # root_http 
        query = "select val from ops_archive_val where name='%s' and key='root_http'" % archive_name
        print "# Getting root_https for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur.execute(query)
        root_http  = cur.fetchone()[0]
        print "# root_http:   %s" % root_http
        cur.close()
        return root_http


    def write_assoc_file(self, assoc_file,names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO']):

        variables = []
        for name in names:
            variables.append(self.ctx.assoc[name].tolist())
            
        print "# Writing CCDS files information to: %s" % assoc_file
        N = len(names)
        header =  "# " + "%s "*N % tuple(names)
        format =  "%-s "*N 
        tableio.put_data(assoc_file,tuple(variables), header=header, format=format)
        return


    def write_assoc_json(self, assoc_jsonfile,names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO']):

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
