""" Set of common query tasks for the multi-epoch pipeline """


# --------------------------------------------------------
# The query template used to get the geometry of the tile
# --------------------------------------------------------
QUERY_GEOM = '''
            SELECT PIXELSCALE, NAXIS1, NAXIS2,
            RA, DEC,
            RAC1, RAC2, RAC3, RAC4,
            DECC1, DECC2, DECC3, DECC4,
            RACMIN,RACMAX,DECCMIN,DECCMAX,
            CROSSRAZERO
            FROM {tablename}
            WHERE tilename='{tilename}'
            '''


# ------------------------------------------------
# The query template to get the the CCDs
# ------------------------------------------------
QUERY_CCDS = """
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
def get_geom_query(**kwargs):
    
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
    
    query_string = QUERY_GEOM.format(tablename=tablename, tilename=tilename)
    return query_string


def get_ccds_query(tile_edges, **kwargs):

    '''
    Get the database query that returns the ccds and store them in a numpy
    record array
    
    kwargs
    ``````
     - exec_name
     - tagname
     - select_extras
     - and_extras
     - from_extras
    '''
    
    select_extras = kwargs.get('select_extras')
    and_extras    = kwargs.get('and_extras')
    from_extras   = kwargs.get('from_extras')
    tagname       = kwargs.get('tagname',       'Y2T1_FIRSTCUT')
    exec_name     = kwargs.get('exec_name',     'immask')
    
    corners_and = [
        "((image.RAC1 BETWEEN %.10f AND %.10f) AND (image.DECC1 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC2 BETWEEN %.10f AND %.10f) AND (image.DECC2 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC3 BETWEEN %.10f AND %.10f) AND (image.DECC3 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC4 BETWEEN %.10f AND %.10f) AND (image.DECC4 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        ]
    
    query = QUERY_CCDS.format(
        tagname       = tagname,
        exec_name     = exec_name,
        select_extras = select_extras,
        from_extras   = from_extras,
        and_extras    = and_extras + ' AND\n (' + ' OR '.join(corners_and) + ')',
        )

    return query
