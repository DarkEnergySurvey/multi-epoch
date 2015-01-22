#!/usr/bin/env python

import os,sys
import math
import time

# DESDM modules
import despydb
import despyastro
from despyastro import astrometry
from despyastro import wcsutil
import numpy

class DEStiling:
    
    """
    This is the improvement on the transaltion of gencoaddtile.tcl in
    order to reproduce the *same* centers as before, but with and
    accurate set of corners and unique corners definitions. It also
    introduces a new schema for the corners.
    
    Author:
      Felipe Menanteau, NCSA, April 2014.

    Modified into new Scheme

    """

    def __init__ (self, pixscale=0.263,
                  NAXIS1=10000,
                  NAXIS2=10000,
                  tileprefix='DES'):

        self.pixelscale = pixscale
        self.NAXIS1     = NAXIS1
        self.NAXIS2     = NAXIS2
        self.tileprefix = tileprefix


    def get_tilename(self):

        """Construct the name of the tile"""
        RA  = astrometry.dec2deg(self.ra_center/15. ,sep="",short='yes')
        DEC = astrometry.dec2deg(self.dec_center,    sep="",short='yes')
        if self.dec_center < 0.0:
            self.tilename = "%s%s%s" % (self.tileprefix,RA,DEC)
        else:
            self.tilename = "%s%s+%s" % (self.tileprefix,RA,DEC)
        return

    @staticmethod
    def get_overlap(dec):

        # OVERLAP!
        # one arcminute overlap, works everywhere but close to pole (dec < -75 deg)
        # Completely add-hoc definition!  It should be changed to a
        # desirable overlap. We would like overlap should be 1 arcminute ALWAYS
        # regardless of RA,DEC!!!  We could ajust the code so increase in ra,dec
        # are diferent and allows for such overlap, but that will change the centroids
        # We should have ra_overlap, dec_overlap
        overlap1  =  1/60.
        overlap2  =  2/60.
        overlap5  =  5/60.
        overlap10 = 10/60.

        if abs(dec) < 74:
            overlap = overlap1 # This is the one we want all the time
        elif abs(dec) < 82.5:
            overlap = overlap2
        elif abs(dec) < 87.6:
            overlap = overlap5
        else:
            overlap = overlap10
        # Make it visible
        return overlap

    def create_header(self):

        """ Defines in full the tile header as a dictionary, notice that only CRVAL[1,2] are changing"""

        self.header = {
            'NAXIS'   :  2,                      #/ Number of pixels along this axis
            'NAXIS1'  :  self.NAXIS1,            #/ Number of pixels along this axis
            'NAXIS2'  :  self.NAXIS2,            #/ Number of pixels along this axis
            'CTYPE1'  : 'RA---TAN',              #/ WCS projection type for this axis
            'CTYPE2'  : 'DEC--TAN',              #/ WCS projection type for this axis
            'CUNIT1'  : 'deg',                   #/ Axis unit
            'CUNIT2'  : 'deg',                   #/ Axis unit
            'CRVAL1'  :  self.ra_center,         #/ World coordinate on this axis
            'CRPIX1'  :  (self.NAXIS1+1)/2.0,    #/ Reference pixel on this axis
            'CD1_1'   :  -self.pixelscale/3600., #/ Linear projection matrix -- CD1_1 is negative
            'CD1_2'   :  0,                      #/ Linear projection matrix -- CD1_2 is zero, no rotation
            'CRVAL2'  :  self.dec_center,        #/ World coordinate on this axis
            'CRPIX2'  :  (self.NAXIS2+1)/2.0,    #/ Reference pixel on this axis
            'CD2_1'   :  0,                      #/ Linear projection matrix -- CD2_1 is zero, no rotation
            'CD2_2'   :  +self.pixelscale/3600.  #/ Linear projection matrix -- CD2_2 is positive
            }


        # Now for consistecy, add lower-case keys, to make it 'case-insensity' fake
        for k, v in self.header.items():
            self.header[k.lower()] = v

    def computeCornersTilename(self):
        
        """
          DES Tile's corner definition, similar to the CCD Image
          Corner Coordinates definitions for DECam
          see: https://desdb.cosmology.illinois.edu/confluence/display/PUB/CCD+Image+Corners+Coordinates


                                   x-axis 
               (RA4,DEC4)                             (RA3,DEC3)
               Corner 4 +---------------------------+ Corner 3
             (1,NAXIS2) |                           | (NAXIS1,NAXIS2)
                        |                           |
                        |                           |
                        |                           |
                        |                           |
                        |                           |   y-axis
                        |                           |
                        |                           |
                        |                           |
                        |                           |
                        |                           |
              (RA1,DEC1)|                           | (RA2,DEC2)            
               Corner 1 +---------------------------+ Corner 2
                  (1,1)                              (NAXIS1,1)

                                RA Grows (when CD1_1 is negative)
                                <------

        """
        header = self.header
        nx = header['NAXIS1']
        ny = header['NAXIS2']

        wcs = wcsutil.WCS(header)
        self.RAC1,self.DECC1 = wcs.image2sky(1 , 1 )
        self.RAC2,self.DECC2 = wcs.image2sky(nx, 1 )
        self.RAC3,self.DECC3 = wcs.image2sky(nx, ny)
        self.RAC4,self.DECC4 = wcs.image2sky(1 , ny)

        ras  = numpy.array([self.RAC1,  self.RAC2, self.RAC3,  self.RAC4])
        decs = numpy.array([self.DECC1, self.DECC2,self.DECC3, self.DECC4])

        if self.crossRAzero == 'Y':
            # Maybe we substract 360 instead?
            self.RACMIN = ras.max()
            self.RACMAX = ras.min()
        else:
            self.RACMIN = ras.min()
            self.RACMAX = ras.max()
            
        self.DECCMIN = decs.min()
        self.DECCMAX = decs.max()

        return
        

    def compareDB(self):

        """
        Compare with existing DB entries, alternatively, we could also
        compare with a csv file that was dumped previously:
        COADDTILE_from_desar_20140517.csv
        """
        
        cur = self.dbh.cursor()
        
        # ------------- DB Section ------------------------
        # Query the COADDTILE table to compare results
        query = """select TILENAME,
                          RA,DEC,
                          RALL,  RALR,  RAUL,   RAUR,  
                          DECLL, DECLR, DECUL,  DECUR,
                          URALL, URAUR, UDECLL, UDECUR 
                          from COADDTILE where TILENAME='%s'""" % self.tilename
        cur.execute(query)
        (TILENAME,
         RA,DEC,
         RALL,  RALR,  RAUL,   RAUR, 
         DECLL, DECLR, DECUL,  DECUR,
         URALL, URAUR, UDECLL, UDECUR) = cur.fetchone()
        # ------------- DB Section ------------------------

        # Check for variations in the centroid
        dRA  = abs(self.ra_center  - RA)
        dDEC = abs(self.dec_center - DEC)
        if dRA > 1e-3 or dDEC > 1e-3:
            print "RA,DEC problem for %s, %s %s" % (self.tilename, dRA, dDEC)
            
        # Check (U)pper and (L)ower RAs
        if URALL > 360: URALL = URALL - 360 # fix ura[u,l] > 360 to be consistent with new def
        if URAUR > 360: URAUR = URAUR - 360 # 
        dURAL = abs(URALL - self.ural)
        dURAU = abs(URAUR - self.urau)
        if dURAL > 1e-3 or dURAU > 1e-3:
            print "URA[L,U] problem for %s, %s %s" % (self.tilename, dURAL, dURAU)

        # Check (U)pper and (L)ower DECs
        dUDECL = abs(UDECLL-self.udecl)
        dUDECU = abs(UDECUR-self.udecu)
        if dUDECL > 1e-3 or dUDECU > 1e-3:
            print "UDEC[L,U] problem for %s, %s %s" % (self.tilename, dUDECL, dUDECU)

        cur.close()
        return
    
    def createCOADDTILE(self,table='felipe.coaddtile_new'):

        cur = self.dbh.cursor()

        # make sure we delete before we created
        drop = "drop   table %s purge" % table

        # Create command
        create = """
        create table %s (
        TILENAME                VARCHAR2(50),
        RA                      NUMBER(15,10),
        DEC                     NUMBER(15,10),
        RAC1			NUMBER(15,10),
        RAC2			NUMBER(15,10),
        RAC3			NUMBER(15,10),
        RAC4			NUMBER(15,10),
        DECC1			NUMBER(15,10),
        DECC2			NUMBER(15,10),
        DECC3			NUMBER(15,10),
        DECC4			NUMBER(15,10),
        RACMIN                  NUMBER(15,10),
        RACMAX                  NUMBER(15,10),
        DECCMIN                 NUMBER(15,10),
        DECCMAX                 NUMBER(15,10),
        URAL                    NUMBER(15,10),
        URAU                    NUMBER(15,10),
        UDECL                   NUMBER(15,10),
        UDECU                   NUMBER(15,10),
        CROSSRAZERO             CHAR(1) check (CROSSRAZERO in ('N','Y')),
        PIXELSCALE              NUMBER(10,5),
        NAXIS1                  NUMBER(6),
        NAXIS2                  NUMBER(6),
        CRPIX1                  NUMBER(15,10),
        CRPIX2                  NUMBER(15,10),
        CRVAL1                  NUMBER(15,10),
        CRVAL2                  NUMBER(15,10),
        CD1_1                   NUMBER(15,10),
        CD1_2                   NUMBER(15,10),
        CD2_1                   NUMBER(15,10),
        CD2_2                   NUMBER(15,10),
        constraint %s PRIMARY KEY (TILENAME)
        )
        """ % (table,table.split(".")[1])
        
        # -- Add description of columns
        comments ="""comment on column %s.DEC    is 'RA  center of DES tile (deg)' 
        comment on column %s.RA      is 'DEC center of DES file (deg)'
        comment on column %s.RAC1    is 'Corner 1 RA of DES tile (deg)'
        comment on column %s.RAC2    is 'Corner 2 RA of DES tile (deg)'
        comment on column %s.RAC3    is 'Corner 3 RA of DES tile (deg)'
        comment on column %s.RAC4    is 'Corner 4 RA of DES tile (deg)'
        comment on column %s.DECC1   is 'Corner 1 DEC of DES tile (deg)'
        comment on column %s.DECC2   is 'Corner 2 DEC of DES tile (deg)'
        comment on column %s.DECC3   is 'Corner 3 DEC of DES tile (deg)'
        comment on column %s.DECC4   is 'Corner 4 DEC of DES tile (deg)'
        comment on column %s.RACMIN  is 'Minimum RA[1-4] corner (deg)'
        comment on column %s.RACMAX  is 'Maximum RA[1-4] corner (deg)'
        comment on column %s.DECCMIN is 'Minimum DEC[1-4] corner (deg)'
        comment on column %s.DECCMAX is 'Maximum DEC[1-4] corner (deg)'
        comment on column %s.URAL    is 'Unique RA lower (deg)'
        comment on column %s.URAU    is 'Unique RA upper (deg)'
        comment on column %s.UDECL   is 'Unique DEC lower (deg)'
        comment on column %s.UDECU   is 'Unique DEC upper (deg)'
        comment on column %s.CROSSRAZERO  is 'DES tile crosses RA=0 [Y/N]'"""

        print "# Will create new COADDTILE table: %s" % table
        try:
            print "# Dropping table: %s" % table
            cur.execute(drop)
        except:
            print "# Could not drop table: %s -- probably doesn't exist" % table
        print "# Creating table: %s" % table
        cur.execute(create)

        print "# Adding comments to: %s" % table
        for comment in comments.split("\n"):
            print comment % table
            cur.execute(comment % table)

        # Grand permission
        roles = ['des_reader','PROD_ROLE','PROD_READER_ROLE']
        for role in roles:
            grant = "grant select on %s to %s" % (table.split(".")[1],role)
            print "# Granting permission: %s\n" % grant
            cur.execute(grant)

        self.dbh.commit()
        cur.close()
        return

    def insertCOADDTILE(self,table='felipe.coaddtile_new'):

        """
        To create a new db table in desoper
        
        1) mydesoper 
        2) SQL> @felipe.coaddtile_new.sql;
        3) SQL> grant select on coaddtile_new to des_reader; 
        """
        
        dbh = self.dbh

        columns = ('TILENAME',
                   'RA',
                   'DEC',
                   'RAC1',
                   'RAC2',
                   'RAC3',
                   'RAC4',
                   'DECC1',
                   'DECC2',
                   'DECC3',
                   'DECC4',
                   'RACMIN',
                   'RACMAX',
                   'DECCMIN',
                   'DECCMAX',
                   'URAL',
                   'URAU',
                   'UDECL',
                   'UDECU',
                   'CROSSRAZERO',
                   'PIXELSCALE',
                   'NAXIS1',
                   'NAXIS2',
                   'CRPIX1',
                   'CRPIX2',
                   'CRVAL1',
                   'CRVAL2',
                   'CD1_1',
                   'CD1_2',
                   'CD2_1',
                   'CD2_2')

        values = (self.tilename,
                  self.ra_center,
                  self.dec_center,
                  self.RAC1,
                  self.RAC2,
                  self.RAC3,
                  self.RAC4,
                  self.DECC1,
                  self.DECC2,
                  self.DECC3,
                  self.DECC4,
                  self.RACMIN,
                  self.RACMAX,
                  self.DECCMIN,
                  self.DECCMAX,
                  self.ural,
                  self.urau,
                  self.udecl,
                  self.udecu,
                  self.crossRAzero,
                  self.pixelscale,
                  self.header['NAXIS1'],
                  self.header['NAXIS2'],
                  self.header['CRPIX1'],
                  self.header['CRPIX2'],
                  self.header['CRVAL1'],
                  self.header['CRVAL2'],
                  self.header['CD1_1'],
                  self.header['CD1_2'],
                  self.header['CD2_1'],
                  self.header['CD2_2'])

        cols = ",".join(columns)
        insert_cmd = "INSERT INTO %s (%s) VALUES %s" % (table,cols,values)

        sys.stdout.write("\r# Inserting %s to %s" % (self.tilename,table))
        sys.stdout.flush()

        cur = self.dbh.cursor()
        cur.execute(insert_cmd)
        cur.close()
        return
    
    def generateTiles(self,
                      ra_ini   = 275, # do not change
                      dec_ini  = +30, # do not change
                      dec_end  = -85, # do not change
                      ra_range = 360, # do not change
                      checkDB = False,
                      writeDB = True,
                      sectionDB = 'db-desoper',
                      tablename = 'felipe.coaddtile_new'):

        """
        This is the ugly and unelengant loop to reproduce the *exact*
        DES tile arrangment on the sky.
        """
        
        # The final RA
        ra_end = ra_ini + ra_range

        # Connect if we'd like to check against DB
        if checkDB or writeDB:
            self.dbh = despydb.desdbi.DesDbi(section=sectionDB)

        # if write DB, initialize the table
        if writeDB: self.createCOADDTILE(table=tablename)
        
        d2r = math.pi/180. # degrees to radians shorthand
        
        # Width for declination in degrees
        degwidth      = self.pixelscale*self.NAXIS1/3600.
        dechalfwidth  = degwidth/2.0

        done = False
        dec = dec_ini
        while done is False: # DEC loop

            self.dec_center = dec
            
            # Get the overlap size
            overlap = self.get_overlap(dec)
            # Unique Width for declination
            udechalfwidth = (degwidth-overlap)/2.0
            # Increase in declination (silly -1 sign)
            incrdec  = -1.0*(degwidth - overlap)
            # RA unique increases, depend on value of declination
            raincr = (degwidth - overlap)/math.cos(dec*d2r)/2.0 # RA increase for unique vals

            ra = ra_ini
            firstra = 1
            while ra < ra_end: # RA loop

                # Unique Dec. lower and upper bounds
                self.udecl = dec - udechalfwidth
                self.udecu = dec + udechalfwidth
                # Unique lower and uppper bounds
                self.ural = ra - raincr
                self.urau = ra + raincr

                # Keep track of the initial point
                if firstra == 1:
                    saveural = self.ural
                    firstra = 0

                # Quick fix for formating the RA
                if ra > 360:
                    self.ra_center = ra - 360.0
                elif ra < 0: # In case we want to do negative RAs but want to keep names sanes
                    self.ra_center = ra + 360.0
                else:
                    self.ra_center = ra

                # Get the tilename --> self.tilename
                self.get_tilename()

                # Fix values > 360 for unique edges
                if self.ural > 360:
                    self.ural = self.ural - 360
                if self.urau > 360:
                    self.urau = self.urau - 360

                # We only need this for a full wrap around
                newra = ra + (degwidth - overlap)/math.cos(dec*d2r)
                if newra > ra_end and ra_range >= 360:
                    self.urau = saveural
                    
                # Flag the ones that cross RA zero
                # Oracle needs a 'Y/N' cannot handle True/False
                duras = abs(self.urau - self.ural)
                if  duras > 2*degwidth/math.cos(dec*d2r) :
                    self.crossRAzero = 'Y' 
                else:
                    self.crossRAzero = 'N'

                # We need to compare before we fix the ura[l,u] > 360 values
                # This slows down things quite a bit
                if checkDB: self.compareDB()
                
                # Create the header dictionary for that tile
                self.create_header()

                # Compute the new corners RAC[1,4] and DEC[1,4] -- needs a header dictionary
                self.computeCornersTilename()
                
                # Here we might want to add the module that will add entroies to a DB
                if writeDB: self.insertCOADDTILE(table=tablename)

                ra  = newra
                # End or RA loop

            # Increase in Declination
            dec = dec + incrdec
            # Stop when we reach dec1
            if dec < dec_end: done = True

        # Commit to DB
        if writeDB:
            self.dbh.commit()
            print ""

        return

############ DESTILE Header example #####################################
#NAXIS   =                    2 /
#NAXIS1  =                10000 / Number of pixels along this axis
#NAXIS2  =                10000 / Number of pixels along this axis
#CTYPE1  = 'RA---TAN'           / WCS projection type for this axis
#CUNIT1  = 'deg     '           / Axis unit
#CRVAL1  =   3.214829310000E+02 / World coordinate on this axis
#CRPIX1  =   5.000500000000E+03 / Reference pixel on this axis
#CD1_1   =  -7.305556000000E-05 / Linear projection matrix
#CD1_2   =   0.000000000000E+00 / Linear projection matrix
#CTYPE2  = 'DEC--TAN'           / WCS projection type for this axis
#CUNIT2  = 'deg     '           / Axis unit
#CRVAL2  =  -5.209722200000E+01 / World coordinate on this axis
#CRPIX2  =   5.000500000000E+03 / Reference pixel on this axis
#CD2_1   =   0.000000000000E+00 / Linear projection matrix
#CD2_2   =   7.305556000000E-05 / Linear projection matrix
##########################################################################

