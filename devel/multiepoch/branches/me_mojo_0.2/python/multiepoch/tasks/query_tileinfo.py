#!/usr/bin/env python

"""
Builds a dictionary/header with all of the information on the COADDTILE_XXXX
table for a given TILENAME (input). It store the RAC[1,4] and DECC[1,4] corners
of the TILENAME an computes and stores the: (RACMIN, RACMAX) and
(DECCMIN,DECCMAX) for that tile.

Author: Felipe Menanteau, NCSA, Nov 2014.

"""

try:
    from mojo.jobs.base_job import BaseJob

except:
    print "WARNING: mojo not loaded"
    
from despydb import desdbi
import numpy
import json


QUERY = '''
    SELECT PIXELSCALE, NAXIS1, NAXIS2,
    RA, DEC,
    RAC1, RAC2, RAC3, RAC4,
    DECC1, DECC2, DECC3, DECC4,
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
    
    - tileinfo["RACS"]            plot_ccd_corners_destile.py -- RA corners of tile
    - tileinfo["DECCS"]           plot_ccd_corners_destile.py -- DEC corners of tile
    - tileinfo["DEC"]             plot_ccd_corners_destile.py -- DEC center of tile

    '''

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


    def __call__(self):

        # CHECK IF DATABASE HANDLER IS PRESENT
        if 'dbh' not in self.ctx:
            try:
                db_section = self.ctx.get('db_section','db-desoper')
                self.ctx.dbh = desdbi.DesDbi(section=db_section)
            except:
                raise ValueError('Database handler could not be provided for context.')
        else:
            print "# Will recycle existing db-handle"

        # EXECUTE THE QUERY
        cur = self.ctx.dbh.cursor()
        cur.execute(self.get_query(**self.ctx.get_kwargs_dict()))
        desc = [d[0] for d in cur.description]
        # cols description
        line = cur.fetchone()
        cur.close()

        # Make a dictionary/header for the all columns from COADDTILE table
        # FIXME ?? return of query to COADDTILE dict in ctx, rest into tileinfo
        self.ctx.tileinfo = dict(zip(desc, line))

        # Lower-case for compatibility with wcsutils
        #for k, v in self.ctx.tileinfo.items():
        #    self.ctx.tileinfo[k.lower()] = v

        # The minimum values for the tilename
        ras  = numpy.array([
            self.ctx.tileinfo['RAC1'], self.ctx.tileinfo['RAC2'],
            self.ctx.tileinfo['RAC3'], self.ctx.tileinfo['RAC4']
            ])
        decs = numpy.array([
            self.ctx.tileinfo['DECC1'], self.ctx.tileinfo['DECC2'],
            self.ctx.tileinfo['DECC3'], self.ctx.tileinfo['DECC4']
            ])

        ### TODO : add alls the following infered parameters to tileinfo?
        ### TODO @ Felipe : add RACMIN xxx into database schema for COADDTILE
        if self.ctx.tileinfo['CROSSRAZERO'] == 'Y':
            # Maybe we substract 360?
            self.ctx.tileinfo['RACMIN'] = ras.max()
            self.ctx.tileinfo['RACMAX'] = ras.min()
        else:
            self.ctx.tileinfo['RACMIN'] = ras.min()
            self.ctx.tileinfo['RACMAX'] = ras.max()
            
        self.ctx.tileinfo['DECCMIN'] = decs.min()
        self.ctx.tileinfo['DECCMAX'] = decs.max()

        ######################################################################
        # This re-packing will be done at plotting time now
        # Store the packed corners of the COADDTILES for plotting later --
        #self.ctx.tileinfo['RACS']  = ras  #numpy.append(ras,ras[0])
        #self.ctx.tileinfo['DECCS'] = decs #numpy.append(decs,decs[0])
        ####################################################################

    def write_tileinfo(self,geomfile=None):

        if not geomfile:
            geomfile = "%s.json" % self.ctx.tilename
            
        geo = open(geomfile,'w')
        geo.write(json.dumps(self.ctx.tileinfo,sort_keys=True,indent=4))
        geo.close()
        print "# Wrote the tile Geometry to file: %s" % geomfile
        # To read in ...
        #with open(geomfile, 'rb') as fp:
        #    data = json.load(fp)
        return


    def __str__(self):
        return 'query tileinfo'


def cmdline():

    """
    The funtion to generate and populate the commnand-line arguments into the context
    """

    import argparse
    parser = argparse.ArgumentParser(description="Collect the tile geometry information using the DESDM Database")

    # The positional arguments
    parser.add_argument("tilename", help="The Name of the Tile Name to query")

    # Positional arguments
    parser.add_argument("--db_section", action="store", default="db-desoper",
                        help="DataBase Section to connect")
    parser.add_argument("--coaddtile_table", action="store", default="felipe.coaddtile_new",
                        help="Database table with COADDTILE information")
    parser.add_argument("--json_file", action="store", default=None, # We might want to change the name of the "--option"
                        help="Name of the output json file where we will store the tile information")
    args = parser.parse_args()
    return args
    
if __name__ == "__main__":

    from mojo.utils.struct import Struct

    args = cmdline()

    # Initialize the class
    job = Job()
    # In case we want to execute only the query
    #job.get_query(**args.__dict__)
    # Add cmdline options to the context using Struct
    job.ctx = Struct(dict(**args.__dict__))
    job()  # Execute -- do call()
    job.write_tileinfo(args.json_file) # Write the json file with the tileinfo

    
