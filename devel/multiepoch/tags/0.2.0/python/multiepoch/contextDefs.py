
"""
Set of functions to add defintions to the context
"""

import querylibs 
import utils
import os
import copy

def create_local_archive(local_archive):
    import os
    """ Creates the local cache for the desar archive """
    if not os.path.exists(local_archive):
        print "# Will create LOCAL ARCHIVE at %s" % local_archive
        os.mkdir(local_archive)
    return

def define_https_names(ctx,logger=None):
    
    """
    Define FILEPATH_HTTPS using the information on FILEPATH_LOCAL and the context
    """

    if 'root_https' not in ctx.assoc.keys():
        ctx = utils.check_dbh(ctx, logger=logger)
        ctx.root_https = querylibs.get_root_https(ctx.dbh,logger=logger, archive_name=ctx.archive_name)

    filepath_https = ctx.assoc['FILEPATH_LOCAL']
    filepath_https = [f.replace(ctx.local_archive,'{path}'.format(path=ctx.root_https)) for f in filepath_https]

    return filepath_https

def define_weight_names(ctx):

    """
    A common method to define the weights names based in the
    context using the information contained in assoc[FILEPATH_LOCAL]
    """

    # short-cuts for clarity
    lo_ar = ctx.local_archive
    lw_ar = ctx.local_weight_archive
    we    = ctx.weight_extension
    filepath_local_weight = ctx.assoc['FILEPATH_LOCAL']

    # 1. replace local_archive --> local_weight_archive
    filepath_local_weight = [f.replace(lo_ar,  '{lw}'.format(lw=lw_ar))   for f in filepath_local_weight]
    # 2. replace  .fits --> _wgt.fits
    filepath_local_weight = [f.replace('.fits','{we}.fits'.format(we=we)) for f in filepath_local_weight]

    return filepath_local_weight

def define_me_names(ctx):

    """
    A common method to define the weights names based in the
    context using the information contained in assoc[FILEPATH_LOCAL]
    """

    # short-cuts for clarity
    lo_ar = ctx.local_archive
    me_ar = ctx.local_archive_me
    mex   = ctx.extension_me
    filepath_local_me = ctx.assoc['FILEPATH_LOCAL']

    # 1. replace local_archive --> local_archive_me
    filepath_local_me = [f.replace(lo_ar,  '{lw}'.format(lw=me_ar))   for f in filepath_local_me]
    # 2. replace  .fits --> _me.fits
    filepath_local_me = [f.replace('.fits','{me}.fits'.format(me=mex)) for f in filepath_local_me]

    return filepath_local_me


def get_BANDS(assoc, detname='det', logger=None, doBANDS=['all']):

    import numpy

    """
    Generic function to set up the band from the context information
    into the context in case they are missing. This function defines
    how the BAND names are to be setup at every step into the context
    in case they are not present
    """

    # Avoid the Unicode 'u' in detname
    detname = detname.encode('ascii')

    if logger: logger.info("Extracting the BANDs information from assoc")
    ctxext = {}
    ctxext['BANDS']   = numpy.unique(assoc['BAND']).tolist() 
    ctxext['NBANDS']  = len(ctxext['BANDS'])                  

    # The SWarp-combined detection image input and ouputs
    ctxext['detBAND'] ='%s' % detname

    # Logic to define the bands we want to loop over actually as defined by the --doBANDS option
    if 'all' in doBANDS or doBANDS =='all':     # Define doBANDS as BANDS if all
        ctxext['doBANDS'] = ctxext['BANDS']
        ctxext['dBANDS']  = list(ctxext['doBANDS']) + [detname]
    else:
        ctxext['doBANDS'] = copy.copy(doBANDS) # Make copies to avoid list.remove(item)
        ctxext['dBANDS']  = copy.copy(doBANDS)
        if detname not in ctxext['dBANDS']:
            ctxext['dBANDS'] = ctxext['dBANDS'] + [detname]


    # Safe catch Remove detname from doBANDS if present
    if detname in ctxext['doBANDS']:
        ctxext['doBANDS'].remove(detname)

    # Set FLAG not to override context
    logger.info("Setting context gotBANDS=True")
    ctxext['gotBANDS'] = True
    return ctxext 


