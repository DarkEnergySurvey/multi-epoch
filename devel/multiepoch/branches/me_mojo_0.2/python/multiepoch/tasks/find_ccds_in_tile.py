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
import despyastro
from mojo.jobs.base_job import BaseJob
from despydb import desdbi

# THE QUERY TEMPLATE THAT IS RUN TO GET THE CCDs
# -----------------------------------------------------------------------------
#

QUERY = """
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
         ops_proctag.TAG = '{tagname}'
      AND
         {and_extras} 
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
    - ccdinfo

    '''

    def get_query(self,**kwargs): 
        '''
        Get the database query that returns the ccds.

        kwargs
        ``````
            - exec_name
            - tagname
            - select_extras
            - and_extras
            - from_extras
        '''
        select_extras = kwargs.get('select_extras', SELECT_EXTRAS)
        and_extras    = kwargs.get('and_extras',    AND_EXTRAS)
        from_extras   = kwargs.get('from_extras',   FROM_EXTRAS)
        tagname       = kwargs.get('tagname',       'Y2T1_FIRSTCUT')
        exec_name     = kwargs.get('exec_name',     'immask')


        # Create the tile_edges tuple structure
        self.ctx.tile_edges = (
            self.ctx.tileinfo['RACMIN'], self.ctx.tileinfo['RACMAX'],
            self.ctx.tileinfo['DECCMIN'],self.ctx.tileinfo['DECCMAX']
            )

        corners_and = [
                "((image.RAC1 BETWEEN %.10f AND %.10f) AND (image.DECC1 BETWEEN %.10f AND %.10f))\n" % self.ctx.tile_edges,
                "((image.RAC2 BETWEEN %.10f AND %.10f) AND (image.DECC2 BETWEEN %.10f AND %.10f))\n" % self.ctx.tile_edges,
                "((image.RAC3 BETWEEN %.10f AND %.10f) AND (image.DECC3 BETWEEN %.10f AND %.10f))\n" % self.ctx.tile_edges,
                "((image.RAC4 BETWEEN %.10f AND %.10f) AND (image.DECC4 BETWEEN %.10f AND %.10f))\n" % self.ctx.tile_edges,
                ]

        query = QUERY.format(
                tagname       = tagname,
                exec_name     = exec_name,
                select_extras = select_extras,
                from_extras   = from_extras,
                and_extras    = and_extras + ' AND\n (' + ' OR '.join(corners_and) + ')',
                )

        print query
        return query


    def __call__(self):

        # Get the db handle
        if 'dbh' not in self.ctx:
            try:
                db_section = self.ctx.get('db_section','db-desoper')
                print "# Creating db-handle to section: %s" % db_section
                self.ctx.dbh = desdbi.DesDbi(section=db_section)
            except:
                raise ValueError('ERROR: Database handler could not be provided for context.')
        else:
            print "# Will recycle existing db-handle"

        # Call the query built function
        query = self.get_query(**self.ctx.get_kwargs_dict())
        
        print "# Getting CCD images within the tile definition"
        # Get the ccd images that are part of the DESTILE
        #self.ctx.CCDS = despyastro.genutil.query2dict_of_columns(query,dbhandle=self.ctx.dbh,array=True)
        self.ctx.CCDS = despyastro.genutil.query2rec(query,dbhandle=self.ctx.dbh)
        print "# Nelem %s" % len(self.ctx.CCDS['FILENAME'])

        # Get the filters we found
        self.ctx.BANDS  = numpy.unique(self.ctx.CCDS['BAND'])
        self.ctx.NBANDS = len(self.ctx.BANDS)

    def __str__(self):
        return 'find ccds in tile'

    def write_info(self,ccdsinfo_file,names=['FILENAME','PATH','BAND','MAG_ZERO']):

        print "# Writing CCDS information to: %s" % ccdsinfo_file

        # FIX
        # FIX -- this way of dumping is silly!!!!!
        # FIX

        #names = self.ctx.CCDS.dtype.names
        N = len(names)
        header =  "%12s "*N % tuple(names)

        o = open(ccdsinfo_file,"w")
        o.write("# %s\n" % header)
        for k in range(len(self.ctx.CCDS['FILENAME'])):
            for name in names:
                o.write("%s " % self.ctx.CCDS[name][k])
            o.write("\n")
        o.close()
        return

def read_tileinfo(geomfile):

    print "# Reading the tile Geometry from file: %s" % geomfile
    with open(geomfile, 'rb') as fp:
        tileinfo = json.load(fp)
    return tileinfo

def cmdline():

    """
    The funtion to generate and populate the commnand-line arguments into the context
    """

    import argparse
    parser = argparse.ArgumentParser(description="Find the CCDs that fall inside a tile")


    # The positional arguments
    parser.add_argument("tileinfo", help="The json file with the tile information")

    # Positional arguments
    parser.add_argument("--db_section", action="store", default="db-desoper",
                        help="DataBase Section to connect")
    parser.add_argument("--select_extras", action="store", default=SELECT_EXTRAS,
                        help="string with extra SELECT for query")
    parser.add_argument("--and_extras", action="store", default=AND_EXTRAS,
                        help="string with extra AND for query")
    parser.add_argument("--from_extras", action="store", default=FROM_EXTRAS,
                        help="string with extra FROM for query")
    parser.add_argument("--tagname", action="store", default='Y2T1_FIRSTCUT',
                        help="TAGNAME for images in the database")
    parser.add_argument("--exec_name", action="store", default='immask',
                        help="EXEC_NAME for images in the database")
    parser.add_argument("--ccdsinfo", action="store", default=None, # We might want to change the name of the "--option"
                        help="Name of the output file where we will store the cccds information")
    args = parser.parse_args()
    return args


if __name__ == "__main__":

    from mojo.utils.struct import Struct

    args = cmdline()

    # Initialize the class
    job = Job()
    job.ctx = Struct(dict(**args.__dict__))
    job.ctx.tileinfo = read_tileinfo(args.tileinfo)
    # Add cmdline options to the context using Struct
    job()  # Execute -- do call()

    # Write out the ccds information
    job.write_info(args.ccdsinfo)

    print 


