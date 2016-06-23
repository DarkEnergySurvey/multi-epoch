import fitsio
import numpy

def gethdu(filename, extname='SCI'):
    # Explore extensions
    hdu = None
    with fitsio.FITS(filename) as fits:
        for k in xrange(len(fits)):
            h = fits[k].read_header()

            # Make sure that we can get the EXTNAME
            if not h.get('EXTNAME'):
                continue

            if h['EXTNAME'].strip() == extname:
                hdu = k
                
    if hdu is None:
        print "WARNING: Cannot find HDU for EXTNAME:%s" % extname
    return hdu

def addDECamMask(filename,outname,ext=0, plot=False,**kw):

    from drawDECam import drawDECam as dDECam

    """
    Mask a fitsfile with the DECam shape
    """
    data,header = fitsio.read(filename, ext=ext, header=True)
    masked_data = numpy.zeros(data.shape,dtype=data.dtype)

    # Get the ccds
    ccds = dDECam.getDECamCCDs(header,plot=plot,**kw)
    for k in ccds.keys():
        # Unpack the edges
        [x1,x2],[y1,y2] = ccds[k]
        masked_data[y1:y2,x1:x2] = data[y1:y2,x1:x2]  
    fitsio.write(outname,masked_data,header=header,clobber=True)
    return 
