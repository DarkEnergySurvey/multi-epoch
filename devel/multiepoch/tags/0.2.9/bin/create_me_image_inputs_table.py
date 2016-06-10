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

CREATE_INPUTS = """
create table {tablename_root}_{tagname_table} as
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

def create_table_from_query(tagname,tagname_table=None,db_section='db-destest',filetype='red_immask',tablename_root='me_images',clobber=False,verb=False):

    # Get the handle
    dbh = desdbi.DesDbi(section=db_section)
    cur = dbh.cursor()

    # Fall back to default TAGNAME
    if not tagname_table:
        tagname_table = tagname
    
    # Check if tablename exists
    tablename = "%s_%s" % (tablename_root,tagname_table)
    table_exists = mutils.checkTABLENAMEexists(tablename,dbh=dbh,verb=verb)
    if table_exists and clobber:
        print "# Dropping table: %s" % tablename
        drop = "drop table %s purge" % tablename
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
        mutils.grant_read_permission(tablename,dbh)
    else:
        print "# WARNING:\n#\tCannot create table:\n#\t%s,\n#\tuse clobber=True" % tablename

    return 

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Create table with Multi-epoch inputs from coadds and cataloguing")
    parser.add_argument("tagname", action="store",default=None,
                        help="Name of the TAGNAME used to select inputs")
    parser.add_argument("--tagname_table", action="store",default=None,
                        help="Optiona Name of the TAGNAME for the table naming, defaul=TAGNAME")
    parser.add_argument("--db_section", action="store", default='db-destest',choices=['db-desoper','db-destest'],
                        help="DB Section to query")
    parser.add_argument("--clobber", action="store_true",default=False,
                        help="Clobber existing table?")
    args = parser.parse_args()

    t0 = time.time()
    create_table_from_query(args.tagname,tagname_table=args.tagname_table,db_section=args.db_section,clobber=args.clobber,verb=True)
    print "Table Create time: %s" % elapsed_time(t0)
    
