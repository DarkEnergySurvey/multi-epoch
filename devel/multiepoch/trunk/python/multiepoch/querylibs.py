""" Set of common query tasks for the multi-epoch pipeline """

import despyastro


# QUERIES
# -----------------------------------------------------------------------------

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

# The query template used to get the the CCDs
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
     
        

# QUERY FUNCTIONS
# -----------------------------------------------------------------------------

def get_tileinfo_from_db(dbh, **kwargs):
    ''' Execute database query to get geometry inforation for a tile.
    '''

    query_geom = QUERY_GEOM.format(**kwargs)
    print "# Getting geometry information for tile:%s" % kwargs.get('tilename')
    cur = dbh.cursor()
    cur.execute(query_geom)
    desc = [d[0] for d in cur.description]
    # cols description
    line = cur.fetchone()
    cur.close()
    # Make a dictionary/header for all the columns from COADDTILE table
    tileinfo = dict(zip(desc, line))

    return tileinfo


# -----------------------------------------------------------------------------

def get_CCDS_from_db(dbh, tile_edges, **kwargs): 
    '''
    Execute the database query that returns the ccds and store them in a numpy
    record array
    kwargs: exec_name, tagname, select_extras, and_extras, from_extras
    '''

    print "# Building the query to find the CCDS"
    corners_and = [
        "((image.RAC1 BETWEEN %.10f AND %.10f) AND (image.DECC1 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC2 BETWEEN %.10f AND %.10f) AND (image.DECC2 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC3 BETWEEN %.10f AND %.10f) AND (image.DECC3 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC4 BETWEEN %.10f AND %.10f) AND (image.DECC4 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        ]
    ccd_query = QUERY_CCDS.format(
        tagname       = kwargs.get('tagname',       'Y2T1_FIRSTCUT'),
        exec_name     = kwargs.get('exec_name',     'immask'),
        select_extras = kwargs.get('select_extras'),
        from_extras   = kwargs.get('from_extras'),
        and_extras    = kwargs.get('and_extras')+  ' AND\n (' + ' OR '.join(corners_and) + ')',
        )

    print "# Will execute the query:\n%s\n" %  ccd_query
    # Get the ccd images that are part of the DESTILE
    CCDS = despyastro.genutil.query2rec(ccd_query, dbhandle=dbh)

    return CCDS 
