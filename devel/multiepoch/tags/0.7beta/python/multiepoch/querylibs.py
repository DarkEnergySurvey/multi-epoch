
"""
Set of common query tasks for the multi-epoch pipeline
"""

import despyastro
import numpy

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
         file_archive_info.FILENAME,file_archive_info.COMPRESSION,
         file_archive_info.PATH,
         image.BAND,
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
     

# -----------------------------------------------------------------------------        
# QUERY FUNCTIONS
# -----------------------------------------------------------------------------

def get_tileinfo_from_db(dbh, **kwargs):
    """
    Execute database query to get geometry inforation for a tile.
    """
    
    logger = kwargs.pop('logger', None)
    query_geom = QUERY_GEOM.format(**kwargs)
    
    mess = "Getting geometry information for tile:%s" % kwargs.get('tilename')
    if logger: logger.info(mess)
    else: print mess
    
    cur = dbh.cursor()
    cur.execute(query_geom)
    desc = [d[0] for d in cur.description]
    # cols description
    line = cur.fetchone()
    cur.close()
    # Make a dictionary/header for all the columns from COADDTILE table
    tileinfo = dict(zip(desc, line))
    
    return tileinfo

def get_CCDS_from_db(dbh, tile_edges, **kwargs): 
    """
    Execute the database query that returns the ccds and store them in a numpy record array
    kwargs: exec_name, tagname, select_extras, and_extras, from_extras
    """

    logger = kwargs.pop('logger', None)

    mess = "Building and running the query to find the CCDS"
    if logger: logger.debug(mess)
    else: print mess

    corners_and = [
        "((image.RAC1 BETWEEN %.10f AND %.10f) AND (image.DECC1 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC2 BETWEEN %.10f AND %.10f) AND (image.DECC2 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC3 BETWEEN %.10f AND %.10f) AND (image.DECC3 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC4 BETWEEN %.10f AND %.10f) AND (image.DECC4 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        ]
    ccd_query = QUERY_CCDS.format(
        tagname       = kwargs.get('tagname'),
        exec_name     = kwargs.get('exec_name',     'immask'),
        select_extras = kwargs.get('select_extras'),
        from_extras   = kwargs.get('from_extras'),
        and_extras    = kwargs.get('and_extras')+  ' AND\n (' + ' OR '.join(corners_and) + ')',
        )

    mess = "Will execute the query:\n%s\n" %  ccd_query
    if logger: logger.info(mess)
    else: print mess
    
    # Get the ccd images that are part of the DESTILE
    CCDS = despyastro.genutil.query2rec(ccd_query, dbhandle=dbh)

    mess = "Found %s input images" %  len(CCDS)
    if logger: logger.info(mess)
    else: print mess


    # Here we fix 'COMPRESSION' from None --> '' if present
    if 'COMPRESSION' in CCDS.dtype.names:
        CCDS['COMPRESSION'] = numpy.where(CCDS['COMPRESSION'],CCDS['COMPRESSION'],'')

    return CCDS 


# -------------------------------------------------------------------------
# QUERY methods for root names -- available to all tasks
# -------------------------------------------------------------------------
def get_root_archive(dbh, archive_name='desar2home', logger=None):
    """ Get the root-archive fron the database
    """
    cur = dbh.cursor()
    # root_archive
    query = "SELECT root FROM ops_archive WHERE name='%s'" % archive_name
    if logger:
        logger.debug("Getting the archive root name for section: %s" % archive_name)
        logger.debug("Will execute the SQL query:\n********\n** %s\n********" % query)
    cur.execute(query)
    root_archive = cur.fetchone()[0]
    if logger: logger.info("root_archive: %s" % root_archive)
    return root_archive


def get_root_https(dbh, archive_name='desar2home', logger=None):
    """ Get the root_https fron the database
    """
    cur = dbh.cursor()
    # root_https
    # to add it:
    # insert into ops_archive_val (name, key, val) values ('prodbeta', 'root_https', 'https://desar2.cosmology.illinois.edu/DESFiles/Prodbeta/archive');
    query = "SELECT val FROM ops_archive_val WHERE name='%s' AND key='root_https'" % archive_name
    if logger:
        logger.debug("Getting root_https for section: %s" % archive_name)
        logger.debug("Will execute the SQL query:\n********\n** %s\n********" % query)
    cur.execute(query)
    root_https = cur.fetchone()[0]
    if logger: logger.info("root_https:   %s" % root_https)
    cur.close()
    return root_https


def get_root_http(dbh, archive_name='desar2home', logger=None):
    """ Get the root_http  fron the database
    """
    cur = dbh.cursor()
    # root_http 
    query = "SELECT val FROM ops_archive_val WHERE name='%s' AND key='root_http'" % archive_name
    if logger:
        logger.debug("Getting root_https for section: %s" % archive_name)
        logger.debug("Will execute the SQL query:\n********\n** %s\n********" % query)
    cur.execute(query)
    root_http  = cur.fetchone()[0]
    if logger: logger.info("root_http:   %s" % root_http)
    cur.close()
    return root_http

# -----------------------------------------------------------------------------


