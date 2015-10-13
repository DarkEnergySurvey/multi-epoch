#!/usr/bin/env python

import fitsio
import numpy 
import time
import argparse
import logging
import os,sys

from despyfits import maskbits
from despyfits import compressionhdu as chdu
from despyastro import astrometry
from despyastro import zipper_interp as zipp
from despymisc.miscutils import elapsed_time


# -----------------------------------
# Translator for DES_EXT, being nice.
DES_EXT = {}
DES_EXT['SCI'] = 'IMAGE'
DES_EXT['WGT'] = 'WEIGHT'
DES_EXT['MSK'] = 'MASK'
# -----------------------

def cmdline():

    parser = argparse.ArgumentParser(description="Create a coadd MEF fits file from a list of single-plane fits files")
    
    parser.add_argument("sci_file", default=None,
                        help="Input SCI fits file")
    parser.add_argument("msk_file", default=None,
                        help="Input MSK fits file")
    parser.add_argument("wgt_file", default=None,
                        help="Input WGT fits file")
    parser.add_argument("-o","--outname", default=None, 
                        help="Name of output FITS file.")
    parser.add_argument("--clobber", action='store_true', default=False,
                        help="Clobber output MEF fits file")
    args = parser.parse_args()

    # Sanity ckecks for inputs...
    # Make sure that outname is defined
    if not args.outname:
        raise ValueError('Undefined outname as %s' % args.outname)
    
    # Output file exits
    if os.path.isfile(args.outname) and args.clobber is False:
        raise ValueError("Output file exists, try --clobber option, no files created")
    return args

def update_hdr_compression(hdr,extname):
    
    hdr['DES_EXT']  = DES_EXT[extname]
    hdr['FZALGOR']  = chdu.get_FZALGOR(extname)
    hdr['FZQMETHD'] = chdu.get_FZQMETHD(extname)
    hdr['FZDTHRSD'] = chdu.get_FZDTHRSD(extname)
    hdr['FZQVALUE'] = chdu.get_FZQVALUE(extname)
    return hdr

def create_logger(level=logging.NOTSET):

    logging.basicConfig(level=level,
                        format='[%(asctime)s] [%(levelname)s] %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')
    logger = logging.getLogger('Merge')
    return logger

def merge(**kwargs):

    """
    Merge the coadded science (SCI), mask (MSK) and weight (WGT) files
    into a single MEF file using fitsio, and perform zipper
    interpolation along columns on the SCI plane using the 'custom'
    MSK weight plane created by SWarp.

    Felipe Menanteau

    """

    sci_file    = kwargs.get('sci_file')
    msk_file    = kwargs.get('msk_file')
    wgt_file    = kwargs.get('wgt_file')
    logger      = kwargs.get('logger',None)
    outname     = kwargs.get('outname',False)
    clobber     = kwargs.get('clobber',True)
    interp_mask = kwargs.get('interp_mask',1)
    BADPIX_INTERP = kwargs.get('BADPIX_INTERP',maskbits.BADPIX_INTERP)
    verbose = True

    if not logger:
        logger = create_logger(level=logging.NOTSET)
        
    logger.info("Reading in %s" % sci_file)
    SCI,sci_hdr = fitsio.read(sci_file, ext=0, header=True)
    logger.info("Reading in %s" % msk_file)
    MSK,msk_hdr = fitsio.read(msk_file, ext=0, header=True)
    logger.info("Reading in %s" % wgt_file)
    WGT,wgt_hdr = fitsio.read(wgt_file, ext=0, header=True)
    
    # Make the mask for MSK
    MSK = numpy.where(MSK == 0,1,0)

    # Perform column interpolation -- Axis=2
    if BADPIX_INTERP:
        SCI,MSK = zipp.zipper_interp(SCI,MSK,interp_mask=interp_mask,axis=2,BADPIX_INTERP=BADPIX_INTERP,logger=logger)
    else:
        SCI     = zipp.zipper_interp(SCI,MSK,interp_mask=interp_mask,axis=2,BADPIX_INTERP=BADPIX_INTERP,logger=logger)

    # Update compression settings
    sci_hdr = update_hdr_compression(sci_hdr,'SCI')
    msk_hdr = update_hdr_compression(msk_hdr,'MSK')
    wgt_hdr = update_hdr_compression(wgt_hdr,'WGT')

    # Add to image history
    sci_hdr['HISTORY'] = time.asctime(time.localtime()) + \
                         ' column_interp over mask 0x{:04X}'.format(interp_mask)
    # Write it out now
    logger.info("Writing %s" % outname)
    ofits = fitsio.FITS(outname,'rw',clobber=True)
    ofits.write(SCI,extname='SCI',header=sci_hdr)
    ofits.write(MSK,extname='MSK',header=msk_hdr)
    ofits.write(WGT,extname='WGT',header=wgt_hdr)
    ofits.close()

    return

