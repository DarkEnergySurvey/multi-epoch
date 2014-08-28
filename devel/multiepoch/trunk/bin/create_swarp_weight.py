#!/usr/bin/env python

import fitsio
import numpy
import os,sys
import copy
import time

class DESFITS:

    """
    A Class to handle DECam fits/fits.fz files. Uses fitsio to 
    open/read/close files.
    
    Example:
    
    >>> desobj = DESFITS(fileName)
    >>> desobj.write()
    """

    def __init__ (self, fileName, outName, clobber=False,**kwargs):
        self.fileName  = extract_filename(fileName)
        self.outName   = extract_filename(outName)

        if not clobber and os.path.exists(self.outName):
            print "# Skipping %s " % self.outName
            sys.exit()

        # Gets SCI, MSK, WGT and created VAR
        self.read_HDUs()
  
        # Make copies (shallow) of SCI, MSK, WGT and created VAR (OUT_*)
        self.copy_ndarrays() 
  
    def read_HDUs(self):
        """
        Read in the HDU as ndarrays and headers as dictionaries with fitsio for a DESDM/DECam image
        """
        # Get the fitsio element -- we'll modify this in place
        print "# Reading in extensions and headers for %s" % self.fileName
        self.ifits = fitsio.FITS(self.fileName,'r')
        sci_hdu, msk_hdu, wgt_hdu = get_hdu_numbers(self.ifits)
        # Read in the Science, Mask and Weight Images array with fitsio, as we'll
        # need to write them out using fitsio once we are done with them
        self.SCI = self.ifits[sci_hdu].read()
        self.MSK = self.ifits[msk_hdu].read()
        self.WGT = self.ifits[wgt_hdu].read()
        # Now let's read the headers
        self.h_sci = self.ifits[sci_hdu].read_header()
        self.h_msk = self.ifits[msk_hdu].read_header()
        self.h_wgt = self.ifits[wgt_hdu].read_header()     
        # Get the image size to set the allowed fraction of image to be already masked
        (self.ny,self.nx) = self.SCI.shape
        # Pass them up
        self.sci_hdu = sci_hdu
        self.msk_hdu = msk_hdu
        self.wgt_hdu = wgt_hdu
        print "# Done reading HDU "
  
    def copy_ndarrays(self):
        """
        Make shallow copies (shallow is enough) of the SCI, MSK and WGT
        ndarrays using python copy function to preserve the original
        information of the fits files. We need to do this before they
        are modified in place by the the LSST framework functions.
        """
        print "# Making shallow copies of SCI, MSK and WGT ndarrays"
        self.OUT_SCI = copy.copy(self.SCI)
        self.OUT_MSK = copy.copy(self.MSK)
        self.OUT_WGT = copy.copy(self.WGT)
        # A handy handle
        self.headers = (self.h_sci,self.h_msk,self.h_wgt)
        # Let's make a handle for the DESDM object
        self.DESDMImage = (self.OUT_SCI,self.OUT_MSK,self.OUT_WGT)

    def fix_weight(self):

        """ Fix the weight of the interp pixels
        INTERPbit  = 2^2 = 4
        TRAIL      = 2^6 = 64
        """

        masked_good   = numpy.where(self.MSK == 0)
        masked_trail =  numpy.where((self.MSK & 64) > 0)
        #masked_interp = numpy.where((self.MSK & 4)  > 0)
        
        # Replace by uniform distribution around mean
        # We might want to do a poison or gaussian
        mean = self.WGT[masked_good].mean()
        rms  = self.WGT[masked_good].std()
        Ntrail = len(masked_trail[0])
        rep_vals = numpy.random.uniform(mean-rms,mean + rms, Ntrail)

        self.OUT_WGT[masked_trail] = rep_vals
        print "# Fixing TRAIL = 2^6=64 by uniform"

    def check_outName(self):
        """
        Make sure that fpack files have the .fz extension
        """
        baseName = os.path.basename(self.outName)
        extName  = os.path.splitext(baseName)[1]
      
        if self.compress and extName == '.fits':
            raise IOError ("--compress specified with '.fits' outfile")

        if not self.compress and extName == '.fz':
            raise IOError ("--compress not specified with '.fz' outfile")


    def write(self,**kwargs):

        """
        Use fitsio to write the output file compressed or not
        """
         
        # Decide if compress, that will define the fileName, compression type and tile_dims
        self.compress  = kwargs.get('compress',None)
        # Define type of compresion and tile_dims
        if self.compress:
            self.compress  = 'RICE'
            self.tile_dims = [1,2048]
        else:
            self.compress  = None
            self.tile_dims = None
         
        # Check the output name is consistent with compression
        self.check_outName()
        # Write the output file, one HDU at a time
        ofits = fitsio.FITS(self.outName,'rw',clobber=True)
        # Science -- use scia -- ndarray representation
        ofits.write(self.OUT_SCI,header=self.h_sci,compress=self.compress,tile_dims=self.tile_dims)
        # The Mask
        ofits.write(self.OUT_MSK,header=self.h_msk,compress=self.compress,tile_dims=self.tile_dims)
        # The Weight
        ofits.write(self.OUT_WGT,header=self.h_wgt,compress=self.compress,tile_dims=self.tile_dims)
        # Close the file
        ofits.close()
        print >>sys.stderr,"# Wrote: %s" % self.outName

###########################
# General useful functions
###########################
def get_hdu_numbers(FITS):
    """
    Simple function to figure the HDU extensions for DESDM fits files
    in:  a fitsio.FITS object
    out: (sci_ext, msk_ext, wgt_ext) extension numbers 
    """
    sci_ext = None
    msk_ext = None
    wgt_ext = None
    # Loop trough each HDU on the fits file
    for i in range(len(FITS)):
        h = FITS[i].read_header()       # Get the header
        if ('DES_EXT' in h.keys()) :
            extname = h['DES_EXT'].strip()
            if   (extname == 'IMAGE') : sci_ext = i
            elif (extname == 'MASK')  : msk_ext = i
            elif (extname == 'WEIGHT'): wgt_ext = i
  
    if (sci_ext is None or msk_ext is None or wgt_ext is None):
        raise ValueError("Cannot find IMAGE, MASK, WEIGHT extensions via DES_EXT keyword in header")

    return sci_ext,msk_ext,wgt_ext

# Safe extraction of filename
def extract_filename(filename):
    if filename[0] == "!": filename=filename[1:]
    filename = os.path.expandvars(filename)
    filename = os.path.expanduser(filename)
    return filename

# Format time
def elapsed_time(t1,verb=False):
    import time
    t2    = time.time()
    stime = "%dm %2.2fs" % ( int( (t2-t1)/60.), (t2-t1) - 60*int((t2-t1)/60.))
    if verb:
        print >>sys.stderr,"Elapsed time: %s" % stime
    return stime


if __name__ == "__main__":

    # Get the start time
    t0 = time.time()
    
    fileName = sys.argv[1]
    outName  = sys.argv[2]
    clobber = False

    desfits = DESFITS(fileName,outName,clobber=False)
    desfits.fix_weight()
    desfits.write()
    print >>sys.stderr,"# Time:%s" % elapsed_time(t0)
