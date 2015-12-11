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
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider
import multiepoch.utils as utils
import multiepoch.querylibs as querylibs
import time
from despymisc.miscutils import elapsed_time
    

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

        # This is the description that argparse will register.
        """
        Collect the tile geometry information using the DESDM Database
        """
        
        # Required inputs
        tilename           = CUnicode(None, help="The Name of the Tile Name to query",
                                      argparse={'argtype': 'positional', } )
        # Optional inputs
        db_section         = CUnicode("db-destest", help="DataBase Section to connect",
                                      argparse={'choices': ('db-desoper','db-destest')} )
        json_tileinfo_file = CUnicode("",
                                      help="Name of the output json file where we will store the tile information",
                                      argparse={'required': True,})
        
        # we set required to True because we need this if executed as
        # script, even though argument will be declared with -- and
        # only then we use the parser
        coaddtile_table    = CUnicode("felipe.coaddtile_new", help="Database table with COADDTILE information",
                                      argparse=True)

        # Logging -- might be factored out
        stdoutloglevel     = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                      argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel       = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                      argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        def _validate_conditional(self):
            # if in job standalone mode json
            if self.mojo_execution_mode == 'job as script' and self.json_tileinfo_file == "":
                mess = 'If job is run standalone json_tileinfo_file cannot be ""'
                raise IO_ValidationError(mess)

    def run(self):
        

        # Check that we have a database handle
        self.ctx = utils.check_dbh(self.ctx, logger=self.logger)

        # Make a dictionary/header for the all columns from COADDTILE table
        t0 = time.time()
        self.ctx.tileinfo = querylibs.get_tileinfo_from_db(self.ctx.dbh,logger=self.logger,
                                                           **self.input.as_dict())
        self.logger.info("Query completed in %s" % elapsed_time(t0))

    def __str__(self):
        return 'query tileinfo' 



if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
    # if Job is run as script, we write the json file
    job.logger.info("Writing ouput to: %s" % job.input.json_tileinfo_file)
    job.write_ctx_to_json(job.input.json_tileinfo_file,vars_list=['tileinfo', 'tilename'])
                          
