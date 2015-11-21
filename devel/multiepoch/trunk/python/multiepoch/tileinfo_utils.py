from despyastro import CCD_corners
import fitsio
import json

def define_tileinfo(tilename,**kwargs):

    # These are all of the inputs
    xsize       = kwargs.get('xsize')
    ysize       = kwargs.get('ysize')
    ra_center   = kwargs.get('ra_cent')
    dec_center  = kwargs.get('dec_cent')
    pixelscale  = kwargs.get('pixelscale',0.263) # in arsec/pixel
    units       = kwargs.get('units','arcmin')
    json_file   = kwargs.get('json_file',None)

    # Get the dimensions
    NAXIS1, NAXIS2 = get_image_size(xsize,ysize, pixelscale=pixelscale, units=units)

    kw = {'pixelscale' : pixelscale,
          'ra_cent'    : ra_center,
          'dec_cent'   : dec_center,
          'NAXIS1'     : NAXIS1,
          'NAXIS2'     : NAXIS2}
    # create hdr and update corners
    header = create_header(**kw)

    # Now we write the json file
    if json_file:
        print "# Writing json file to %s" % json_file
        write_tileinfo_json(tilename,header,json_file)

    return header

def write_tileinfo_json(tilename,hdr,json_file):
    
    # TODO:
    # Make all keywords consisten with CCD_corners.update_DESDM_corners definitions
    # these are not consisten and should be fixed in destiling and when generating the tiles definitions
    dict = {
        'RA'          : hdr['RA_CENT'],
        'DEC'         : hdr['DEC_CENT'],
        'CROSSRAZERO' : hdr['CROSSRA0']}

    keys = ['RAC1', 'RAC2', 'RAC3', 'RAC4',
            'DECC1', 'DECC2', 'DECC3', 'DECC4',
            'RACMIN','RACMAX','DECCMIN','DECCMAX','PIXELSCALE']
    for k in keys:
        dict[k] = hdr[k]

    # Make it a json-like dictionary
    json_dict = {"tileinfo":dict,
                 "tilename":tilename}
    # Now write it
    with open(json_file, 'w') as outfile:
        json.dump(json_dict, outfile, sort_keys = True, indent = 4)
    return

def get_image_size(xsize,ysize, pixelscale=0.263, units='arcmin'):

    """ Computes the NAXIS1/NAXIS2 for a pixel scale and TAN projection """

    if units == 'arcsec':
        scale = pixelscale
    elif units == 'arcmin':
        scale = pixelscale/60.
    elif units == 'degree':
        scale = pixelscale/3600.
    elif units == 'pixel':
        scale = 1
    else:
        exit("ERROR: units not defined")
    NAXIS1 = int(xsize/scale)
    NAXIS2 = int(ysize/scale)
    return NAXIS1,NAXIS2
    

def create_header(**kwargs):


    """ Defines in full the tile header as a dictionary, notice that only CRVAL[1,2] are changing"""

    pixelscale = kwargs.get('pixelscale',0.263) # in arsec/pixel
    NAXIS1     = kwargs.get('NAXIS1')
    NAXIS2     = kwargs.get('NAXIS2')
    RA_CENT    = kwargs.get('ra_cent')  # in dec
    DEC_CENT   = kwargs.get('dec_cent') # in dec
    
    header_dict = {
        'NAXIS'   :  2,                      #/ Number of pixels along this axis
        'NAXIS1'  :  NAXIS1,            #/ Number of pixels along this axis
        'NAXIS2'  :  NAXIS2,            #/ Number of pixels along this axis
        'CTYPE1'  : 'RA---TAN',              #/ WCS projection type for this axis
        'CTYPE2'  : 'DEC--TAN',              #/ WCS projection type for this axis
        'CUNIT1'  : 'deg',                   #/ Axis unit
        'CUNIT2'  : 'deg',                   #/ Axis unit
        'CRVAL1'  :  RA_CENT,         #/ World coordinate on this axis
        'CRPIX1'  :  (NAXIS1+1)/2.0,    #/ Reference pixel on this axis
        'CD1_1'   :  pixelscale/3600., #/ Linear projection matrix -- CD1_1 is negative
        'CD1_2'   :  0,                      #/ Linear projection matrix -- CD1_2 is zero, no rotation
        'CRVAL2'  :  DEC_CENT,        #/ World coordinate on this axis
        'CRPIX2'  :  (NAXIS2+1)/2.0,    #/ Reference pixel on this axis
        'CD2_1'   :  0,                      #/ Linear projection matrix -- CD2_1 is zero, no rotation
        'CD2_2'   :  pixelscale/3600.,  #/ Linear projection matrix -- CD2_2 is positive
        'PIXELSCALE' :  pixelscale  #/ Linear projection matrix -- CD2_2 is positive
        }

    header = fitsio.FITSHDR()
    for k, v in header_dict.items():
        new_record = {'name': k,'value':v}
        header.add_record(new_record)

    # Update corners
    header = CCD_corners.update_DESDM_corners(header,border=0,get_extent=True,verb=False)
    return header


