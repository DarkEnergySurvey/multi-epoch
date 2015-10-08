#!/usr/bin/env python

from multiepoch import zipper_interp as zipp
import fitsio
import numpy 
from despyfits import maskbits


def update_wcs_matrix(header,x0,y0,naxis1,naxis2):

    from despyastro import wcsutil

    """
    Update the wcs header object with the right CRPIX[1,2] CRVAL[1,2] for a given subsection
    """
    import copy
    # We need to make a deep copy/otherwise if fails
    h = copy.deepcopy(header)
    # Get the wcs object
    wcs = wcsutil.WCS(h)
    # Recompute CRVAL1/2 on the new center x0,y0
    CRVAL1,CRVAL2 = wcs.image2sky(x0,y0)
    # Asign CRPIX1/2 on the new image
    CRPIX1 = int(naxis1/2.0)
    CRPIX2 = int(naxis2/2.0)
    # Update the values
    h['CRVAL1'] = CRVAL1
    h['CRVAL2'] = CRVAL2
    h['CRPIX1'] = CRPIX1
    h['CRPIX2'] = CRPIX2
    return h



if __name__ == "__main__":


    sci_file = '/Users/felipe/DES2246-4457-pre2/products/DES2246-4457_r_sci.fits'
    msk_file = '/Users/felipe/DES2246-4457-pre2/products/DES2246-4457_r_msk.fits'
    wgt_file = '/Users/felipe/DES2246-4457-pre2/products/DES2246-4457_r_wgt.fits'
    file_out = 'test.fits'
    
    print "Using fitsio version %s" % fitsio.__version__
    
    x0 = 2336
    y0 = 4901

    dx = 2000
    dy = 1000
    
    x1 = x0-dx
    x2 = x0+dx
    y1 = y0-dy
    y2 = y0+dy
    
    if y1<0: y1=0
    if x1<0: x1=0

    naxis1 = 2*dx
    naxis2 = 2*dy
    
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
    SCI = scifits[0][y1:y2,x1:x2]
    MSK = mskfits[0][y1:y2,x1:x2]
    WGT = wgtfits[0][y1:y2,x1:x2]
    
    # Make the mask for MSK
    MSK = numpy.where(MSK == 0,1,0)

    # Axis 2 -- cols
    SCI,MSK = zipp.zipper_interp(SCI,MSK,interp_mask=1,axis=2,BADPIX_INTERP=maskbits.BADPIX_INTERP)

    # Axis 1 -- rows
    #SCI,MSK = zipp.zipper_interp(SCI,MSK,interp_mask=1,axis=1,BADPIX_INTERP=maskbits.BADPIX_INTERP)

    # Update the WCS in the headers and make a copy
    h_section_sci = update_wcs_matrix(sci_hdr,x0,y0,naxis1,naxis2)
    h_section_msk = update_wcs_matrix(msk_hdr,x0,y0,naxis1,naxis2)
    h_section_wgt = update_wcs_matrix(wgt_hdr,x0,y0,naxis1,naxis2)
    
    print "# Writing %s" % file_out
    ofits = fitsio.FITS(file_out,'rw',clobber=True)
    ofits.write(SCI,extname='SCI',header=h_section_sci)
    ofits.write(MSK,extname='MSK',header=h_section_msk)
    ofits.write(WGT,extname='WGT',header=h_section_wgt)
    ofits.close()
    
