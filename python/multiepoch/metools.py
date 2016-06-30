import fitsio
import numpy
import os

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

def compare2fits(file1,file2,write=False):

    # Make sure the exists
    exists1 = os.path.exists(file1)
    exists2 = os.path.exists(file2)

    if not exists1 or not exists2:
        print "# WARNING: Files do not exists:"
        if not exists1: print "# %s: %s" % (file1,exists1)
        if not exists2: print "# %s: %s" % (file2,exists2)
        return

    # Make sure that have the same number of hdus
    fits1 = fitsio.FITS(file1) 
    hdus1 = xrange(len(fits1))
    fits2 = fitsio.FITS(file2) 
    hdus2 = xrange(len(fits2))
    
    for hdu in hdus1:
        data1,header1 = fitsio.read(file1,ext=hdu,header=True)
        data2,header2 = fitsio.read(file2,ext=hdu,header=True)
        diff = data1-data2
        Npix = diff.shape[0]*diff.shape[1]
        ix = numpy.where(diff != 0)
        Ndiff = len(ix[0])
        if Ndiff > 0:
            fraction = float(Ndiff)/float(Npix)
            v1    = diff[ix].min()
            v2    = diff[ix].max()
            vmean = diff[ix].mean()
            vmedian = numpy.median(diff[ix])
            print "# FILES ARE DIFERENT at %s pixels or %6.3f perc, %s,%s,%s,%s (min,max,mean,median) " % (Ndiff,100*fraction, v1,v2,vmean,vmedian)

        else:
            print "%s hdu %s -- %s" % (file1, hdu,Ndiff)

        if write:
            base,ext = os.path.splitext(os.path.basename(file1))
            outname = "%s_diff%s" % (base,ext)
            print "# Will write file: %s" % outname
            fitsio.write(outname,diff,header=header1,clobber=True)

                
