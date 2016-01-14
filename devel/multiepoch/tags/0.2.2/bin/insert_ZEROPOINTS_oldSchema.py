#!/usr/bin/env python

import os,sys
import time
# DESDM Modules
import despydb

"""
A temporary ans simple script to copy the matched ZEROPOINT.MAG_ZERO
values from old runs to new runs when do not exits in ther ZEROPOINT
table.

Felipe Menanteau, Sept 2014.

"""


def checkTABLENAMEexists(tablename):

    """
    Check if exists. Tablename has to be a full owner.table_name format"""

    print "# Checking if %s exists" % tablename
    
    # Make sure is all upper case
    tablename = tablename.upper()

    query = """
    select count (*) from all_tables where owner||'.'||table_name='%s'""" % tablename

    dbh = despydb.desdbi.DesDbi(section="db-desoper")
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


def createZEROPOINTtable(tablename,clobber=False):

    dbh = despydb.desdbi.DesDbi(section="db-desoper")
    cur = dbh.cursor()
    
    # make sure we delete before we created -- if it exist only
    drop = "drop   table %s purge" % tablename
    
    create = """
    create table %s (
    IMAGEID                 VARCHAR2(100),
    IMAGENAME               VARCHAR2(100),
    MAG_ZERO                NUMBER(22,8),
    PARENTID                NUMBER(22,10),
    constraint %s PRIMARY KEY (IMAGEID)
    )
    """ % (tablename,tablename.split(".")[1])

    # -- Add description of columns
    comments ="""comment on column %s.IMAGEID     is 'ID from IMAGE.ID'
    comment on column %s.IMAGENAME   is 'IMAGENAME from IMAGE.IMAGENAME'
    comment on column %s.MAG_ZERO    is 'MAG_ZERO in counts/s'
    comment on column %s.PARENTID    is 'ID from ZEROPOINT.ID' """ 

    # Check if table exists
    table_exist = checkTABLENAMEexists(tablename)

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


def insertZEROPOINTS(tablename,clobber=False):
        
    """
    INSERT the ZEROPOINTS to tablename
    """
    
    # First we make sure that the ZEROPOINT table exists
    createZEROPOINTtable(tablename,clobber=clobber)
    
    query = """SELECT distinct zeropoint.IMAGEID, image.IMAGENAME, zeropoint.MAG_ZERO
                 from IMAGE, ZEROPOINT, IMAGE imagezp, RUNTAG
                 where image.run in ('20140421093250_20121207','20140421090850_20121124') and
                       image.IMAGETYPE   = 'red' and
                       imagezp.ID        = zeropoint.IMAGEID and 
                       imagezp.IMAGENAME = image.IMAGENAME and 
                       imagezp.RUN       = runtag.RUN and 
                       runtag.tag        = 'SVA1_FINALCUT'"""

    print "# Will query:\n %s\n" % query

    dbh = despydb.desdbi.DesDbi(section="db-desoper")
    cur = dbh.cursor()
    #qdict = despyastro.query2dict_of_columns(query,dbhandle=dbh)
    qdict = despyastro.genutil.query2dict_of_columns(query,dbhandle=dbh,array=True)
    N = len(qdict['MAG_ZERO'])
    print "# Found %s matches with query" % N
    
    cols = ",".join(qdict.keys())
    for k in range(N):

        sys.stdout.write("\r# Inserting MAG_ZERO for %s (%s/%s)" % (qdict['IMAGENAME'][k],k+1,N))
        sys.stdout.flush()

        values = []
        for key in qdict.keys():
            values.append(qdict[key][k])
        insert_cmd = "INSERT INTO %s (%s) VALUES %s" % (tablename,cols,tuple(values))
        cur.execute(insert_cmd)

    cur.close()
    dbh.commit()
    print "# Done ...\n"
    return

if __name__ == "__main__":

    insertZEROPOINTS(tablename='felipe.extraZEROPOINT',clobber=True)
    
