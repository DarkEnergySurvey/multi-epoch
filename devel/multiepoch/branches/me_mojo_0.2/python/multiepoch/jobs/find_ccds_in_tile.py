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

"""

import numpy
import despyastro
from mojo.jobs.base_job import BaseJob

# THE QUERY THAT IS RUN TO GET THE CCDs
# -----------------------------------------------------------------------------
#

# DEPRECATED
QUERY_OLDSCHEMA = """
    SELECT
        {select_extras}
        filepath_desar.PATH, image.IMAGENAME, filepath_desar, COMPRESSION, image.BAND, image.RUN,
        image.PROJECT, image.IMAGETYPE, image.ID, c.RA, c.RAC1, c.RAC2, c.RAC3,
        c.RAC4, c.DEC, c.DECC1, c.DECC2, c.DECC3, c.DECC4
    FROM
        IMAGE, felipe.IMAGECORNERS c, FILEPATH_DESAR,
        {from_extras} 
    WHERE
        image.IMAGENAME=c.IMAGENAME
    AND
        filepath_desar.ID=IMAGE.ID
    AND
        {and_extras} 
     """

""" This the query in the new schema based in FILENAME
It will get:
  - the filepath of the FILENANE.
  - corners of FILENAME
  - within a tagname (i.e. Y2T1_FIRSTCUT) 
  - for FILENAME that are 'red'
  - for FILENAME with a given EXEC_NAME (i.e. 'immask')
"""

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

# DEFAULT PARAMETER VALUES
# -----------------------------------------------------------------------------
SELECT_EXTRAS = "felipe.extraZEROPOINT.FILENAME, felipe.extraZEROPOINT.MAG_ZERO,"
FROM_EXTRAS   = "felipe.extraZEROPOINT"
AND_EXTRAS    = "file_archive_info.FILENAME  = felipe.extraZEROPOINT.FILENAME"
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

        return query


    def __call__(self):

        # FM writes...
        # Comment for Michael:
        # I'd like to enforce 'tagname' to exist, and not be an optional kwarg
        # such as:
        #    query = self.get_query(tagname, **self.ctx.get_kwargs_dict())

        # Call the query built function
        query = self.get_query(**self.ctx.get_kwargs_dict())
        
        if self.ctx.verbose:
            print "# Getting images within the tile: %s\n %s" % (\
                    self.ctx.tilename, query)
        # Get the ccd images that are part of the DESTILE
        #self.ctx.CCDS = despyastro.genutil.query2dict_of_columns(query,dbhandle=self.ctx.dbh,array=True)
        self.ctx.CCDS = despyastro.genutil.query2rec(query,dbhandle=self.ctx.dbh)
        print "# Nelem %s" % len(self.ctx.CCDS['FILENAME'])

        # Get the filters we found
        self.ctx.BANDS  = numpy.unique(self.ctx.CCDS['BAND'])
        self.ctx.NBANDS = len(self.ctx.BANDS)


    def __str__(self):
        return 'find ccds in tile'
