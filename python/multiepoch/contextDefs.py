
"""
Set of functions to add defintions to the context
"""


def set_tile_directory(ctx,outputpath='./TILEBUILDER'):

    """ Se the output directory for the TILE to be built"""

    import os

    if ctx.get('basename'):
        ctx.basedir = os.path.dirname(ctx.basename)
    else:
        ctx.basedir  = os.path.join(outputpath, ctx.tilename)
        ctx.basename = os.path.join(ctx.basedir, ctx.tilename)

    # In most cases filepattern == tilename
    ctx.filepattern = os.path.basename(ctx.basename)

    ctx.auxdir = os.path.join(ctx.basedir,"aux")
    ctx.logdir = os.path.join(ctx.basedir,"log")
    
    # Make sure that the filepaths exists
    if not os.path.exists(ctx.auxdir):
        print "# Creating directory: %s" % ctx.auxdir
        os.makedirs(ctx.auxdir)

    if not os.path.exists(ctx.logdir):
        print "# Creating directory: %s" % ctx.logdir
        os.makedirs(ctx.logdir)
    #else:
    #    print "# Will write output files to: %s" % ctx.basedir
    return ctx

def get_local_weight_names(ctx,wgt_ext):

    """
    A common method to define the local weight names based on
    FILEPATH_LOCAL passed in the ctx, and wgt_ext
    """
    # A shortcut
    filepath_local = ctx.assoc['FILEPATH_LOCAL']
    Nfiles = len(filepath_local)
    # Define the wgt local filenames
    filepath_local_wgt = []
    for k in range(Nfiles):
        basename  = filepath_local[k].split(".fits")[0] 
        extension = filepath_local[k].split(".fits")[1:]
        local_wgt = "%s%s.fits" % (basename,wgt_ext)
        filepath_local_wgt.append(local_wgt)
    return filepath_local_wgt

def set_BANDS(ctx,detname='det',detBANDS=[], force=False):

    import numpy

    """
    Generic function to set up the band from the context information
    into the context in case they are missing. This function defines
    how the BAND names are to be setup at every step into the context
    in case they are not present
    """

    if not ctx.get('BANDinfo') or force:
        print "# Setting the BANDs information in the context"
        ctx.BANDS    = numpy.unique(ctx.assoc['BAND']) 
        ctx.NBANDS   = len(ctx.BANDS)                  
        # --------------
        # MIGHT BE USEFUL IN CASE WE WANT TO  NAME THE Detection as 'det_riz'
        # In case we want to store with the 'real bands' as a list to access later
        # Figure out which bands to use that match the detecBANDS
        #useBANDS = list( set(ctx.BANDS) & set(detecBANDS) )
        #print "# Will use %s bands for detection" % useBANDS
        #ctx.detBAND ='det%s' % "".join(useBANDS)
        # ---------------------

        # The SWarp-combined detection image input and ouputs
        ctx.detBAND  ='%s' % detname
        ctx.dBANDS   = list(ctx.BANDS) + [ctx.detBAND]
        ctx.BANDinfo = True
    else:
        print "# BANDs already setup -- Skipping"
    return ctx

def set_SWarp_output_names(ctx,detname='det'):


    """ Add SWarp output names to the context in case they are not present """

    # Make sure that bands have been set
    ctx = set_BANDS(ctx,detname)

    if not ctx.get('comb_sci') or not ctx.get('comb_wgt'):
        
        # SWarp outputs per filer
        ctx.comb_sci      = {} # SWarp coadded science images
        ctx.comb_wgt      = {} # SWarp coadded weight images
        ctx.comb_sci_tmp  = {} # SWarp coadded temporary science images  -- to be removed later
        ctx.comb_wgt_tmp  = {} # SWarp coadded custom weight images -- to be removed

        # Loop over bands
        for BAND in ctx.dBANDS:
            # SWarp outputs names
            ctx.comb_sci[BAND]     = "%s_%s_sci.fits" %  (ctx.basename, BAND)
            ctx.comb_wgt[BAND]     = "%s_%s_wgt.fits" %  (ctx.basename, BAND)
            # temporary files need for dual-run -- to be removed
            ctx.comb_sci_tmp[BAND] = "%s_%s_sci_tmp.fits" %  (ctx.basename, BAND)
            ctx.comb_wgt_tmp[BAND] = "%s_%s_wgt_tmp.fits" %  (ctx.basename, BAND)
    else:
        print "# SWarp output names already in the context"

    return ctx
