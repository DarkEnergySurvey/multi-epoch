#!/usr/bin/env python

import os,sys
import time
import numpy
import matplotlib.pyplot as plt
from matplotlib.patches import Ellipse, Polygon
from pyfits import getheader

# DESDM Modules
import despymisc
import despydb
import despyastro
from despyastro import astrometry
from despyastro import wcsutil
# -----------------------

class DEScoords:
    
    """
    A Class to handle coordinate transformation in DESDM process DECam
    images using the new database schema (i.e. db-destest connection)

    Author:
      Felipe Menanteau, NCSA, April 2014.

    """

    def __init__ (self,section = "db-destest",verbose=False):

        # Setup desar connections to both DB
        self.section = section 
        self.verbose = verbose
        
        # Make the DB connections
        self.dbh = despydb.desdbi.DesDbi(section=self.section)
            

    def computeCCDcorners_filename(self,filename):

        """
        Get all elements from database by creating a 'fake' header to
        get wcs information.
        The call returns:
                rac1,decc1,rac2,decc2,rac3,decc3,rac4,decc4
                as two arrays ras,decs
        """
        
        query = """ select * from IMAGE where image.filename='%s'""" % filename
        cur = self.dbh.cursor()
        cur.execute(query)
        desc = [d[0].lower() for d in cur.description] # keep lowecase for consistency with wcsutil
        line = cur.fetchone()
        cur.close()
        
        # Make the header
        header = dict(zip(desc, line))
        nx = header['naxis1']
        ny = header['naxis2']
        
        wcs = wcsutil.WCS(header)
        self.RAC1,self.DECC1 = wcs.image2sky(1 , 1 )
        self.RAC2,self.DECC2 = wcs.image2sky(nx, 1 )
        self.RAC3,self.DECC3 = wcs.image2sky(nx, ny)
        self.RAC4,self.DECC4 = wcs.image2sky(1 , ny)
        self.RA , self.DEC   = wcs.image2sky(nx/2.0, ny/2.0)
        
        ras  = numpy.array([self.RAC1, self.RAC2, self.RAC3, self.RAC4])
        decs = numpy.array([self.DECC1,self.DECC2,self.DECC3,self.DECC4])
        
        return ras,decs

    def updateCCDcorners_filename(self,filename):
        
        """
        Compute the corners for a filename and update the new values
        them into the IMAGE database.  We are doing this only this
        time in an adhoc way, in the future this function won't be
        necessary.

        IMPORTANT:
        To check that corners are OK, use compare_corners_NewFramework
        inside qatoolkit
        """

        self.computeCCDcorners_filename(filename)

        cols   = ['RA', 'RAC1', 'RAC2', 'RAC3', 'RAC4',
                  'DEC','DECC1','DECC2','DECC3','DECC4']
        values = [self.RA,  self.RAC1,  self.RAC2,  self.RAC3,  self.RAC4,
                  self.DEC, self.DECC1, self.DECC2, self.DECC3, self.DECC4]

        # Loop over items and format
        s = ''
        for item in dict(zip(cols,values)).items():
            s = s + "%s=%s," % item
        update_query = "update IMAGE set %s where filename='%s' " % (s[:-1],filename)
        cur = self.dbh.cursor()
        cur.execute(update_query)
        self.dbh.commit()
        cur.close()
        return

    def getCCDcorners(self,filename):

        keys   = ['RA', 'RAC1', 'RAC2', 'RAC3', 'RAC4',
                  'DEC','DECC1','DECC2','DECC3','DECC4']
        query = """ select %s from IMAGE where image.filename='%s' """ % (','.join(keys),filename)
        if self.verbose: print "# Will query:\n %s\n" % query
        cur = self.dbh.cursor()
        cur.execute(query)
        (ra,rac1,rac2,rac3,rac4,dec,decc1,decc2,decc3,decc4) = cur.fetchone()
        cur.close()
        ras  = numpy.array([rac1, rac2, rac3, rac4])
        decs = numpy.array([decc1,decc2,decc3,decc4])
        return ras,decs

    def updateCCDcorners_reqnum(self,reqnum,exec_name='maskcosmics'):

        """
        Update the CCD corners for a given reqnum, it used the
        function: update_CCDcorners_filename(filename)
        """

        query = """select image.filename from image, wgb
        where image.filename=wgb.filename and reqnum=%s and exec_name='%s'
        order by ccdnum,wrapnum """ % (reqnum,exec_name)
        print "# Will query:\n %s\n" % query
        
        cur = self.dbh.cursor()
        cur.execute(query)
        # Get them all at once
        list_of_tuples = cur.fetchall()
        cur.close()
        filenames, = zip(*list_of_tuples)
        print "# Found %s files with reqnum=%s and exec_name=%s" % (len(filenames),reqnum,exec_name)
        print "# Will attemp to update CCD corners for all"
        for i in range(len(filenames)):
            sys.stdout.write("\r# Updating CCD corners for %s (%s/%s)" % (filenames[i],i+1,len(filenames)))
            sys.stdout.flush()
            self.updateCCDcorners_filename(filenames[i])
        print "# Done ...\n"
        return



class DEScoordsOLD:

    """
    Set of functions to write and into the old database schema for
    coadd work. Compute new CCD corners and insert the into the new
    table

    A Class to handle coordinate transformation in DESDM process DECam
    images using the old DB schema

    Author:
      Felipe Menanteau, NCSA, April 2014.
    """

    def __init__ (self,verbose=False):

        self.verbose = verbose
        self.dbh = despydb.desdbi.DesDbi(section="db-desoper")


    def insertCCDcorners_query(self,query,tablename='felipe.imagecorners_test',clobber=False):
        
        """
        INSERT the CCD corners for a given query, it uses the
        function: insertCCDcornersID(ID,IMAGENAME) to do the work.
        """

        # Make sure is all upper case
        #tablename = tablename.upper()

        t0 = time.time()
        # First we make sure that the IMAGECORNERS table exists
        self.createIMAGECORNERStable(tablename=tablename,clobber=clobber)

        query = "select ID, IMAGENAME from image %s " % query
        if self.verbose: print "# Will query:\n %s\n" % query

        try:
            cur = self.dbh.cursor()
        except:
            self.dbh =  despydb.desdbi.DesDbi(section="db-desoper")
            cur = self.dbh.cursor()
            
        cur.execute(query)
        # Get them all at once
        list_of_tuples = cur.fetchall()
        cur.close()
        ids, imagenames = zip(*list_of_tuples)
        print "# Found %s files with query" % len(ids)
        print "# Will attempt to compute new CCD corners for all..."
        for i in range(len(ids)):
            sys.stdout.write("\r# Inserting CCD corners for %s (%s/%s)" % (ids[i],i+1,len(ids)))
            sys.stdout.flush()
            self.insertCCDcornersID(ids[i],imagenames[i],tablename,commit=False)

        # Commit after we are done
        self.dbh.commit()
        print "#\n# Time:%s" % despymisc.miscutils.elapsed_time(t0)
        return
    
    def insertCCDcornersID(self,ID, IMAGENAME, TABLENAME, commit=False):
        
        """
        Computes the corners for an ID and insert them into the
        database.  We are doing this only this time in an adhoc way

        # Make sure the table as been created
        SQL> @felipe.imagecorners.sql
        """

        # Get the corners self.RA,RA[1,4], etc
        self.computeCCDcornersID(ID)

        columns = ['ID','IMAGENAME',
                   'RA', 'RAC1', 'RAC2', 'RAC3', 'RAC4',
                   'DEC','DECC1','DECC2','DECC3','DECC4']
        
        values = (ID,IMAGENAME,
                  self.RA,  self.RAC1,  self.RAC2,  self.RAC3,  self.RAC4,
                  self.DEC, self.DECC1, self.DECC2, self.DECC3, self.DECC4)
        
        cols = ",".join(columns)
        insert_cmd = "INSERT INTO %s (%s) VALUES %s" % (TABLENAME,cols,values)

        #sys.stdout.write("\r# Inserting %s to %s" % (ID,TABLENAME))
        #sys.stdout.flush()

        cur = self.dbh.cursor()
        cur.execute(insert_cmd)
        cur.close()
        if commit: self.dbh.commit()
        return


    def computeCCDcornersID(self,ID):

        """
        Gets all elements from database by creating a 'fake' header to
        get wcs information.
        The call returns:
                rac1,decc1,rac2,decc2,rac3,decc3,rac4,decc4
                as two arrays ras,decs

        Uses the 'old' schema for the DB that relies in the ID as the
        unique identifier.
        """
        
        query = " select * from image where image.id='%s'" % ID
        cur = self.dbh.cursor()
        cur.execute(query)
        desc = [d[0].lower() for d in cur.description] # keep lowecase for consistency with wcsutil
        line = cur.fetchone()
        cur.close()
        
        # Make the header
        header = dict(zip(desc, line))
        nx = header['naxis1']
        ny = header['naxis2']
        
        wcs = wcsutil.WCS(header)
        self.RAC1,self.DECC1 = wcs.image2sky(1 , 1 )
        self.RAC2,self.DECC2 = wcs.image2sky(nx, 1 )
        self.RAC3,self.DECC3 = wcs.image2sky(nx, ny)
        self.RAC4,self.DECC4 = wcs.image2sky(1 , ny)
        self.RA , self.DEC   = wcs.image2sky(nx/2.0, ny/2.0)
        
        ras  = numpy.array([self.RAC1, self.RAC2, self.RAC3, self.RAC4])
        decs = numpy.array([self.DECC1,self.DECC2,self.DECC3,self.DECC4])
        
        return ras,decs

    def getCornersTILENAME(self,tilename):
        
        """ Get the Corners for a DES TILENAME as defined in the old database schema
        
          RA     RA  of center of focal plane or CCD (deg) 
          DEC    DEC of center of focal plane or CCD (deg) 
        
          RALL   RA Lower-left
          RALR   RA Lower-right
          RAUL   RA Upper-left
          RAUR   RA Upper-right
        
          DECLL  DEC Lower-left
          DECLR  DEC Lower-right
          DECUR  DEC Upper-right
          DECUL  DEC Upper-left
        
                 UL                             UR
                  +------------------------------+
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  |                              |
                  +------------------------------+
                 LL                              LR
        """

        query = " select RALL,DECLL, RALR, DECLR, RAUR, DECUR, RAUL, DECUL from COADDTILE where TILENAME='%s'" % tilename
        if self.verbose: print query
        cur = self.dbh.cursor()
        cur.execute(query)
        RALL, DECLL, RALR, DECLR, RAUR, DECUR, RAUL, DECUL  = cur.fetchone()
        ras   = numpy.array([RALL,  RALR,  RAUR,  RAUL])
        decs  = numpy.array([DECLL, DECLR, DECUR, DECUL])
        return ras,decs
        

    def getCCDsfromTILENAME(self,tilename,query=None):

        """ Get all of the DECam CCDs that fall inside a TILENAME from a given DB query in the old Schema"""

        # First we need to figure out the cornes of the tilename

        ras, decs = self.getCornersTILENAME(tilename)
        
        # understanble names
        [RALL,  RALR,  RAUR,  RAUL]  = list(ras)
        [DECLL, DECLR, DECUR, DECUL] = list(decs)

        print tilename, RALL,  RALR,  RAUR,  RAUL
        print tilename, DECLL, DECLR, DECUR, DECUL

        return


    def checkTABLENAMEexists(self,tablename):

        """tablename has to be a full owner.table_name format"""

        print "# Checking if %s exists" % tablename

        # Make sure is all upper case
        tablename = tablename.upper()

        query = """
        select count (*) from all_tables where owner||'.'||table_name='%s'""" % tablename
        cur = self.dbh.cursor()
        cur.execute(query)
        count = cur.fetchone()[0]
        cur.close()
        
        if count >= 1:
            table_exists = True
        else:
            table_exists = False
        print "# %s exists: %s " % (tablename,table_exists)
        return table_exists


    def createIMAGECORNERStable(self,tablename,clobber=False):
        
        cur = self.dbh.cursor()

        # make sure we delete before we created -- if it exist only
        drop = "drop   table %s purge" % tablename

        create = """
        create table %s (
        ID                      NUMBER(22,10),
        IMAGENAME               VARCHAR2(100),
        RA                      NUMBER(22,8),
        DEC                     NUMBER(22,8),
        RAC1                    NUMBER(22,8),
        RAC2                    NUMBER(22,8),
        RAC3                    NUMBER(22,8),
        RAC4                    NUMBER(22,8),
        DECC1                   NUMBER(22,8),
        DECC2                   NUMBER(22,8),
        DECC3                   NUMBER(22,8),
        DECC4                   NUMBER(22,8),
        constraint %s PRIMARY KEY (ID)
        )
        """ % (tablename,tablename.split(".")[1])

        # -- Add description of columns
        comments ="""comment on column %s.DEC    is 'RA  center of DES tile (deg)' 
        comment on column %s.RA     is 'DEC center of DES file (deg)'
        comment on column %s.RAC1   is 'Corner 1 RA of DES tile (deg)'
        comment on column %s.RAC2   is 'Corner 2 RA of DES tile (deg)'
        comment on column %s.RAC3   is 'Corner 3 RA of DES tile (deg)'
        comment on column %s.RAC4   is 'Corner 4 RA of DES tile (deg)'
        comment on column %s.DECC1  is 'Corner 1 DEC of DES tile (deg)'
        comment on column %s.DECC2  is 'Corner 2 DEC of DES tile (deg)'
        comment on column %s.DECC3  is 'Corner 3 DEC of DES tile (deg)'
        comment on column %s.DECC4  is 'Corner 4 DEC of DES tile (deg)'"""

        # Check if table exists
        table_exist = self.checkTABLENAMEexists(tablename)

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
        self.dbh.commit()
        cur.close()
        return

# ################################################
# # TEST PLOTING ROUTINES -- to be removed
# #
# def plot_reqnum_query(reqnum=60, exec_name = 'maskcosmics'):
#
#     query = """select image.filename, image.ccdnum from image, wgb
#     where image.filename=wgb.filename and reqnum=%s and exec_name='%s'
#     order by ccdnum,wrapnum """ % (reqnum,exec_name)
#     print "# Will query:\n %s\n" % query
#
#     descoo = DEScoords()
#     cur = descoo.dbh.cursor()
#     cur.execute(query)
#     # Get them all at once
#     list_of_tuples = cur.fetchall()
#     cur.close()
#     filenames, ccdnum = zip(*list_of_tuples)
#     print "# Found %s files with reqnum=%s and exec_name=%s" % (len(filenames),reqnum,exec_name)
#
#     x1 = 25.
#     x2 = 60.
#     y1 = -1.0
#     y2 = +1.0
#     aspect = abs(y2-y1)/abs(x2-x1)
#     plt.figure(1,figsize = (15,15*aspect))
#     ax = plt.gca()
#     for filename in filenames:
#         print "Doing %s" % filename
#         ras, decs = descoo.getCCDcorners(filename)
#         plt.plot(ras,decs,'k-')
#         P = Polygon( zip(ras,decs), closed=True, fill=False, hatch='/')
#         ax.add_patch(P)
#
#     plt.xlim(x1,x2)
#     plt.ylim(y1,y2)
#     plt.show()
#     return
#
# #  Plot just one CCD by filename
# def plot_one_CCD(filename):
#     descoo = DEScoords()
#     ras, decs = descoo.getCCDcorners(filename)
#     x_range = abs(ras.min()  - ras.max())
#     y_range = abs(decs.min() - decs.max())
#     aspect = y_range/x_range
#     plt.figure(1,figsize = (8,8*aspect))
#     ax = plt.gca()
#     plt.plot(ras,decs,'k-')
#     P = Polygon( zip(ras,decs), closed=True, fill=False, hatch='/')
#     ax.add_patch(P)
#     plt.show()
#     return
#
#
# def plot_around_region(reqnum,exec_name):
#
#     descoo = DEScoords()
#
#     # Now select the tilenames in the same region
#     query = """ select TILENAME from COADDTILE where
#                 RALL > 28.5 and RALL < 30.5 and
#                 DECLL > -1.5 and DECLL < 1.0"""
#     print "# Will query:\n %s\n" % query
#     cur = descoo.dbh.cursor()
#     cur.execute(query)
#     list_of_tuples = cur.fetchall()
#     tilenames, = zip(*list_of_tuples)
#
#     print tilenames
#
#     query = """select image.filename, image.ccdnum from image, wgb
#                where image.filename=wgb.filename and
#                reqnum=%s and
#                exec_name='%s' and
#                image.RA  > 28 and image.RA < 31 and
#                image.DEC > -1.5 and image.DEC < 1.5
#                order by ccdnum,wrapnum """ % (reqnum,exec_name)
#     print "# Will query:\n %s\n" % query
#     cur = descoo.dbh.cursor()
#     cur.execute(query)
#     # Get them all at once
#     list_of_tuples = cur.fetchall()
#     cur.close()
#     filenames, ccdnum = zip(*list_of_tuples)
#     print "# Found %s files with reqnum=%s and exec_name=%s" % (len(filenames),reqnum,exec_name)
#
#     # Start pylab
#     plt.figure(1,figsize = (15,15))
#     ax = plt.gca()
#
#     # Plot the CCDs filenames
#     for filename in filenames:
#         print "Doing %s" % filename
#         ras, decs = descoo.getCCDcorners(filename)
#         plt.plot(ras,decs,'k:')
#         P = Polygon( zip(ras,decs), closed=True, fill=False, hatch='/')
#         ax.add_patch(P)
#
#     # Plot the selected tilenames
#     for tilename in tilenames:
#         print "Doing %s" % tilename
#         ras,decs = descoo.getCornersTILENAME(tilename)
#         print ras[0:2]
#         plt.text(ras[0]+0.1,decs[0]+0.1,tilename,color='red')
#         P = Polygon( zip(ras,decs), closed=True, fill=False, ec='red',ls='solid')
#         ax.add_patch(P)
#
#     plt.show()
#     return
#
#################################################


