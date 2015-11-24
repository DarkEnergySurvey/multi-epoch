
"""
Set of common query tasks for the multi-epoch pipeline
"""

import despyastro
import numpy
import time
from despymisc.miscutils import elapsed_time
from multiepoch import utils 

# -------------------
# QUERY STRINGS
# -------------------

# The query template used to get the geometry of the tile
QUERY_GEOM = """
    SELECT PIXELSCALE, NAXIS1, NAXIS2,
        RA_CENT, DEC_CENT,
        RA_SIZE,DEC_SIZE,
        RAC1, RAC2, RAC3, RAC4,
        DECC1, DECC2, DECC3, DECC4,
        RACMIN,RACMAX,DECCMIN,DECCMAX,
        CROSSRAZERO
    FROM {coaddtile_table}
    WHERE tilename='{tilename}'
"""

# Using the pre-computed felipe.ME_INPUTS_<TAGNAME> table, this is significantly faster
QUERY_ME_INPUTS_LONG = """
     SELECT
         {select_extras}
         me.FILENAME,me.COMPRESSION,me.PATH,me.BAND,
         me.RA_CENT,me.DEC_CENT,
         me.RAC1,  me.RAC2,  me.RAC3,  me.RAC4,
         me.DECC1, me.DECC2, me.DECC3, me.DECC4
     FROM
         {from_extras} 
         felipe.me_inputs_{tagname} me
     WHERE
         {and_extras} 
         """

QUERY_ME_INPUTS = """
     SELECT
         {select_extras}
         me.FILENAME,me.COMPRESSION,me.PATH,me.BAND,
         me.RA_CENT,me.DEC_CENT,
         me.RAC1,  me.RAC2,  me.RAC3,  me.RAC4,
         me.DECC1, me.DECC2, me.DECC3, me.DECC4
     FROM
         {from_extras} 
         felipe.me_inputs_{tagname} me
         """

QUERY_ME_TEMPLATE = """
     SELECT
         {select_extras}
         me.FILENAME,me.COMPRESSION,me.PATH,me.BAND,
         me.RA_CENT, me.RAC1,  me.RAC2,  me.RAC3,  me.RAC4,
         me.DEC_CENT,me.DECC1, me.DECC2, me.DECC3, me.DECC4,
         ABS(me.RAC2  - me.RAC3 )  as RA_SIZE_CCD,
         ABS(me.DECC1 - me.DECC2 ) as DEC_SIZE_CCD	
     FROM
         {from_extras} 
         felipe.me_inputs_{tagname} me
     WHERE
         {and_extras} 
         """

QUERY_ME_TEMPLATE_RAZERO = """
 with me as 
    (SELECT /*+ materialize */ 
          FILENAME, COMPRESSION, PATH, BAND,
          (case when RA_CENT > 180. THEN RA_CENT-360. ELSE RA_CENT END) as RA_CENT, 
          (case when RAC1 > 180.    THEN RAC1-360.    ELSE RAC1 END) as RAC1,	  
          (case when RAC2 > 180.    THEN RAC2-360.    ELSE RAC2 END) as RAC2,		
          (case when RAC3 > 180.    THEN RAC3-360.    ELSE RAC3 END) as RAC3,
          (case when RAC4 > 180.    THEN RAC4-360.    ELSE RAC4 END) as RAC4,
          DEC_CENT, DECC1, DECC2, DECC3, DECC4
     FROM felipe.me_inputs_{tagname})
  SELECT 
         {select_extras}
         me.FILENAME,me.COMPRESSION,me.PATH,me.BAND,
         me.RA_CENT, me.RAC1,  me.RAC2,  me.RAC3,  me.RAC4,
         me.DEC_CENT,me.DECC1, me.DECC2, me.DECC3, me.DECC4,
         ABS(me.RAC2  - me.RAC3 )  as RA_SIZE_CCD,
         ABS(me.DECC1 - me.DECC2 ) as DEC_SIZE_CCD	
     FROM
         {from_extras} 
         me
     WHERE
         {and_extras} 
"""

# ----------------
# QUERY FUNCTIONS
# ----------------
def get_tileinfo_from_db(dbh, **kwargs):
    """
    Execute database query to get geometry inforation for a tile.
    """
    
    logger = kwargs.pop('logger', None)
    query_geom = QUERY_GEOM.format(**kwargs)

    utils.pass_logger_info("Getting geometry information for tile:%s" % kwargs.get('tilename'),logger)
    print  query_geom
    cur = dbh.cursor()
    cur.execute(query_geom)
    desc = [d[0] for d in cur.description]
    # cols description
    line = cur.fetchone()
    cur.close()
    # Make a dictionary/header for all the columns from COADDTILE table
    tileinfo = dict(zip(desc, line))

    # Add RA_SIZE, DEC_SIZE if not present / to make it compatible with older tables
    #if 'RA_SIZE' not in tileinfo:
    #    if tileinfo['CROSSRAZERO'] == 'Y':
    #        tileinfo['RA_SIZE']  = abs(tileinfo['RACMAX']-(tileinfo['RACMIN']-360))
    #    else:
    #        tileinfo['RA_SIZE']  = abs(tileinfo['RACMAX']- tileinfo['RACMIN'])
    #if 'DEC_SIZE' not in tileinfo:
    #    tileinfo['DEC_SIZE'] = abs(tileinfo['DECCMAX']-tileinfo['DECCMIN'])
        
    return tileinfo

def get_CCDS_from_db_distance_sql(dbh, tile_geometry, **kwargs): 
    """
    Execute the database query that returns the ccds and store them in a numpy record array
    kwargs: exec_name, tagname, select_extras, and_extras, from_extras

    This is a more general query, that will work on case that the TILE
    is smaller than the input CCDS
    """

    # Get params from kwargs
    logger        = kwargs.get('logger', None)
    tagname       = kwargs.get('tagname')
    tileinfo      = kwargs.get('tileinfo') 
    select_extras = kwargs.get('select_extras','')
    from_extras   = kwargs.get('from_extras','')
    and_extras    = kwargs.get('and_extras','') 

    utils.pass_logger_debug("Building and running the query to find the CCDS",logger)

    # Get the tile geomtry -- TODO: this could all be part of the tiles definition
    ra_center_tile, dec_center_tile, ra_size_tile, dec_size_tile = get_tile_geometry(tileinfo)

    # Build the The distance AND condition for the query
    distance_and = [
        "ABS(RA_CENT  -  %.10f) < (%.10f + 0.505*ABS(RAC2 - RAC3 )) AND\n" % (ra_center_tile, ra_size_tile*0.5),
        "ABS(DEC_CENT -  %.10f) < (%.10f + 0.505*ABS(DECC1- DECC2))\n"     % (dec_center_tile,dec_size_tile*0.5),
        ]

    if tileinfo['CROSSRAZERO'] == 'Y':
        QUERY_CCDS = QUERY_ME_TEMPLATE_RAZERO
    else:
        QUERY_CCDS = QUERY_ME_TEMPLATE

    ccd_query = QUERY_CCDS.format(
        tagname       = tagname,
        select_extras = select_extras,
        from_extras   = from_extras,
        and_extras    = and_extras,
        )

    ccd_query = ccd_query + "\n" + and_extras +' (\n ' + ' '.join(distance_and) + ')'

    #ccd_query = ccd_query + '  (\n ' + ' '.join(distance_and) + ')'
    utils.pass_logger_info("Will execute the query:\n%s\n" %  ccd_query,logger)
    
    # Get the ccd images that are part of the DESTILE
    t0 = time.time()
    CCDS = despyastro.query2rec(ccd_query, dbhandle=dbh)

    if CCDS is False:
        utils.pass_logger_info("No input images found", logger)
        return CCDS
    
    utils.pass_logger_info("Query time for CCDs: %s" % elapsed_time(t0),logger)
    utils.pass_logger_info("Found %s input images" %  len(CCDS),logger)

    # Here we fix 'COMPRESSION' from None --> '' if present
    if 'COMPRESSION' in CCDS.dtype.names:
        CCDS['COMPRESSION'] = numpy.where(CCDS['COMPRESSION'],CCDS['COMPRESSION'],'')
    return CCDS 



def get_CCDS_from_db_distance_np(dbh, tile_geometry, **kwargs): 
    """
    Execute the database query that returns the ccds and store them in a numpy record array
    kwargs: exec_name, tagname, select_extras, and_extras, from_extras

    This is a more general query, that will work on case that the TILE
    is smaller than the input CCDS
    """

    # Get params from kwargs
    logger        = kwargs.pop('logger', None)
    tagname       = kwargs.get('tagname')
    tileinfo      = kwargs.get('tileinfo') 
    select_extras = kwargs.get('select_extras','')
    from_extras   = kwargs.get('from_extras','')
    and_extras    = kwargs.get('and_extras','') 
    
    utils.pass_logger_debug("Building and running the query to find the CCDS",logger)

    # Get the tile geomtry -- TODO: this could all be part of the tiles definition
    ra_center_tile, dec_center_tile, ra_size_tile, dec_size_tile = get_tile_geometry(tileinfo)

    QUERY_CCDS = QUERY_ME_INPUTS

    ccd_query = QUERY_CCDS.format(
        tagname       = tagname,
        select_extras = select_extras,
        from_extras   = from_extras,
        )

    if and_extras !='':
        ccd_query = ccd_query + "\nWHERE\n" + and_extras

    utils.pass_logger_info("Will execute the query:\n%s\n" %  ccd_query,logger)
    
    # Get the ccd images that are part of the DESTILE
    t0 = time.time()
    CCDS = despyastro.query2rec(ccd_query, dbhandle=dbh)
    CCDS = utils.update_CCDS_RAZERO(CCDS,crossrazero=tileinfo['CROSSRAZERO'])
    CCDS = find_distance(CCDS,tile_geometry)

    if CCDS is False:
        utils.pass_logger_info("No input images found", logger)
        return CCDS
    
    utils.pass_logger_info("Query time for CCDs: %s" % elapsed_time(t0),logger)
    utils.pass_logger_info("Found %s input images" %  len(CCDS),logger)

    # Here we fix 'COMPRESSION' from None --> '' if present
    if 'COMPRESSION' in CCDS.dtype.names:
        CCDS['COMPRESSION'] = numpy.where(CCDS['COMPRESSION'],CCDS['COMPRESSION'],'')
    return CCDS 

def get_CCDS_from_db_corners(dbh, tile_edges, **kwargs): 
    """
    Execute the database query that returns the ccds and store them in a numpy record array
    kwargs: exec_name, tagname, select_extras, and_extras, from_extras

    **** NOTE ***
    This query will only work if the size of the TILE is larger than the size of the input CCD images
    ***********
    """

    QUERY_CCDS = QUERY_ME_INPUTS
    logger = kwargs.pop('logger', None)

    utils.pass_logger_debug("Building and running the query to find the CCDS",logger)

    corners_and = [
        "((image.RAC1 BETWEEN %.10f AND %.10f) AND (image.DECC1 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC2 BETWEEN %.10f AND %.10f) AND (image.DECC2 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC3 BETWEEN %.10f AND %.10f) AND (image.DECC3 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        "((image.RAC4 BETWEEN %.10f AND %.10f) AND (image.DECC4 BETWEEN %.10f AND %.10f))\n" % tile_edges,
        ]

    ccd_query = QUERY_CCDS.format(
        tagname       = kwargs.get('tagname'),
        exec_name     = kwargs.get('exec_name','immask'),
        filetype      = kwargs.get('filetype','red_immask'),
        select_extras = kwargs.get('select_extras'),
        from_extras   = kwargs.get('from_extras'),
        and_extras    = kwargs.get('and_extras') + ' AND\n (' + ' OR '.join(corners_and) + ')'
        )

    utils.pass_logger_debug("Will execute the query:\n%s\n" %  ccd_query,logger)
    
    # Get the ccd images that are part of the DESTILE
    CCDS = despyastro.genutil.query2rec(ccd_query, dbhandle=dbh)
    utils.pass_logger_info("Found %s input images" %  len(CCDS),logger)

    # Here we fix 'COMPRESSION' from None --> '' if present
    if 'COMPRESSION' in CCDS.dtype.names:
        CCDS['COMPRESSION'] = numpy.where(CCDS['COMPRESSION'],CCDS['COMPRESSION'],'')
    return CCDS 

# --------------------------------------------------------
# QUERY methods for root names -- available to all tasks
# --------------------------------------------------------
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

    if archive_name == 'desar2home':
        root_https = "https://desar2.cosmology.illinois.edu/DESFiles/desarchive"
        return root_https

    cur = dbh.cursor()
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


def find_distance(CCDS,tile_geometry):

    (ra_center_tile, dec_center_tile, ra_size_tile, dec_size_tile) = tile_geometry
    
    RA_CENT      = CCDS['RA_CENT']
    DEC_CENT     = CCDS['DEC_CENT']
    RA_SIZE_CCD  = numpy.abs(CCDS['RAC2']  - CCDS['RAC3'])
    DEC_SIZE_CCD = numpy.abs(CCDS['DECC1'] - CCDS['DECC2'])

    RA_dist_centers  = RA_CENT  - ra_center_tile
    DEC_dist_centers = DEC_CENT - dec_center_tile

    idx = numpy.logical_and(
        numpy.abs(RA_CENT  - ra_center_tile)  < ( 0.5*ra_size_tile  + 0.505*RA_SIZE_CCD),
        numpy.abs(DEC_CENT - dec_center_tile) < ( 0.5*dec_size_tile + 0.505*DEC_SIZE_CCD)
        )
    return CCDS[idx]


def get_tile_geometry(tileinfo):
    ra_center_tile  = tileinfo['RA']
    dec_center_tile = tileinfo['DEC']
    dec_size_tile   = abs(tileinfo['DECCMIN'] - tileinfo['DECCMAX'])
    ra_size_tile    = abs(tileinfo['RACMIN']  - tileinfo['RACMAX'])   # RA_size
    return ra_center_tile, dec_center_tile, ra_size_tile, dec_size_tile
