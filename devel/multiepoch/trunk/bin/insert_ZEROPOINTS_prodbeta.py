#!/usr/bin/env python

import os,sys
import time
# DESDM Modules
import despydb


"""
A temporary ans simple script to copy the matched ZEROPOINT.MAG_ZERO
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
                from image where filename like '%s' and filetype='red'""" % regexp
    print "# Will query \n%s\n" % query

    print "# Will find expnames for filenames in reqnum=%s, attnum=%s" % (reqnum,attnum)

    dbh = despydb.desdbi.DesDbi(section=section)
    cur = dbh.cursor()
    cur.execute(query)
    (filenames,exposurenames) = zip(*cur.fetchall())
    cur.close()

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
    grant = "grant select on %s to des_reader" % tablename.split(".")[1]
    print "# Granting permission: %s\n" % grant
    cur.execute(grant)
    dbh.commit()
    cur.close()
    return


def insertZEROPOINTS(tablename,section='db-destest',clobber=False):
        
    """
    INSERT the ZEROPOINTS to a tablename from old Schema in db-desoper
    """
    

    # Get the filenames and exposurenames
    filenames, exposurenames = findEXPOSURES(reqnum=1007,attnum=5,section=section)
    Nexp = len(exposurenames)

    # Make sure that the ZEROPOINT table exists
    createZEROPOINTtable(tablename,section=section,clobber=clobber)


    dbh_desoper = despydb.desdbi.DesDbi(section="db-desoper")
    cur_desoper = dbh_desoper.cursor()
    
    dbh = despydb.desdbi.DesDbi(section=section)
    cur = dbh.cursor()

    # To find the runs from SVA1_FINALCUT for el Gordo and RXJ
    # select RUN from  RUNTAG  where runtag.RUN like '%20121207%' and runtag.tag='SVA1_FINALCUT';
    # select RUN from  RUNTAG  where runtag.RUN like '%20121124%' and runtag.tag='SVA1_FINALCUT';

    # Template query to find the zeropoints
    query = """SELECT distinct zeropoint.IMAGEID, image.IMAGENAME, zeropoint.MAG_ZERO
    from IMAGE, ZEROPOINT, RUNTAG
    where
    image.IMAGETYPE = 'red' and
    image.ID        = zeropoint.IMAGEID and
    image.RUN       = runtag.RUN and
    runtag.tag      = 'SVA1_FINALCUT' and
    image.run in ('20130717213138_20121124','20130712064117_20121207') and
    image.IMAGENAME='%s' """ 

    # Cols from the new table
    cols = "FILENAME,IMAGENAME,MAG_ZERO,IMAGEID"
    
    for k in range(Nexp):
        cur_desoper.execute(query % exposurenames[k])

        items =     dbh = despydb.desdbi.DesDbi(section=section)
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
    grant = "grant select on %s to des_reader" % tablename.split(".")[1]
    print "# Granting permission: %s\n" % grant
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
    
    query = """SELECT distinct zeropoint.IMAGEID, image.IMAGENAME, zeropoint.MAG_ZERO
    from IMAGE, ZEROPOINT, RUNTAG
    where
    image.IMAGETYPE = 'red' and
    image.ID        = zeropoint.IMAGEID and
    image.RUN       = runtag.RUN and
    runtag.tag      = 'SVA1_FINALCUT' and
    image.run in ('20130717213138_20121124','20130712064117_20121207') and
    image.IMAGENAME='%s' """ 
    
    cols = "FILENAME,IMAGENAME,MAG_ZERO,IMAGEID"
    
    for k in range(Nexp):

        cur_desoper.execute(query % exposurenames[k])
        items = cur_desoper.fetchone()

        # Check if returns a value
        if items is None:
            continue
        
        (imageid, imagename, mag_zero) = items

        sys.stdout.write("\r# Inserting MAG_ZERO for %s (%s/%s)" % (exposurenames[k],k+1,Nexp))
        sys.stdout.flush()

        # Now we insert
        values = (filenames[k], exposurenames[k], mag_zero, imageid)
        insert_cmd = "INSERT INTO %s (%s) VALUES %s" % (tablename,cols,values)
        cur.execute(insert_cmd)

    cur.close()
    dbh.commit()
    print "# Done ...\n"
    return


if __name__ == "__main__":

    # We only want the runs from Michael for El Gordo and RXJ
    # REQNUM     ATTNUM
    # ---------- ----------
    #  1007	    5
    #  1155	    1
    # On the new destest
    #
    # REQNUM     ATTNUM
    # ---------- ----------
    # 1413       02
    # 1417       02
    # 1417       03

    insertZEROPOINTS(tablename='felipe.extraZEROPOINT',reqnum=1413,attnum=2,clobber=True)
    insertZEROPOINTS(tablename='felipe.extraZEROPOINT',reqnum=1417,attnum=2,clobber=False)# no-clobber table
    insertZEROPOINTS(tablename='felipe.extraZEROPOINT',reqnum=1417,attnum=3,clobber=False)# no-clobber table
