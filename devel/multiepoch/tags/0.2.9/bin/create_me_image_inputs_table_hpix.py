#!/usr/bin/env python

"""
Create the table with the basic inputs files and meta-data for
multi-epoch COADDS for a given TAGNAME
"""

from despydb import desdbi
from despymisc.miscutils import elapsed_time
import multiepoch.utils as mutils
import time
import argparse
import despyastro

CREATE_INPUTS = """
create table {me_tablename} as
     SELECT 
         file_archive_info.FILENAME,
	 file_archive_info.COMPRESSION,
         file_archive_info.PATH,
         image.PFW_ATTEMPT_ID,
         ops_proctag.UNITNAME,
         image.BAND,
         image.CCDNUM,
         image.EXPNUM,
         image.CROSSRA0,
         image.RACMIN,image.RACMAX,
         image.DECCMIN,image.DECCMAX,
	 image.RA_CENT,image.DEC_CENT,
         image.RAC1,  image.RAC2,  image.RAC3,  image.RAC4,
         image.DECC1, image.DECC2, image.DECC3, image.DECC4,
         (case when image.CROSSRA0='Y' THEN abs(image.RACMAX - (image.RACMIN-360)) ELSE abs(image.RACMAX - image.RACMIN) END) as RA_SIZE,
         abs(image.DECCMAX - image.DECCMIN) as DEC_SIZE
     FROM
         ops_proctag, image, file_archive_info
     WHERE
         file_archive_info.FILENAME  = image.FILENAME AND
	 image.PFW_ATTEMPT_ID = ops_proctag.PFW_ATTEMPT_ID AND
         image.FILETYPE  = '{filetype}' AND
         ops_proctag.TAG = '{tagname}'
"""

def create_table_from_query(me_tablename,tagname,db_section='db-destest',filetype='red_immask',clobber=False,verb=False):

    # Get the handle
    dbh = desdbi.DesDbi(section=db_section)
    cur = dbh.cursor()

    table_exists = mutils.checkTABLENAMEexists(me_tablename,dbh=dbh,verb=verb)
    if table_exists and clobber:
        print "# Dropping table: %s" % me_tablename
        drop = "drop table %s purge" % me_tablename
        cur.execute(drop)

    # Create if not exist or clobber=True
    if not table_exists or clobber:
        # Format the create query
        kw = locals()
        create = CREATE_INPUTS.format(**kw)
        if verb:
            print "Will run:"
            print create
        cur.execute(create)
        cur.close()
        dbh.commit()
        # Grant permission
        mutils.grant_read_permission(me_tablename,dbh)
    else:
        print "# WARNING:\n#\tCannot create table:\n#\t%s,\n#\tuse clobber=True" % me_tablename

    return dbh


def create_hpix_from_table(me_tablename,hpix_tablename,dbh=None,NSIDE=32,NEST=True,db_section='db-destest'):

    import healpy as hp
    import numpy as np
    import numpy.lib.recfunctions as recfuncs 

    # Get a cursor and dbh
    if not dbh:
        dbh = desdbi.DesDbi(section=db_section)
    cur = dbh.cursor()

    # Get FILENAME, RA, DEC from tmp table
    QUERY = "select FILENAME, RA_CENT, DEC_CENT from {me_tablename}"
    rec = despyastro.query2rec(QUERY.format(me_tablename=me_tablename), dbhandle=dbh)

    # Compute the hpix values
    phi = rec['RA_CENT']/180.*np.pi
    theta = (90. - rec['DEC_CENT'])/180.*np.pi
    pixs = hp.ang2pix(NSIDE, theta, phi, nest=NEST)

    # make a record array of the pixs and merge it with the record array
    extra = np.rec.fromrecords(zip(pixs),names='HPIX_32') 
    rec = recfuncs.merge_arrays([rec,extra],flatten=True,asrecarray=True)

    # Drop if exists
    if mutils.checkTABLENAMEexists(hpix_tablename,dbh=dbh,verb=True):
        print "# Dropping table: %s" % hpix_tablename
        drop = "drop table %s purge" % hpix_tablename
        cur.execute(drop)

    # Create the table with pixels
    create = """
    create table %s (
    FILENAME VARCHAR2(100) NOT NULL,
    HPIX_32  NUMBER(8) NOT NULL,
    constraint %s_unique UNIQUE(FILENAME)
    )
    """ % (hpix_tablename,hpix_tablename.split(".")[1])
    cur.execute(create)
    #cur.close()
    dbh.commit()
    # Grant permission
    mutils.grant_read_permission(me_tablename,dbh)

    # Now we can insert and merge
    columns = ['FILENAME','HPIX_32']
    cvals = []
    for column in columns:
        cvals.append(':%s'%column)
    
    cols = ','.join(columns)
    vals = ','.join(cvals)
    qinsert = 'insert into %s (%s) values (%s)' % (hpix_tablename.upper(), cols, vals)
    cur.executemany(qinsert, zip(rec['FILENAME'],rec['HPIX_32']))
    dbh.commit()
    return

def merge_tables(newtable,table1,table2,clobber=False,dbh=None,db_section=None,verb=None):

    # Get a cursor and dbh
    if not dbh:
        dbh = desdbi.DesDbi(section=db_section)
    cur = dbh.cursor()

    # Make sure that the new table is not there
    if mutils.checkTABLENAMEexists(newtable,dbh=dbh,verb=verb) and clobber:
        print "# Dropping table: %s" % newtable
        drop = "drop table %s purge" % newtable
        cur.execute(drop)

    QEXE = "create table {newtable} as select {table1}.*, {table2}.HPIX_32 from {table1},{table2} where {table1}.FILENAME={table2}.FILENAME"
    print "# Merging tables %s and %s --> %s" % (table1,table2,newtable)
    cur.execute(QEXE.format(newtable=newtable,table1=table1,table2=table2))

    # Grant permission
    mutils.grant_read_permission(newtable,dbh)

    # Now we build the indexes for HPIX_32 and RA_CENT,DEC_CENT
    print "# Making indices for HPIX_32 and RA_CENT,DEC_CENT"
    qindx = "create index hpix32_idx on %s(HPIX_32)" % newtable
    cur.execute(qindx)
    qindx = "create index ra_cent_idx on %s(RA_CENT)" % newtable
    cur.execute(qindx)
    qindx = "create index dec_cent_idx on %s(DEC_CENT)" % newtable
    cur.execute(qindx)
    cur.close()
    return

def cleanup_tables(tablelist,dbh=None,db_section=None):

    # Get a cursor and dbh
    if not dbh:
        dbh = desdbi.DesDbi(section=db_section)
    cur = dbh.cursor()

    for tablename in tablelist:
        print "# Dropping table %s" % tablename
        qexe = 'drop table %s purge' % tablename
        cur.execute(qexe)
    cur.close()
    return

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Create table with Multi-epoch inputs from coadds and cataloguing")
    parser.add_argument("tagname", action="store",default=None,
                        help="Name of the TAGNAME used to select inputs")
    parser.add_argument("--tagname_table", action="store",default=None,
                        help="Optional Name of the TAGNAME for the table naming, defaul=TAGNAME")
    parser.add_argument("--tablename_root", action="store",default='me_images',
                        help="Optional Name of the output tablename root name, defaul=me_images")
    parser.add_argument("--db_section", action="store", default='db-destest',choices=['db-desoper','db-destest'],
                        help="DB Section to query")
    parser.add_argument("--clobber", action="store_true",default=False,
                        help="Clobber existing table?")
    args = parser.parse_args()

    if not args.tagname_table:
        args.tagname_table = args.tagname

    t0 = time.time()

    # The name of the final merged table name
    tablename = "%s_%s" % (args.tablename_root, args.tagname_table)
    me_tablename   = 'felipe.me_image_tmp'
    hpix_tablename = 'felipe.hpix32_tmp'

    dbh = create_table_from_query(me_tablename,args.tagname,db_section=args.db_section,clobber=args.clobber,verb=True)
    print "# Table Creation time: %s" % elapsed_time(t0)

    # Compute the healpix indices and insert them into a table
    create_hpix_from_table(me_tablename,hpix_tablename='felipe.hpix32_tmp',dbh=dbh,NSIDE=32,NEST=True)

    # Merge the two table into the name that we want
    merge_tables(tablename,table1=me_tablename,table2=hpix_tablename,dbh=dbh,clobber=args.clobber)

    # Clean up tables
    cleanup_tables([me_tablename,hpix_tablename],dbh)

    print "# Table %s is ready" % tablename
