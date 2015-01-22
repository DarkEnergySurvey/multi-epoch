#!/usr/bin/env python

"""
Builds a dictionary/header with all of the information on the COADDTILE_XXXX
table for a given TILENAME (input). It store the RAC[1,4] and DECC[1,4] corners
of the TILENAME an computes and stores the: (RACMIN, RACMAX) and
(DECCMIN,DECCMAX) for that tile.

Author: Felipe Menanteau, NCSA, Nov 2014.

"""
import json
import numpy

from despydb import desdbi

from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance

from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider
    

QUERY = '''
    SELECT PIXELSCALE, NAXIS1, NAXIS2,
    RA, DEC,
    RAC1, RAC2, RAC3, RAC4,
    DECC1, DECC2, DECC3, DECC4,
    RACMIN,RACMAX,DECCMIN,DECCMAX,
    CROSSRAZERO
    FROM {tablename}
    WHERE tilename='{tilename}'
    '''

class Job(BaseJob):

    '''
    DESDM multi-epoch pipeline : QUERY TILEINFO JOB
    ===============================================
    This job queries the DESDM desoper database for information about a given
    tile and write this information into ctx.tileinfo.

    Required INPUT from context (ctx):
    ``````````````````````````````````
    - tilename :        string, default_value=None
    - coaddtile_table : string, default_value="felipe.coaddtile_new"
    - db_section:       string, default_balue="db-desoper"

    Writes as OUTPUT to context (ctx):
    ``````````````````````````````````
    - tileinfo : dictionary

    What we **REALLY** use from tileinfo in the process:

    Object                         function/task used
    -------                        ----------------------
    - tiles_edges =                find_ccds_in_tile.py
       [RAMIN,RAMAX,DECMIN,DECMAX]   

    - tileinfo["PIXELSCALE"]      "PIXEL_SCALE",call_SWarp.py 
    - tileinfo["NAXIS1"]          "IMAGE_SIZE", call_SWarp.py\ IMAGE_SIZE=(NAXIS1,NAXIS2)
    - tileinfo["NAXIS2"]          "IMAGE_SIZE", call_SWarp.py/ 
    - tileinfo["RA"]              "CENTER", call_SWarp.py \ CENTER=(RA,DEC)
    - tileinfo["DEC"]             "CENTER", call_SWarp.py /
    
    - tileinfo["RACS"]            plot_ccd_corners_destile.py -- RA  corners of tile
    - tileinfo["DECCS"]           plot_ccd_corners_destile.py -- DEC corners of tile
    - tileinfo["DEC"]             plot_ccd_corners_destile.py -- DEC center of tile

    '''

    class Input(IO):
        # Required inputs
        tilename   = CUnicode(None, help="The Name of the Tile Name to query")
        # Optional inputs
        db_section = CUnicode("db-destest", help="DataBase Section to connect")
        json_job_output_file = CUnicode("", help= ("Name of the output json "
            "file where we will store the tile information"))
        coaddtile_table = CUnicode("felipe.coaddtile_new", help=("Database "
            "table with COADDTILE information"))

        def _validate_conditional(self):
            # if in job standalone mode json
            if (self.execution_mode == 'mojo run_job' or
                    self.execution_mode == 'job as script') and (
                            self.json_job_output_file == ""):
                mess = 'If job is run standalone json_job_output_file cannot be ""'
                raise IO_ValidationError(mess)


    def run(self):

        # CHECK IF DATABASE HANDLER IS PRESENT
        if 'dbh' not in self.ctx:
            try:
                self.ctx.dbh = desdbi.DesDbi(section=self.input.db_section)
            except:
                raise ValueError('Database handler could not be provided for context.')
        else:
            print "# Will recycle existing db-handle"

        # EXECUTE THE QUERY
        cur = self.ctx.dbh.cursor()
        cur.execute(self.get_query(**self.input.as_dict()))
        desc = [d[0] for d in cur.description]
        # cols description
        line = cur.fetchone()
        cur.close()

        # Make a dictionary/header for the all columns from COADDTILE table
        self.ctx.tileinfo = dict(zip(desc, line))

        # dump data into json if json_job_output_file is given
        if self.input.json_job_output_file != "":
            ContextProvider().json_dump_ctx(self.ctx,
                    var_list=('tilename', 'tileinfo',),
                    filename=self.input.json_job_output_file)


    def get_query(self, **kwargs):

        '''
        Get the database query that returns DES tile information.

        kwargs
        ``````
            - tablename
            - tilename
        '''

        tablename = kwargs.get('coaddtile_table', None)
        tilename  = kwargs.get('tilename', None)

        if not tablename or not tilename:
            raise ValueError('ERROR: tablename and tilename need to be provided as kwargs')

        query_string = QUERY.format(tablename=tablename, tilename=tilename)
        return query_string


    def __str__(self):
        return 'query tileinfo' 


if __name__ == '__main__':
    from mojo.utils import main_runner
    main_runner.run_as_main(Job)
