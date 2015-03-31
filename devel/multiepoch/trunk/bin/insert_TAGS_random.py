#!/usr/bin/env python

import os,sys

import multiepoch.utils as utils
import multiepoch.querylibs as querylibs
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
SELECT_EXTRAS = "felipe.extraZEROPOINT.MAG_ZERO,"
FROM_EXTRAS   = "felipe.extraZEROPOINT"
AND_EXTRAS    = "felipe.extraZEROPOINT.FILENAME = image.FILENAME" 

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
    grant = "grant select on %s to des_reader" % tablename.split(".")[1]
    print "# Granting permission: %s\n" % grant
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


def insertTAG(tilename,myTAG,tablename='felipe.tags',clobber=False,db_section='db-destest',SELECT_BY="EXPOSURES",NEXP=4):

    options = {
        'tilename'   : tilename,
        'coaddtile_table' : "felipe.coaddtile_new",
        'archive_name'  : "prodbeta",
        'tagname'       : 'Y2T_FIRSTCUT',
        'exec_name'     : 'immask',
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
    tile_edges = (tileinfo['RACMIN'], tileinfo['RACMAX'],tileinfo['DECCMIN'],tileinfo['DECCMAX'])
    CCDS = querylibs.get_CCDS_from_db(dbh, tile_edges,**options)

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
            expnames_sel = numpy.random.choice(expnames,NEXP,replace=False)
            k = numpy.array( [fname[0:9] in expnames_sel for fname in filenames])
            filenames_sel = filenames[k==True]
        elif SELECT_BY == "CCDS":
            print "# Will select %s CCDS randomly and insert to %s" % (Nmedian[BAND],tablename)
            filenames_sel = numpy.random.choice(filenames,Nmedian[BAND],replace=False)
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

if __name__ == "__main__":

    insertTAG('DES2246-4457',myTAG='DES2246-4457_RAN_CCD', tablename='felipe.tags',clobber=True,  SELECT_BY="CCDS")
    insertTAG('DES2246-4457',myTAG='DES2246-4457_RAN_EXP', tablename='felipe.tags',clobber=False, SELECT_BY="EXPOSURES")
    
    
