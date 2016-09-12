
"""
Set of common query tasks for the multi-epoch pipeline
"""

import despyastro
import numpy
import time
from despymisc.miscutils import elapsed_time
from multiepoch import utils
from multiepoch import file_handler as fh

# -------------------
# QUERY STRINGS
# -------------------

# The query template used to get the geometry of the tile
QUERY_GEOM = """
    SELECT ID, PIXELSCALE, NAXIS1, NAXIS2,
        RA_CENT, DEC_CENT,
        RA_SIZE,DEC_SIZE,
        RAC1, RAC2, RAC3, RAC4,
        DECC1, DECC2, DECC3, DECC4,
        RACMIN,RACMAX,DECCMIN,DECCMAX,
        CROSSRA0
    FROM {coaddtile_table}
    WHERE tilename='{tilename}'
"""

# Using the pre-computed felipe.ME_IMAGES_<TAGNAME> table, this is significantly faster
QUERY_ME_NP_TEMPLATE = """
     SELECT
         {select_extras}
         me.FILENAME,me.COMPRESSION,me.PATH,me.BAND,
         me.RA_CENT,me.DEC_CENT,
         me.RAC1,  me.RAC2,  me.RAC3,  me.RAC4,
         me.DECC1, me.DECC2, me.DECC3, me.DECC4
     FROM
         {from_extras} 
         felipe.me_images_{tagname} me
         """

DISTANCE_METHOD = """
         (ABS(me.RA_CENT  -  {ra_center_tile})  < (0.5*{ra_size_tile}  + 0.5*me.RA_SIZE)) AND
         (ABS(me.DEC_CENT -  {dec_center_tile}) < (0.5*{dec_size_tile} + 0.5*me.DEC_SIZE))
"""


QUERY_ME_IMAGES_TEMPLATE = """
     SELECT
         {select_extras}
         {select_zeropoint}
         me.FILENAME,me.COMPRESSION,me.PATH,
         me.FILENAME_BKG,me.COMPRESSION_BKG,me.PATH_BKG,
         me.FILENAME_SEG,me.COMPRESSION_SEG,me.PATH_SEG,
         me.BAND,me.EXPNUM,
         me.RA_SIZE,me.DEC_SIZE,
         me.RA_CENT, me.RAC1,  me.RAC2,  me.RAC3,  me.RAC4,
         me.DEC_CENT,me.DECC1, me.DECC2, me.DECC3, me.DECC4
     FROM
         {from_extras}
         {from_zeropoint}
         felipe.me_images_{tagname} me
     WHERE
         {and_ccdnum}
         {and_extras}
         {and_blacklist}
         {and_zeropoint}
         {search_method}
"""

QUERY_ME_IMAGES_TEMPLATE_RAZERO = """
 with me as 
    (SELECT /*+ materialize */
         FILENAME,COMPRESSION,PATH,
         FILENAME_BKG,COMPRESSION_BKG,PATH_BKG,
         FILENAME_SEG,COMPRESSION_SEG,PATH_SEG,
         BAND,EXPNUM,CCDNUM,
         RA_SIZE,DEC_SIZE,
         (case when RA_CENT > 180. THEN RA_CENT-360. ELSE RA_CENT END) as RA_CENT, 
         (case when RAC1 > 180.    THEN RAC1-360.    ELSE RAC1 END) as RAC1,	  
         (case when RAC2 > 180.    THEN RAC2-360.    ELSE RAC2 END) as RAC2,		
         (case when RAC3 > 180.    THEN RAC3-360.    ELSE RAC3 END) as RAC3,
         (case when RAC4 > 180.    THEN RAC4-360.    ELSE RAC4 END) as RAC4,
         (case when RACMIN > 180.  THEN RACMIN-360.  ELSE RACMIN END) as RACMIN,
         (case when RACMAX > 180.  THEN RACMAX-360.  ELSE RACMIN END) as RACMAX,
         DEC_CENT, DECC1, DECC2, DECC3, DECC4, DECCMIN, DECCMAX
     FROM felipe.me_images_{tagname})
  SELECT 
         {select_extras}
         {select_zeropoint}
         me.FILENAME,me.COMPRESSION,me.PATH,
         me.FILENAME_BKG,me.COMPRESSION_BKG,me.PATH_BKG,
         me.FILENAME_SEG,me.COMPRESSION_SEG,me.PATH_SEG,
         me.BAND,me.EXPNUM,
         me.RA_SIZE,me.DEC_SIZE,
         me.RA_CENT, me.RAC1,  me.RAC2,  me.RAC3,  me.RAC4,
         me.DEC_CENT,me.DECC1, me.DECC2, me.DECC3, me.DECC4
     FROM
         {from_extras}
         {from_zeropoint}
         me
     WHERE
         {and_extras}
         {and_blacklist}
         {and_zeropoint}
         {search_method}
"""


QUERY_ME_CATALOGS_TEMPLATE = """
     SELECT 
         {cats_select_extras}
         cat.FILENAME, cat.PATH, cat.BAND,cat.CCDNUM, cat.EXPNUM
     FROM 
         felipe.me_cats_{tagname} cat
     WHERE
         cat.EXPNUM in
       (SELECT 
         distinct me.EXPNUM
        FROM
         {cats_from_extras} 
         felipe.me_images_{tagname} me
        WHERE
         {cats_and_extras}
         {search_method}
         )
         order by cat.FILENAME
"""

QUERY_ME_CATALOGS_TEMPLATE_RAZERO = """
     SELECT 
         {cats_select_extras}
         cat.FILENAME, cat.PATH, cat.BAND,cat.CCDNUM, cat.EXPNUM
     FROM 
         felipe.me_cats_{tagname} cat
     WHERE
         cat.expnum in
       (
         with me as 
          (SELECT /*+ materialize */ 
            EXPNUM,
            RA_SIZE,DEC_SIZE,
            (case when RA_CENT > 180. THEN RA_CENT-360. ELSE RA_CENT END) as RA_CENT,
            (case when RAC1 > 180.    THEN RAC1-360.    ELSE RAC1 END) as RAC1,
            (case when RAC2 > 180.    THEN RAC2-360.    ELSE RAC2 END) as RAC2, 	 
            (case when RAC3 > 180.    THEN RAC3-360.    ELSE RAC3 END) as RAC3,
            (case when RAC4 > 180.    THEN RAC4-360.    ELSE RAC4 END) as RAC4,
            (case when RACMIN > 180.  THEN RACMIN-360.  ELSE RACMIN END) as RACMIN,
            (case when RACMAX > 180.  THEN RACMAX-360.  ELSE RACMIN END) as RACMAX,
            DEC_CENT, DECC1, DECC2, DECC3, DECC4, DECCMIN, DECCMAX
          FROM felipe.me_images_{tagname})

       SELECT 
         distinct me.EXPNUM
        FROM
         {cats_from_extras} 
         me
        WHERE
         {cats_and_extras}
         {search_method}
         )
         
         order by cat.FILENAME
"""

QUERY_ME_SCAMPCAT_TEMPLATE = """
     SELECT 
         {cats_select_extras}
         cat.FILENAME_SCAMPCAT,
         cat.FILENAME_SCAMPHEAD,
         cat.PATH, cat.BAND, cat.EXPNUM
     FROM 
         felipe.me_scampcat_{tagname} cat
     WHERE
        cat.EXPNUM in
       (SELECT 
         distinct me.EXPNUM
        FROM
         {cats_select_extras}
         felipe.me_images_{tagname} me
        WHERE
         {cats_and_extras}
         {search_method}
         )
         order by cat.BAND
"""

QUERY_ME_SCAMPCAT_TEMPLATE_RAZERO = """
     SELECT 
         {select_extras}
         cat.FILENAME_SCAMPCAT,
         cat.FILENAME_SCAMPHEAD,
         cat.PATH, cat.BAND, cat.EXPNUM
     FROM
         felipe.me_scampcat_{tagname} cat
     WHERE
         cat.EXPNUM in
       (
         with me as 
          (SELECT /*+ materialize */ 
            EXPNUM,
            RA_SIZE,DEC_SIZE,
            (case when RA_CENT > 180. THEN RA_CENT-360. ELSE RA_CENT END) as RA_CENT,
            (case when RAC1 > 180.    THEN RAC1-360.    ELSE RAC1 END) as RAC1,
            (case when RAC2 > 180.    THEN RAC2-360.    ELSE RAC2 END) as RAC2, 	 
            (case when RAC3 > 180.    THEN RAC3-360.    ELSE RAC3 END) as RAC3,
            (case when RAC4 > 180.    THEN RAC4-360.    ELSE RAC4 END) as RAC4,
            DEC_CENT, DECC1, DECC2, DECC3, DECC4
          FROM felipe.me_images_{tagname})

       SELECT 
         distinct me.EXPNUM
        FROM
         {from_extras} 
         me
        WHERE
         {and_extras}
         {search_method}
         )
         
         order by cat.BAND
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
    return tileinfo

def get_CCDS_from_db_general_sql(dbh, **kwargs): 
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
    search_type   = kwargs.get('search_type','distance')
    no_zeropoint  = kwargs.get('no_zeropoint',False)
    no_blacklist  = kwargs.get('no_blacklist',False) 
    zp_source     = kwargs.get('zp_source')
    zp_version    = kwargs.get('zp_version')
    zp_flag       = kwargs.get('zp_flag') 
    tilename      = kwargs.get('tilename') 
    ccdnum        = kwargs.get('ccdnum',0) 

    utils.pass_logger_debug("Building and running the query to find the CCDS",logger)

    # Decide the template to use
    if tileinfo['CROSSRA0'] == 'Y':
        QUERY_CCDS = QUERY_ME_IMAGES_TEMPLATE_RAZERO
    else:
        QUERY_CCDS = QUERY_ME_IMAGES_TEMPLATE

    # Get extra query strings for blacklist and zeropoint
    query_zeropoint = get_zeropoint_query(zp_source=zp_source,zp_version=zp_version,zp_flag=zp_flag,no_zeropoint=no_zeropoint)
    query_blacklist = get_blacklist_query(no_blacklist=no_blacklist)

    # Get the optional CCDNUM
    if ccdnum > 0:
        and_ccdnum = "me.CCDNUM=%d AND" % ccdnum
    else:
        and_ccdnum = ""

    # Format the SQL query string
    ccd_query = QUERY_CCDS.format(
        tagname         = tagname,
        select_extras   = select_extras,
        from_extras     = from_extras,
        search_method   = get_search_method(search_type,tileinfo),
        and_extras      = and_extras,
        select_zeropoint = query_zeropoint['select_zeropoint'],
        from_zeropoint  = query_zeropoint['from_zeropoint'],
        and_zeropoint   = query_zeropoint['and_zeropoint'],
        and_blacklist   = query_blacklist['and_blacklist'],
        and_ccdnum      = and_ccdnum,
        )
    utils.pass_logger_info("Will execute the query:\n%s\n" %  ccd_query,logger)
    
    # Get the ccd images that are part of the DESTILE
    t0 = time.time()
    CCDS = despyastro.query2rec(ccd_query, dbhandle=dbh)
    utils.pass_logger_info("Query time for CCDs: %s" % elapsed_time(t0),logger)
    
    if CCDS is False:
        utils.pass_logger_info("Found 0 input images for %s " %  tilename,logger)
        utils.pass_logger_info("No input images found", logger)
        return CCDS
    
    utils.pass_logger_info("Found %s input images for %s " %  (len(CCDS),tilename),logger)

    # Here we fix 'COMPRESSION (BKG/SEG)' from None --> '' if present
    if 'COMPRESSION' in CCDS.dtype.names:
        compression = [ '' if c is None else c for c in CCDS['COMPRESSION'] ]
        CCDS['COMPRESSION'] = numpy.array(compression)

    if 'COMPRESSION_BKG' in CCDS.dtype.names:
        compression = [ '' if c is None else c for c in CCDS['COMPRESSION_BKG'] ]
        CCDS['COMPRESSION_BKG'] = numpy.array(compression)

    if 'COMPRESSION_SEG' in CCDS.dtype.names:
        compression = [ '' if c is None else c for c in CCDS['COMPRESSION_SEG'] ]
        CCDS['COMPRESSION_SEG'] = numpy.array(compression)

    return CCDS 


def get_CCDS_from_db_distance_np(dbh, **kwargs): 
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

    ccd_query = QUERY_ME_NP_TEMPLATE.format(
        tagname       = tagname,
        select_extras = select_extras,
        from_extras   = from_extras,
        and_zeropoint = get_and_zeropoint(tagname,no_zeropoint),
        )

    # If and_extras whe need to add the WHERE statement
    if and_extras !='':
        ccd_query = ccd_query + "\nWHERE\n" + and_extras

    utils.pass_logger_info("Will execute the query:\n%s\n" %  ccd_query,logger)
    
    # Get the ccd images that are part of the DESTILE
    t0 = time.time()
    CCDS = despyastro.query2rec(ccd_query, dbhandle=dbh)
    CCDS = utils.update_CCDS_RAZERO(CCDS,crossrazero=tileinfo['CROSSRA0'])
    CCDS = find_distance(CCDS,tileinfo)

    if CCDS is False:
        utils.pass_logger_info("No input images found", logger)
        return CCDS
    
    utils.pass_logger_info("Query time for CCDs: %s" % elapsed_time(t0),logger)
    utils.pass_logger_info("Found %s input images" %  len(CCDS),logger)

    # Here we fix 'COMPRESSION' from None --> '' if present
    if 'COMPRESSION' in CCDS.dtype.names:
        CCDS['COMPRESSION'] = numpy.where(CCDS['COMPRESSION'],CCDS['COMPRESSION'],'')
    return CCDS 


def get_CATS_from_db_general_sql(dbh, **kwargs): 
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
    cats_select_extras = kwargs.get('cats_select_extras','')
    cats_from_extras   = kwargs.get('cats_from_extras','')
    cats_and_extras    = kwargs.get('cats_and_extras','') 
    search_type   = kwargs.get('search_type','distance')

    utils.pass_logger_debug("Building and running the query to find the finalcut catalogs",logger)

    # Decide the template to use
    if tileinfo['CROSSRA0'] == 'Y':
        QUERY_CATS = QUERY_ME_CATALOGS_TEMPLATE_RAZERO
    else:
        QUERY_CATS = QUERY_ME_CATALOGS_TEMPLATE

    # Format the SQL query string
    cat_query = QUERY_CATS.format(
        tagname         = tagname,
        cats_select_extras   = cats_select_extras,
        cats_from_extras     = cats_from_extras,
        cats_and_extras      = cats_and_extras,
        search_method   = get_search_method(search_type,tileinfo),
        )
    utils.pass_logger_info("Will execute the query:\n%s\n" %  cat_query,logger)
    
    # Get the ccd images that are part of the DESTILE
    t0 = time.time()
    CATS = despyastro.query2rec(cat_query, dbhandle=dbh)
    if CATS is False:
        utils.pass_logger_info("No input images found", logger)
        return CATS
    
    utils.pass_logger_info("Query time for catalogs: %s" % elapsed_time(t0),logger)
    utils.pass_logger_info("Found %s input catalogs" %  len(CATS),logger)

    return CATS


def get_SCAMPCATS_from_db_general_sql(dbh, **kwargs): 
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
    search_type   = kwargs.get('search_type','distance')

    utils.pass_logger_debug("Building and running the query to find the SCAMPCAT and HEAD catalogs",logger)

    # Decide the template to use
    if tileinfo['CROSSRA0'] == 'Y':
        QUERY_SCAMPCATS = QUERY_ME_SCAMPCAT_TEMPLATE_RAZERO
    else:
        QUERY_SCAMPCATS = QUERY_ME_SCAMPCAT_TEMPLATE

    # Format the SQL query string
    cat_query = QUERY_SCAMPCATS.format(
        tagname         = tagname,
        select_extras   = select_extras,
        from_extras     = from_extras,
        search_method   = get_search_method(search_type,tileinfo),
        and_extras      = and_extras,
        )
    utils.pass_logger_info("Will execute the query:\n%s\n" %  cat_query,logger)
    
    # Get the ccd images that are part of the DESTILE
    t0 = time.time()
    SCAMPCATS = despyastro.query2rec(cat_query, dbhandle=dbh)
    if SCAMPCATS is False:
        utils.pass_logger_info("No input images found", logger)
        return SCAMPCATS
    
    utils.pass_logger_info("Query time for catalogs: %s" % elapsed_time(t0),logger)
    utils.pass_logger_info("Found %s input scampcat catalogs" %  len(SCAMPCATS),logger)

    return SCAMPCATS

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


def find_distance(CCDS,tileinfo):

    """ Method using numpy"""

    # Center of TILE and CCDS
    RA_CENT_CCD   = CCDS['RA_CENT']
    DEC_CENT_CCD  = CCDS['DEC_CENT']
    RA_CENT_TILE  = tileinfo['RA_CENT']
    DEC_CENT_TILE = tileinfo['DEC_CENT']
    # SIZE of TILE and CCDS
    RA_SIZE_CCD   = numpy.abs(CCDS['RAC2']  - CCDS['RAC3'])
    DEC_SIZE_CCD  = numpy.abs(CCDS['DECC1'] - CCDS['DECC2'])
    RA_SIZE_TILE  = tileinfo['RA_SIZE']
    DEC_SIZE_TILE = tileinfo['DEC_SIZE']

    idx = numpy.logical_and(
        numpy.abs(RA_CENT_CCD  - RA_CENT_TILE)  < ( 0.5*RA_SIZE_TILE  + 0.5*RA_SIZE_CCD),
        numpy.abs(DEC_CENT_CCD - DEC_CENT_TILE) < ( 0.5*DEC_SIZE_TILE + 0.5*DEC_SIZE_CCD)
        )
    return CCDS[idx]


def get_tile_edges(tileinfo):
    """ Get the tile edges from the tileinfo dictionary"""
    tile_edges = (tileinfo['RACMIN'], tileinfo['RACMAX'],
                  tileinfo['DECCMIN'], tileinfo['DECCMAX'])
    return tile_edges


def get_search_method(search_type,tileinfo):
    """ Get the search_method string to add to the SQL query"""

    if search_type == 'distance':
        search_method = DISTANCE_METHOD.format(
            ra_center_tile  = tileinfo['RA_CENT'],
            dec_center_tile = tileinfo['DEC_CENT'],
            ra_size_tile    = tileinfo['RA_SIZE'],
            dec_size_tile   = tileinfo['DEC_SIZE'],
            )
    elif search_type == 'corners':
        corners_query = """
        ((me.RACMIN  BETWEEN {tile_racmin} AND {tile_racmax}) OR (me.RACMAX  BETWEEN {tile_racmin} AND {tile_racmax})) AND
        ((me.DECCMIN BETWEEN {tile_deccmin} AND {tile_deccmax}) OR (me.DECCMAX BETWEEN {tile_deccmin} AND {tile_deccmax}))
        """
        search_method = corners_query.format(tile_racmin=tileinfo['RACMIN'],
                                             tile_racmax=tileinfo['RACMAX'],
                                             tile_deccmin=tileinfo['DECCMIN'],
                                             tile_deccmax=tileinfo['DECCMAX'])
    else:
        exit("ERROR: Search method is not supported")

    return search_method



def get_zeropoint_query(zp_source,zp_version,zp_flag,no_zeropoint=False):
    query = {}
    if no_zeropoint:
        query['and_zeropoint'] = ''
        query['select_zeropoint'] = ''
        query['from_zeropoint'] = ''
    else:
        query['and_zeropoint'] = """
        ZEROPOINT.IMAGENAME = me.filename AND
        ZEROPOINT.SOURCE  = '%s' AND
        ZEROPOINT.VERSION = '%s' AND
        ZEROPOINT.FLAG    <  %s AND """ % (zp_source,zp_version,zp_flag)
        query['select_zeropoint'] = "ZEROPOINT.MAG_ZERO,"
        query['from_zeropoint'] = "ZEROPOINT,"
    return query


def get_blacklist_query(no_blacklist=False):
    query = {}
    if no_blacklist:
        query['and_blacklist'] = ''
    else:
        query['and_blacklist'] = """
         not exists (select bl.reason from blacklist bl where bl.expnum=me.expnum and bl.ccdnum=me.ccdnum) AND"""
        #query['and_blacklist'] = """
        #me.filename NOT IN
        #(select filename from felipe.me_images_%s me, BLACKLIST where
        #me.expnum=blacklist.expnum and me.ccdnum=blacklist.ccdnum) AND """ % tagname
    return query



def find_mangle_pol_files(tileid,dbh,version='Y3A1v1'):

    """ Find the pol files in the database"""

    poltile = fh.get_poltiles_name(tileid,version=version)
    poltoly = fh.get_poltolys_name(tileid,version=version)
    QUERY = """select PATH, FILENAME from  FILE_ARCHIVE_INFO where filename='{poltile}' or filename='{poltoly}'"""
    q = QUERY.format(poltile=poltile, poltoly=poltoly)
    print "Executing query:\n\t %s\n" % q
    pols = despyastro.query2rec(q,dbhandle=dbh)
    return pols
