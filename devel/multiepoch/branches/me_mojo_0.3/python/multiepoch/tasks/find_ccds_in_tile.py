#!/usr/bin/env python

import json
import numpy
import time
import despyastro
from mojo.jobs.base_job import BaseJob
from despydb import desdbi
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

    def run(self):


        # Check for the db_handle
        self.check_dbh()

        # Call the query built function
        print "# Getting CCD images within the tile definition"
        self.get_CCDS(**self.ctx.get_kwargs_dict())

        # Now we get the locations
        self.get_fitsfile_locations()
        
        # Get the filters we found
        self.ctx.BANDS  = numpy.unique(self.ctx.CCDS['BAND'])
        self.ctx.NBANDS = len(self.ctx.BANDS)

    def get_CCDS(self,**kwargs): 

        '''
        Get the database query that returns the ccds and store them in a numpy record array

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

        print "# Will execute the query:\n%s\n" %  query
        # Get the ccd images that are part of the DESTILE
        #self.ctx.CCDS = despyastro.genutil.query2dict_of_columns(query,dbhandle=self.ctx.dbh,array=True)
        t0 = time.time()
        self.ctx.CCDS = despyastro.genutil.query2rec(query,dbhandle=self.ctx.dbh)
        print "# Query time: %s" % elapsed_time(t0)
        print "# Nelem %s" % len(self.ctx.CCDS['FILENAME'])
        return 


    def get_fitsfile_locations(self):

        """ Find the location of the files in the des archive and https urls"""

        # Get all of the relevant kwargs
        kwargs = self.ctx.get_kwargs_dict()
        archive_name = kwargs.pop('archive_name', 'desar2home') # or get?

        # Number of images/filenames
        Nimages = len(self.ctx.CCDS['FILENAME'])

        # 1. Figure out the archive root and root_http variables, it returns
        # self.ctx.root_archive and self.ctx.root_https
        self.get_root_archive(archive_name)

        # 2. Construnct the relative path for each file as a combination of PATH + FILENAME
        self.ctx.FILEPATH = npadd(self.ctx.CCDS['PATH'],"/")
        self.ctx.FILEPATH = npadd(self.ctx.FILEPATH,self.ctx.CCDS['FILENAME'])

        # 3. Create the archive locations for each file
        path = [self.ctx.root_archive+"/"]*Nimages
        self.ctx.FILEPATH_ARCHIVE = npadd(path,self.ctx.FILEPATH)

        # 4. Create the https locations for each file
        path = [self.ctx.root_https+"/"]*Nimages
        self.ctx.FILEPATH_HTTPS   = npadd(path,self.ctx.FILEPATH)
        return

    def get_root_archive(self,archive_name='desar2home'):

        """
        Get the archive root and root_https fron the database
        """

        cur = self.ctx.dbh.cursor()
        
        # archive root
        query = "select root from ops_archive where name='%s'" % archive_name
        print "# Getting the archive root name for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur.execute(query)
        self.ctx.root_archive = cur.fetchone()[0]
        print "# root_archive: %s" % self.ctx.root_archive

        # root_https
        query = "select val from ops_archive_val where name='%s' and key='root_https'" % archive_name
        print "# Getting root_https for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur.execute(query)
        self.ctx.root_https = cur.fetchone()[0]
        print "# root_https: %s" % self.ctx.root_https

        cur.close()
        return

    def write_info(self,ccdsinfo_file,names=['FILENAME','PATH','BAND','MAG_ZERO']):

        print "# Writing CCDS information to: %s" % ccdsinfo_file
        #names = self.ctx.CCDS.dtype.names
        N = len(names)
        header =  "%12s "*N % tuple(names)
        format =  "%-12s "*N 
        variables = []
        for name in names:
            variables.append(self.ctx.CCDS[name].tolist())
        tableio.put_data(ccdsinfo_file,tuple(variables), header=header, format=format)
        return

    def write_info_json(self,ccdsinfo_jsonfile,names=['FILENAME','PATH','BAND','MAG_ZERO']):

        print "# Writing CCDS information to: %s" % ccdsinfo_jsonfile
        # Make them lists instead of rec arrays
        dict_CCDS = {}
        for name in names:
            dict_CCDS[name] =  self.ctx.CCDS[name].tolist()
        o = open(ccdsinfo_jsonfile,"w")
        o.write(json.dumps(dict_CCDS,sort_keys=True,indent=4))
        o.close()
        return

    def check_dbh(self):

        """ Check if we have a valid database handle (dbh)"""
        
        if 'dbh' not in self.ctx:
            try:
                db_section = self.ctx.get('db_section','db-desoper')
                print "# Creating db-handle to section: %s" % db_section
                self.ctx.dbh = desdbi.DesDbi(section=db_section)
            except:
                raise ValueError('ERROR: Database handler could not be provided for context.')
        else:
            print "# Will recycle existing db-handle"
        return

    def __str__(self):
        return 'find ccds in tile'


def read_tileinfo(geomfile):

    print "# Reading the tile Geometry from file: %s" % geomfile
    with open(geomfile, 'rb') as fp:
        json_dict = json.load(fp)
    return json_dict

def cmdline():

    """
    The function to generate and populate the commnand-line arguments into the context
    """

    import argparse
    parser = argparse.ArgumentParser(description="Find the CCDs that fall inside a tile")

    # The positional arguments
    parser.add_argument("tileinfo", help="The json file with the tile information")

    # Optional arguments
    parser.add_argument("--db_section", action="store", default="db-desoper",
                        help="DataBase Section to connect")
    parser.add_argument("--archive_name", action="store", default="desar2home",
                        help="DataBase Archive Name section")
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
    parser.add_argument("--plot_overlap", action="store_true", default=False, 
                        help="Plot overlapping tiles")
    parser.add_argument("--plot_outname", action="store", default=None, 
                        help="Output file name for plot")
    parser.add_argument("--tiledir", action="store", default="./", 
                        help="Path to Directory where we will write out plot the files ")
    args = parser.parse_args()
    return args

if __name__ == "__main__":

    from mojo.utils.struct import Struct

    args = cmdline()

    # Initialize the class
    job = Job()
    # Add cmdline options to the context using Struct
    job.ctx = Struct(dict(**args.__dict__))
    # Load in the info in the input json file
    json_dict = read_tileinfo(args.tileinfo)
    job.ctx.tileinfo =json_dict['tileinfo']
    job.ctx.tilename =json_dict['tilename']
    job()  # Execute -- do run()
    # Write out the ccds information
    #job.write_info(args.ccdsinfo)
    #job.write_info_json(args.ccdsinfo)
    #exit()

    # In Case we want to plot the overlapping CCDs
    if args.plot_overlap:
        # FELIPE: Add check for existence of self.ctx.tiledir
        print "# Will plot overlapping tiles"
        from multiepoch.tasks.plot_ccd_corners_destile import Job as plot_job
        plot = plot_job(ctx=job.ctx) 
        plot()


