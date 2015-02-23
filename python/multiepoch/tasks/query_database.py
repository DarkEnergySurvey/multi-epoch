#!/usr/bin/env python
"""
"""

import time
import json
import numpy

import despyastro
from despyastro import tableio
from despymisc.miscutils import elapsed_time

from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat,\
        CInt, Instance
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError

import multiepoch.utils as utils
from multiepoch.querylibs import get_tileinfo_from_db, get_CCDS_from_db
import multiepoch.contextDefs as contextDefs

npadd = numpy.core.defchararray.add


class Job(BaseJob):

    '''
    '''

    class Input(IO):

        # This is the description that argparse will register.
        """
        Collect the tile geometry information using the DESDM Database
        """
        
        # Required inputs
        tilename   = CUnicode(None, help="The Name of the Tile Name to query", 
                        argparse={'argtype': 'positional', } )

        # Optional inputs
        db_section = CUnicode("db-destest", help="DataBase Section to connect",
                        argparse={'choices': ('db-desoper','db-destest')} )

        # TODO : only one json output file !!
        json_tileinfo_file = CUnicode("", help=("Name of the output json file "
                                "where we will store the tile information"),
                                argparse={'required': True,})
        coaddtile_table    = CUnicode("felipe.coaddtile_new",
                                help="Database table with COADDTILE information",)
        archive_name       = CUnicode("desar2home",
                                help="DataBase Archive Name section",)
        select_extras      = CUnicode("felipe.extraZEROPOINT.MAG_ZERO,",
                                help="string with extra SELECT for query",)
        and_extras         = CUnicode("felipe.extraZEROPOINT.FILENAME=image.FILENAME",
                                help="string with extra AND for query",)
        from_extras        = CUnicode("felipe.extraZEROPOINT",
                                help="string with extra FROM for query",)
        tagname            = CUnicode('Y2T1_FIRSTCUT',
                                help="TAGNAME for images in the database",)
        exec_name          = CUnicode('immask',
                                help="EXEC_NAME for images in the database",)
        assoc_json         = CUnicode("",
                                help=("Name of the output JSON association "
                                      "file where we will store the cccds "
                                      "information for coadd"),
                                argparse={'required': True})
        plot_outname       = CUnicode("",
                                help=("Output file name for plot, in case we "
                                      "want to plot"))
        filepath_local     = CUnicode("",
                                help=("The local filepath where the input "
                                      "fits files (will) live"))

        def _validate_conditional(self):
            pass



    # THE QUERIES
    # -------------------------------------------------------------------------
    """ 

    # The query template used to get the geometry of the tile
    QUERY_GEOM = '''
                SELECT PIXELSCALE, NAXIS1, NAXIS2,
                RA, DEC,
                RAC1, RAC2, RAC3, RAC4,
                DECC1, DECC2, DECC3, DECC4,
                RACMIN,RACMAX,DECCMIN,DECCMAX,
                CROSSRAZERO
                FROM {coaddtile_table}
                WHERE tilename='{tilename}'
                '''

    # The query template to get the the CCDs
    QUERY_CCDS = ''' 
         SELECT
             {select_extras}
             file_archive_info.FILENAME,file_archive_info.PATH, image.BAND,
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
    """
        

    # RUN 
    # -------------------------------------------------------------------------

    def run(self):

        # Check that we have a database handle
        # FIXME : could make it a input argument, with default value
        self.ctx = utils.check_dbh(self.ctx)

        # GEOMETRY INFORMATION ------------------------------------------------

        t0 = time.time()
        # get_tileinfo_from_db gets all the the arguments needed directly out
        # of self.input and the dbh out of the ctx
        self.ctx.tileinfo = get_tileinfo_from_db(self.ctx.dbh,
                **self.input.as_dict())
        print "# Done in %s" % elapsed_time(t0)
        t1 = time.time()

        # CCD INFORMATION -----------------------------------------------------

        print "# Getting CCD images within the tile definition"
        tile_edges = (self.ctx.tileinfo['RACMIN'], self.ctx.tileinfo['RACMAX'],
                       self.ctx.tileinfo['DECCMIN'],self.ctx.tileinfo['DECCMAX'])
        self.ctx.CCDS = get_CCDS_from_db(self.ctx.dbh, tile_edges,
                **self.input.as_dict())
        print "# Query time: %s" % elapsed_time(t1)
        print "# Nelem %s" % len(self.ctx.CCDS['FILENAME'])

        #  FILE INFORMATION ---------------------------------------------------

        # Get the root paths
        self.ctx.root_archive = self.get_root_archive(
                                    archive_name=self.input.archive_name)
        self.ctx.root_https   = self.get_root_https(
                                    archive_name=self.input.archive_name)

        # Now we get the locations
        self.ctx.assoc = self.get_fitsfile_locations(
                                    filepath_local=self.ctx.filepath_local)


    # UTILITIES 
    # -------------------------------------------------------------------------

    def get_fitsfile_locations(self,filepath_local=None):
        """ Find the location of the files in the des archive and https urls"""

        # Number of images/filenames
        Nimages = len(self.ctx.CCDS['FILENAME'])

        # 1. Construct an new dictionary that will store the
        # information required to associate files for co-addition
        assoc = {}
        assoc['MAG_ZERO'] = self.ctx.CCDS['MAG_ZERO']
        assoc['BAND']     = self.ctx.CCDS['BAND']
        assoc['FILENAME'] = self.ctx.CCDS['FILENAME']

        assoc['FILEPATH'] = npadd(self.ctx.CCDS['PATH'],"/")
        assoc['FILEPATH'] = npadd(assoc['FILEPATH'],self.ctx.CCDS['FILENAME'])

        # 2. Create the archive locations for each file
        path = [self.ctx.root_archive+"/"]*Nimages
        assoc['FILEPATH_ARCHIVE'] = npadd(path, assoc['FILEPATH'])

        # 3. Create the https locations for each file
        path = [self.ctx.root_https+"/"]*Nimages
        assoc['FILEPATH_HTTPS']   = npadd(path, assoc['FILEPATH'])

        # 4. in case we provide a filepath_local
        if filepath_local:
            print "# Setting FILEPATH_LOCAL to: %s" % filepath_local
            path = [filepath_local+"/"]*Nimages
            assoc['FILEPATH_LOCAL']   = npadd(path, assoc['FILEPATH'])

        return assoc

    def get_root_archive(self, archive_name='desar2home'):
        """ Get the root-archive fron the database """
        cur = self.ctx.dbh.cursor()
        # root_archive
        query = "SELECT root FROM ops_archive WHERE name='%s'" % archive_name
        print "# Getting the archive root name for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur.execute(query)
        root_archive = cur.fetchone()[0]
        print "# root_archive: %s" % root_archive
        return root_archive

    def get_root_https(self, archive_name='desar2home'):
        """ Get the root_https fron the database """
        cur = self.ctx.dbh.cursor()
        # root_https
        query = "SELECT val FROM ops_archive_val WHERE name='%s' and key='root_https'" % archive_name
        print "# Getting root_https for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur.execute(query)
        root_https = cur.fetchone()[0]
        print "# root_https:   %s" % root_https
        cur.close()
        return root_https

    def write_assoc_json(self,assoc_jsonfile,names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO']):
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
        return 'query database' 


if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)

    # write the tileinfo file
    job.write_ctx_to_json(self.input.json_tileinfo_file,
            vars_list=['tileinfo', 'tilename'])

    if job.ctx.filepath_local:
        names=['FILEPATH_LOCAL','FILENAME','BAND','MAG_ZERO']
    else:
        names=['FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO']
    self.write_assoc_json(self.ctx.assoc_json,names=names)

    # do plotting implemented in another job module in this module
    # FIXME : what about the generated files? wcl file transfer??
    if job.ctx.plot_outname:
        from multiepoch.tasks.plot_ccd_corners_destile import Job as plot_job
        plot = plot_job(ctx=self.ctx)
        plot()
