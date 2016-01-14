#!/usr/bin/env python

import os,sys

import multiepoch.utils as utils
import multiepoch.querylibs as querylibs
import multiepoch.tasks.find_ccds_in_tile as find_ccds_in_tile
from despydb import desdbi
import numpy

# NOTES on how to get the mean values:
# The mean values are:
# grep g tiles_depth_sql_sorted.out | awk '{ sum +=$3 } END { print sum/NR }' --> 65 (64)
# grep r tiles_depth_sql_sorted.out | awk '{ sum +=$3 } END { print sum/NR }' --> 55 (57)
# grep i tiles_depth_sql_sorted.out | awk '{ sum +=$3 } END { print sum/NR }' --> 60 (63)
# grep z tiles_depth_sql_sorted.out | awk '{ sum +=$3 } END { print sum/NR }' --> 53 (58)
# grep Y tiles_depth_sql_sorted.out | awk '{ sum +=$3 } END { print sum/NR }' --> 51 (51)
# The median values:
# grep i tiles_depth_sql_sorted.out | awk ' { a[i++]=$3; } END { print a[int(i/2)]; }'

# The default EXTRAS
SELECT_EXTRAS = find_ccds_in_tile.SELECT_EXTRAS
FROM_EXTRAS   = find_ccds_in_tile.FROM_EXTRAS
AND_EXTRAS    = find_ccds_in_tile.AND_EXTRAS

def get_tile_geometry(tileinfo):
    
    ra_center_tile  = tileinfo['RA']
    dec_center_tile = tileinfo['DEC']
    dec_size_tile   = abs(tileinfo['DECCMIN']-tileinfo['DECCMAX'])
    
    if tileinfo['CROSSRAZERO'] == 'Y':
        ra_size_tile = abs(tileinfo['RACMIN']- 360 - tileinfo['RACMAX'])
    else:
        ra_size_tile   = abs(tileinfo['RACMIN']-tileinfo['RACMAX'])   # RA_size
        
    tile_geometry = (ra_center_tile, dec_center_tile, ra_size_tile, dec_size_tile)
    return tile_geometry


def get_expnames(ccdlist):
    expnames = [ccd[0:9] for ccd in ccdlist]
    expnames = numpy.array(expnames)
    return numpy.unique(expnames)

def createTAGtable(dbh, tablename,clobber=False):
    
    cur = dbh.cursor()

    # Drop call
    drop = "drop table %s purge" % tablename
    # Create call
    create = """
    create table %s (
    FILENAME                VARCHAR2(100),
    BAND                    VARCHAR2(100),
    TAG                     VARCHAR2(100)
    )
    """ % tablename 

    # -- Add description of columns
    comments ="""comment on column %s.FILENAME    is 'FILENAME from IMAGE.FILENAME'
    comment on column %s.BAND   is 'BAND from IMAGE.BAND'
    comment on column %s.TAG    is 'custom TAG' """

    # Check if table exists
    table_exist = checkTABLENAMEexists(dbh,tablename)

    # Drop if exists and clobber=True
    if table_exist and clobber:
        print "# Dropping table: %s" % tablename
        cur.execute(drop)

    # Create if not exist or clobber=True
    if not table_exist or clobber:
        print "# Creating table: %s" % tablename
        cur.execute(create)
        # Comments
        print "# Adding comments to: %s" % tablename
        for comment in comments.split("\n"):
            print comment % tablename
            cur.execute(comment % tablename)

    # Grand permission
    roles = ['DES_READER','PROD_ROLE','PROD_READER_ROLE']
    for role in roles:
        grant = "grant select on %s to %s" % (tablename.split(".")[1],role)
        print "# Granting permission: %s" % grant
        cur.execute(grant)

    dbh.commit()
    cur.close()
    return


def checkTABLENAMEexists(dbh,tablename):

    """
    Check if exists. Tablename has to be a full owner.table_name format"""

    print "# Checking if %s exists" % tablename
    
    # Make sure is all upper case
    tablename = tablename.upper()

    query = """
    select count (*) from all_tables where owner||'.'||table_name='%s'""" % tablename

    cur = dbh.cursor()
    cur.execute(query)
    count = cur.fetchone()[0]
    cur.close()
    
    if count >= 1:
        table_exists = True
    else:
        table_exists = False
    print "# %s exists: %s " % (tablename,table_exists)
    return table_exists


def insertTAG(tilename,myTAG,tagname,tablename='felipe.tags',clobber=False,db_section='db-destest',SELECT_BY="EXPOSURES",NEXP=4):

    options = {
        'tilename'   : tilename,
        'coaddtile_table' : "felipe.coaddtile_new",
        'archive_name'  : "prodbeta",
        'tagname'       : tagname,
        #'exec_name'     : 'immask',
        'select_extras' : SELECT_EXTRAS,
        'from_extras'   : FROM_EXTRAS,
        'and_extras'    : AND_EXTRAS,
        }

    bands = ['g','r','i','z','Y']
    Nmedian = {
        'g':64,
        'r':57,
        'i':63,
        'z':58,
        'Y':51,
        }

    print "# Creating db-handle to section: %s" % db_section
    dbh = desdbi.DesDbi(section=db_section)
    print "# Getting tile geom information"
    tileinfo = querylibs.get_tileinfo_from_db(dbh,**options)
    
    # Create the tile_edges tuple structure and query the database
    tile_geometry = get_tile_geometry(tileinfo)
    CCDS = querylibs.get_CCDS_from_db_distance(dbh, tile_geometry,**options)

    if CCDS is False:
        print "# no CCDS can be TAG for %s" % tilename
        return

    # Old method using corners
    #tile_edges = (tileinfo['RACMIN'], tileinfo['RACMAX'],tileinfo['DECCMIN'],tileinfo['DECCMAX'])
    #CCDS = querylibs.get_CCDS_from_db_corners(dbh, tile_edges,**options)

    # Select random files for each 
    BANDS  = numpy.unique(CCDS['BAND'])
    cur = dbh.cursor()
    cols = "FILENAME,BAND,TAG"
    createTAGtable(dbh, tablename,clobber=clobber)

    for BAND in BANDS:
        idx = numpy.where(CCDS['BAND'] == BAND)[0]
        filenames = CCDS['FILENAME'][idx]
        NCCDs = len(filenames)
        print "# Found %s CCDimages for filter %s overlaping " % (NCCDs, BAND)

        # Decide on the selection method
        if SELECT_BY == "EXPOSURES":
            print "# Will select %s EXPOSURES randomly and insert to %s" % (NEXP,tablename)
            # Get the unique exposure names for that filter
            expnames     = get_expnames(CCDS['FILENAME'][idx])
            expnames_sel = numpy.random.choice(expnames,min(NEXP,len(expnames)),replace=False)
            k = numpy.array( [fname[0:9] in expnames_sel for fname in filenames])
            filenames_sel = filenames[k==True]
        elif SELECT_BY == "CCDS":
            print "# Will select %s CCDS randomly and insert to %s" % (Nmedian[BAND],tablename)
            filenames_sel = numpy.random.choice(filenames,min(Nmedian[BAND],NCCDs),replace=False)
        else:
            exit("ERROR: No selection method")

        # Now we insert
        for fname in filenames_sel:
            values = (fname,BAND,myTAG)
            insert_cmd = "INSERT INTO %s (%s) VALUES %s" % (tablename,cols,values)
            cur.execute(insert_cmd)

    cur.close()
    dbh.commit()
    print "# Done inserting Random TAGS...\n"
    return

if __name__ == "__main__":


    tiles_RXJ2248 = ['DES2251-4331',
                     'DES2251-4414',
                     'DES2254-4457',
                     'DES2247-4331',
                     'DES2247-4414',
                     'DES2246-4457',
                     'DES2250-4457']
    
    tiles_ElGordo = ['DES0105-4831',
                     'DES0059-4957',
                     'DES0103-4957',
                     'DES0058-4914',
                     #'DES0101-4831'
                     'DES0102-4914',
                     'DES0106-4914']

    tiles_crossRA0 = ['DES2354+0043',
                      'DES2356+0043',
                      'DES2359+0043',
                      'DES0002+0043',
                      'DES2354+0001',
                      'DES2356+0001',
                      'DES2359+0001',
                      'DES0002+0001'] 

                     

    # Now more generaly for a given TAGNAME
    try:
        TAGNAME = sys.argv[1]
    except:
        prog = os.path.basename(__file__)
        usage = "ERROR: \n USAGE: %s <TAGNAME>\n Example: %s %s\n" % (prog,prog,'Y2T3_FINALCUT')
        sys.exit(usage)


    # If table already exist and want to keep adding then clobber=False
    clobber = False
    for tilename in tiles_RXJ2248+tiles_ElGordo:
        print "# Creating TAGS for %s" % tilename
        insertTAG(tilename,myTAG='%s_RAN_CCD' % tilename, tagname=TAGNAME, tablename='felipe.tags',clobber=clobber, SELECT_BY="CCDS")
        clobber = False
        insertTAG(tilename,myTAG='%s_RAN_EXP' % tilename, tagname=TAGNAME, tablename='felipe.tags',clobber=clobber, SELECT_BY="EXPOSURES")
            
    
