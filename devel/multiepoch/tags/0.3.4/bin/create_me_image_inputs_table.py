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
     	 image_red.FILENAME,
	 fai_red.COMPRESSION,
         fai_red.PATH,
	 image_bkg.FILENAME as FILENAME_BKG,
	 fai_bkg.PATH as PATH_BKG,
         fai_bkg.COMPRESSION as COMPRESSION_BKG,
	 image_seg.FILENAME as FILENAME_SEG,
         fai_seg.PATH as PATH_SEG,
         fai_seg.COMPRESSION as COMPRESSION_SEG,
         tag_table.PFW_ATTEMPT_ID,
         image_red.BAND,
         image_red.CCDNUM,
         image_red.EXPNUM,
         image_red.CROSSRA0,
         image_red.RACMIN,image_red.RACMAX,
         image_red.DECCMIN,image_red.DECCMAX,
	 image_red.RA_CENT,image_red.DEC_CENT,
         image_red.RAC1,  image_red.RAC2,  image_red.RAC3,  image_red.RAC4,
         image_red.DECC1, image_red.DECC2, image_red.DECC3, image_red.DECC4,
         (case when image_red.CROSSRA0='Y' THEN abs(image_red.RACMAX - (image_red.RACMIN-360)) ELSE abs(image_red.RACMAX - image_red.RACMIN) END) as RA_SIZE,
         abs(image_red.DECCMAX - image_red.DECCMIN) as DEC_SIZE
     FROM
         {proctag_table} tag_table, image image_red, image image_bkg, miscfile image_seg, file_archive_info fai_red, file_archive_info fai_bkg, file_archive_info fai_seg
     WHERE
         fai_red.FILENAME  = image_red.FILENAME AND
         fai_bkg.FILENAME  = image_bkg.FILENAME AND	
         fai_seg.FILENAME  = image_seg.FILENAME AND	
	 image_red.PFW_ATTEMPT_ID = tag_table.PFW_ATTEMPT_ID AND
	 image_bkg.PFW_ATTEMPT_ID = tag_table.PFW_ATTEMPT_ID AND	
	 image_seg.PFW_ATTEMPT_ID = tag_table.PFW_ATTEMPT_ID AND		
         image_red.FILETYPE  = '{filetype}' AND
	 image_bkg.FILETYPE  = 'red_bkg' AND
	 image_seg.FILETYPE  = 'red_segmap' AND
	 image_red.CCDNUM = image_bkg.CCDNUM AND
	 image_bkg.CCDNUM = image_seg.CCDNUM AND
         tag_table.TAG = '{tagname}'
"""

def create_table_from_query(tagname,tagname_table=None,db_section='db-destest',filetype='red_immask',tablename_root='me_images',proctag_table='ops_proctag',clobber=False,verb=False):

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
                        help="Optional Name of the TAGNAME for the table naming, defaul=TAGNAME")
    parser.add_argument("--proctag_table", action="store",default='PROCTAG',
                        help="Change the proctag table to use (OPS_PROCTAG or PROCTAG)")
    parser.add_argument("--db_section", action="store", default='db-destest',choices=['db-desoper','db-destest'],
                        help="DB Section to query")
    parser.add_argument("--clobber", action="store_true",default=False,
                        help="Clobber existing table?")
    args = parser.parse_args()

    t0 = time.time()
    create_table_from_query(args.tagname,tagname_table=args.tagname_table,db_section=args.db_section,proctag_table=args.proctag_table,clobber=args.clobber,verb=True)
    print "Table Create time: %s" % elapsed_time(t0)
    
