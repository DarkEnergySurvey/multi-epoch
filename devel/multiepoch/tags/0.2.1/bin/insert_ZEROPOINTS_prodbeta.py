#!/usr/bin/env python

import os,sys
import time
import despydb
import despyastro

"""
A temporary and simple script to copy the matched ZEROPOINT.MAG_ZERO
values from old runs to new runs when do not exits in ther ZEROPOINT
table into the db-destest database

Felipe Menanteau, Nov 2014.

"""

def findEXPOSURES(reqnum,attnum,section='db-destest',exec_name='immask'):

    """
    Find the original names (DECamXXXXXXX_YY.fits) for a pair or reqnum, attnum
    Returns a set of filenames and exposurename
    """

    regexp = "%" + "r%dp%02d_%s" % (reqnum, attnum, exec_name) + "%"
    query = """ select filename, 'DECam_'||to_char(expnum,'FM00000000')||'_'||to_char(ccdnum,'FM00')||'.fits'
                from image where filename like '%s' and filetype='red_immask'""" % regexp
    print "# Will query \n%s\n" % query

    print "# Will find expnames for filenames in reqnum=%s, attnum=%s" % (reqnum,attnum)

    dbh = despydb.desdbi.DesDbi(section=section)
    cur = dbh.cursor()
    cur.execute(query)
    (filenames,exposurenames) = zip(*cur.fetchall())
    cur.close()

    print "# Found %s filename for %s exposurenames" %  (len(filenames),len(exposurenames))
    return filenames,exposurenames


def findZEROPOINTS(filenames, exposurenames, section='db-desoper'):

    """ Find the zero points in the old Schema for SVA1_FINALCUL"""

    dbh = despydb.desdbi.DesDbi(section=section)
    cur = dbh.cursor()

    # Runs '20130717213138_20121124' and '20130712064117_20121207' are
    # the original ones with the 'SVA1_FINALCUT' tag
    # To find the runs from SVA1_FINALCUT for el Gordo and RXJ
    # select RUN from  RUNTAG  where runtag.RUN like '%20121207%' and runtag.tag='SVA1_FINALCUT';
    # select RUN from  RUNTAG  where runtag.RUN like '%20121124%' and runtag.tag='SVA1_FINALCUT';

    query = """SELECT distinct zeropoint.IMAGEID, image.IMAGENAME, zeropoint.MAG_ZERO
    from IMAGE, ZEROPOINT, RUNTAG
    where
    image.IMAGETYPE = 'red' and
    image.ID        = zeropoint.IMAGEID and
    image.RUN       = runtag.RUN and
    runtag.tag      = 'SVA1_FINALCUT' and
    image.run in ('20130717213138_20121124','20130712064117_20121207') and
    image.IMAGENAME='%s' """ 
    
    for k in range( len(exposurenames)):
        cur.execute(query % exposurenames[k])
        (id, imagename, mag_zero) = cur.fetchone()
        print exposurenames[k],

    return

def checkTABLENAMEexists(tablename,section):

    """
    Check if exists. Tablename has to be a full owner.table_name format"""

    print "# Checking if %s exists" % tablename
    
    # Make sure is all upper case
    tablename = tablename.upper()

    query = """
    select count (*) from all_tables where owner||'.'||table_name='%s'""" % tablename

    dbh = despydb.desdbi.DesDbi(section=section)
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


def createZEROPOINTtable(tablename,section,clobber=False):
    
    dbh = despydb.desdbi.DesDbi(section=section)
    cur = dbh.cursor()
    # make sure we delete before we created -- if it exist only
    drop = "drop   table %s purge" % tablename
    
    create = """
    create table %s (
    FILENAME                VARCHAR2(100),
    IMAGENAME               VARCHAR2(100),
    MAG_ZERO                NUMBER(22,8),
    IMAGEID                 VARCHAR2(100),
    constraint %s PRIMARY KEY (FILENAME)
    )
    """ % (tablename,tablename.split(".")[1])

    # -- Add description of columns
    comments ="""comment on column %s.IMAGEID     is 'ID from IMAGE.ID'
    comment on column %s.FILENAME    is 'FILENAME from IMAGE.FILENAME'
    comment on column %s.IMAGENAME   is 'IMAGENAME from IMAGE.IMAGENAME old Schema'
    comment on column %s.MAG_ZERO    is 'MAG_ZERO in counts/s' """

    # Check if table exists
    table_exist = checkTABLENAMEexists(tablename,section)

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

def insertZEROPOINTS(tablename,reqnum,attnum,section='db-destest',clobber=False):
        
    """
    INSERT the ZEROPOINTS to a tablename
    """
    

    # Get the filenames and exposurenames
    filenames, exposurenames = findEXPOSURES(reqnum=reqnum,attnum=attnum,section=section)
    Nexp = len(exposurenames)

    # Make sure that the ZEROPOINT table exists
    createZEROPOINTtable(tablename,section=section,clobber=clobber)

    dbh_desoper = despydb.desdbi.DesDbi(section="db-desoper")
    cur_desoper = dbh_desoper.cursor()
    
    dbh = despydb.desdbi.DesDbi(section=section)
    cur = dbh.cursor()
    
    query = """
    with myimage as (select id, filename 
    from des_admin.location, des_admin.RUNTAG 
      where runtag.tag in 
      ('SVA1_FINALCUT','Y1A1_FINALCUT') and 
       location.RUN=runtag.RUN and 
       location.filename='%s' and 
       location.filetype='red') 
       select distinct zeropoint.IMAGEID, myimage.fileNAME, zeropoint.MAG_ZERO from myimage, des_admin.ZEROPOINT where zeropoint.imageid=myimage.id
     """

    # OLD QUERY
    #"""SELECT distinct zeropoint.IMAGEID, image.IMAGENAME, zeropoint.MAG_ZERO
    #from des_admin.IMAGE image, des_admin.ZEROPOINT zeropoint
    #where
    #image.IMAGETYPE = 'red' and
    #image.ID        = zeropoint.IMAGEID and
    #image.run in
    #   (select distinct image.run from des_admin.IMAGE, des_admin.RUNTAG where (runtag.tag in ('SVA1_FINALCUT','Y1A1_FINALCUT') and image.RUN=runtag.RUN)) and
    #image.IMAGENAME='%s' """ 
    
    cols = "FILENAME,IMAGENAME,MAG_ZERO,IMAGEID"
    
    for k in range(Nexp):

        cur_desoper.execute(query % exposurenames[k])
        items = cur_desoper.fetchone()

        # Check if returns a value
        if items is None:
            print "WARNING: Found no ZEROPOINT for: %s\n" % filenames[k]
            continue
        
        (imageid, imagename, mag_zero) = items

        sys.stdout.write("# Inserting MAG_ZERO for %s (%s/%s)\n" % (exposurenames[k],k+1,Nexp))
        # Now we insert
        values = (filenames[k], exposurenames[k], mag_zero, imageid)
        insert_cmd = "INSERT INTO %s (%s) VALUES %s" % (tablename,cols,values)
        cur.execute(insert_cmd)

    cur.close()
    dbh.commit()
    print "# Done ...\n"
    return



def get_REQNUM_ATTNUM(tagname,section='db-destest',clobber=False):

    # To find the REQNUM and ATTNUM:
    # select unique REQNUM, ATTNUM  from OPS_PROCTAG where TAG='Y2T3_FINALCUT';
    QUERY_REQNUM_ATTNUN = "select unique REQNUM, ATTNUM  from OPS_PROCTAG where TAG='{tagname}'"
    dbh = despydb.desdbi.DesDbi(section=section)
    query = QUERY_REQNUM_ATTNUN.format(tagname=tagname)
    rec = despyastro.query2rec(query,dbh,verb=True)
    return rec

if __name__ == "__main__":

    # Now more generaly for a given TAG
    try:
        TAGNAME = sys.argv[1]
    except:
        prog = os.path.basename(__file__)
        usage = "ERROR: \n USAGE: %s <TAGNAME>\n Example: %s %s\n" % (prog,prog,'Y2T3_FINALCUT')
        sys.exit(usage)
    # Get The corresponding REQNUM/ATTNUM for a fiven TAG
    runs = get_REQNUM_ATTNUM(tagname=TAGNAME)

    for run in runs:
        clobber = False
        #clobber = True
        insertZEROPOINTS(tablename='felipe.extraZEROPOINT',reqnum=run['REQNUM'],attnum=run['ATTNUM'],clobber=clobber)
