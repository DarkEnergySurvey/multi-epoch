#!/usr/bin/env python

#from multiepoch import zipper_interp as zipp
import fitsio
import numpy 
import time

from despyfits import maskbits
from despyastro import astrometry
from despyastro import zipper_interp as zipp

if __name__ == "__main__":

    sci_file = '/Users/felipe/MULTIEPOCH_ROOT/TILEBUILDER/DES2246-4457-section/products/DES2246-4457_r_sci.fits'
    msk_file = '/Users/felipe/MULTIEPOCH_ROOT/TILEBUILDER/DES2246-4457-section/products/DES2246-4457_r_msk.fits'
    wgt_file = '/Users/felipe/MULTIEPOCH_ROOT/TILEBUILDER/DES2246-4457-section/products/DES2246-4457_r_wgt.fits'
    file_out = 'test.fits'
    
    print "Using fitsio version %s" % fitsio.__version__
    
    print "# Reading in %s" % sci_file
    scifits = fitsio.FITS(sci_file,'r')
    sci_hdr = scifits[0].read_header()
    
    print "# Reading in %s" % msk_file
    mskfits = fitsio.FITS(msk_file,'r')
    msk_hdr = mskfits[0].read_header()
    
    print "# Reading in %s" % wgt_file
    wgtfits = fitsio.FITS(wgt_file,'r')
    wgt_hdr = wgtfits[0].read_header()
    
    
    # Read in just a section
    SCI = scifits[0].read()
    MSK = mskfits[0].read()
    WGT = wgtfits[0].read()
    
    # Make the mask for MSK
    keep = numpy.where(SCI <= 0)
    MSK  = numpy.where(MSK == 0,1,0)
    # Make sure that we do not interpolate over zerors
    MSK  = numpy.where(SCI == 0,0,MSK)

    # Axis 2 -- cols
    SCI = zipp.zipper_interp(SCI,MSK,interp_mask=1,axis=2,BADPIX_INTERP=None,xblock=1,add_noise=False)

    # Axis 1 -- rows
    #SCI,MSK = zipp.zipper_interp(SCI,MSK,interp_mask=1,axis=1,BADPIX_INTERP=maskbits.BADPIX_INTERP)

    # Update the WCS in the headers and make a copy
    #h_section_sci = astrometry.update_wcs_matrix(sci_hdr,x0,y0,naxis1,naxis2)
    #h_section_msk = astrometry.update_wcs_matrix(msk_hdr,x0,y0,naxis1,naxis2)
    #h_section_wgt = astrometry.update_wcs_matrix(wgt_hdr,x0,y0,naxis1,naxis2)

    # Add to image history
    interp_mask = 1
    #h_section_sci['HISTORY'] = time.asctime(time.localtime()) + \
    #                           ' row_interp over mask 0x{:04X}'.format(interp_mask)
    
    print "# Writing %s" % file_out
    ofits = fitsio.FITS(file_out,'rw',clobber=True)
    ofits.write(SCI,extname='SCI',header=sci_hdr)
    ofits.write(MSK,extname='MSK',header=msk_hdr)
    ofits.write(WGT,extname='WGT',header=wgt_hdr)
    ofits.close()
    
