
"""
Set of functions to add defintions to the context
"""


def create_local_archive(local_archive):
    import os
    """ Creates the local cache for the desar archive """
    if not os.path.exists(local_archive):
        print "# Will create LOCAL ARCHIVE at %s" % local_archive
        os.mkdir(local_archive)
    return

def define_weight_names(ctx):
    import os
    """ A common method to define the weights names bases in the context"""
    filepath_local_wgt = []
    for idx in range(len(ctx.assoc['FILENAME'])):
        local_wgt = os.path.join(ctx.local_weights,
                                 ctx.CCDS[idx]['PATH'],
                                 ctx.CCDS[idx]['FILENAME'].replace('.fits','{we}.fits'.format(we=ctx.weight_extension))
                                 )
        filepath_local_wgt.append(local_wgt)
    return filepath_local_wgt

def get_BANDS(assoc, detname='det', logger=None):

    import numpy

    """
    Generic function to set up the band from the context information
    into the context in case they are missing. This function defines
    how the BAND names are to be setup at every step into the context
    in case they are not present
    """
    if logger: logger.info("# Extracting the BANDs information from assoc")
    ctxext = {}
    ctxext['BANDS'] = numpy.unique(assoc['BAND']) 
    ctxext['NBANDS'] = len(ctxext['BANDS'])                  
    # --------------
    # MIGHT BE USEFUL IN CASE WE WANT TO  NAME THE Detection as 'det_riz'
    # In case we want to store with the 'real bands' as a list to access later
    # Figure out which bands to use that match the detecBANDS
    #ctxext['useBANDS'] = list( set(ctxext['BANDS']) & set(ctxext['detecBANDS']) )
    #print "# Will use %s bands for detection" % 'ctxext['useBANDS']
    #detBAND ='det%s' % "".join(['useBANDS'])
    # ---------------------
    # The SWarp-combined detection image input and ouputs
    ctxext['detBAND'] ='%s' % detname
    ctxext['dBANDS'] = list(ctxext['BANDS']) + [ctxext['detBAND']]
    return ctxext 


##### Deprecated functions ##############
#
# def set_SWarp_output_names(ctx,detname='det',force=False):
#
#     """ Add SWarp output names to the context in case they are not present """
#
#
#     # Only add if not in context or forced
#     if not ctx.get('swarp_names') or force:
#
#         print "# Setting the SWarp output names to the context"
#
#         # Make sure that bands have been set
#         ctx = set_BANDS(ctx,detname)
#
#         # SWarp outputs per filer
#         ctx.comb_sci      = {} # SWarp coadded science images
#         ctx.comb_wgt      = {} # SWarp coadded weight images
#         ctx.comb_sci_tmp  = {} # SWarp coadded temporary science images  -- to be removed later
#         ctx.comb_wgt_tmp  = {} # SWarp coadded custom weight images -- to be removed
#         ctx.comb_MEF      = {} # SWarp coadded MEF files with SCI/WGT
#
#         # Loop over bands
#         for BAND in ctx.dBANDS:
#             # SWarp outputs names
#             ctx.comb_sci[BAND]     = "%s_%s_sci.fits"     %  (ctx.basename, BAND)
#             ctx.comb_wgt[BAND]     = "%s_%s_wgt.fits"     %  (ctx.basename, BAND)
#             # temporary files need for dual-run -- to be removed
#             ctx.comb_sci_tmp[BAND] = "%s_%s_sci_tmp.fits" %  (ctx.basename, BAND)
#             ctx.comb_wgt_tmp[BAND] = "%s_%s_wgt_tmp.fits" %  (ctx.basename, BAND)
#             ctx.comb_MEF[BAND]     = "%s_%s.fits" %  (ctx.basename, BAND)
#
#         ctx.swarp_names = True
#     else:
#         print "# SWarp output names already in the context -- Skipping"
#
#     return ctx


# def setCatNames(ctx,detname='det',force=False):
#
#     """ Set the names for input/ouput for psfex/Sextractor calls"""
#
#     # Only add if not in context or forced
#     if not ctx.get('cat_names') or force:
#
#         print "# Setting the output names for SExPSF/psfex and SExDual and adding them to the context"
#         # Make sure that bands have been set
#         ctx = set_BANDS(ctx,detname)
#       
#         # SExPSF
#         ctx.psfcat = {}
#         ctx.psf    = {}
#         # PsfCall
#         ctx.psfexxml = {}
#         # SExDual
#         ctx.checkimage = {}
#         ctx.cat = {}
#         for BAND in ctx.dBANDS:
#             # SExPSF
#             ctx.psf[BAND]       = "%s_%s_psfcat.psf"  %  (ctx.basename, BAND)
#             ctx.psfcat[BAND]    = "%s_%s_psfcat.fits" %  (ctx.basename, BAND)
#             # psfex
#             ctx.psfexxml[BAND]  = "%s_%s_psfex.xml"   %  (ctx.basename, BAND)
#             # SExDual
#             ctx.cat[BAND]       = "%s_%s_cat.fits"    %  (ctx.basename, BAND)
#             ctx.checkimage[BAND]= "%s_%s_seg.fits"    %  (ctx.basename, BAND)
#
#         print "# Done with Catalogs names"
#         ctx.cat_names = True
#     else:
#         print "# Catalogs output names already in the context -- Skipping"
#
#     return ctx


# def set_BANDS(ctx,detname='det',detBANDS=[], force=False):
#
#     import numpy
#
#     """
#     Generic function to set up the band from the context information
#     into the context in case they are missing. This function defines
#     how the BAND names are to be setup at every step into the context
#     in case they are not present
#     """
#
#     if not ctx.get('BANDinfo') or force:
#         print "# Setting the BANDs information in the context"
#         ctx.BANDS    = numpy.unique(ctx.assoc['BAND']) 
#         ctx.NBANDS   = len(ctx.BANDS)                  
#         # --------------
#         # MIGHT BE USEFUL IN CASE WE WANT TO  NAME THE Detection as 'det_riz'
#         # In case we want to store with the 'real bands' as a list to access later
#         # Figure out which bands to use that match the detecBANDS
#         #useBANDS = list( set(ctx.BANDS) & set(detecBANDS) )
#         #print "# Will use %s bands for detection" % useBANDS
#         #ctx.detBAND ='det%s' % "".join(useBANDS)
#         # ---------------------
#
#         # The SWarp-combined detection image input and ouputs
#         ctx.detBAND  ='%s' % detname
#         ctx.dBANDS   = list(ctx.BANDS) + [ctx.detBAND]
#         ctx.BANDinfo = True
#     else:
#         print "# BANDs already setup -- Skipping"
#     return ctx


