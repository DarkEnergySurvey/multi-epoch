""" Set of common query tasks for the multi-epoch pipeline """

GEOM_QUERY = '''
            SELECT PIXELSCALE, NAXIS1, NAXIS2,
            RA, DEC,
            RAC1, RAC2, RAC3, RAC4,
            DECC1, DECC2, DECC3, DECC4,
            RACMIN,RACMAX,DECCMIN,DECCMAX,
            CROSSRAZERO
            FROM {tablename}
            WHERE tilename='{tilename}'
            '''


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
    
    query_string = GEOM_QUERY.format(tablename=tablename, tilename=tilename)
    return query_string

