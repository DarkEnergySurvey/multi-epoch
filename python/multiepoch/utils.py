
"""
Set of utility functions used across different multi-epoch tasks
"""

# Check if database handle is in the context
def check_dbh(ctx):

    from despydb import desdbi

    """ Check if we have a valid database handle (dbh)"""
    
    if 'dbh' not in ctx:
        try:
            db_section = ctx.get('db_section','db-desoper')
            print "# Creating db-handle to section: %s" % db_section
            ctx.dbh = desdbi.DesDbi(section=db_section)
        except:
            raise ValueError('ERROR: Database handler could not be provided for context.')
    else:
        print "# Will recycle existing db-handle"
    return ctx


def get_NP(MP):

    """ Get the number of processors in the machine
    if MP == 0, use all available processor
    """
    import multiprocessing
    
    # For it to be a integer
    MP = int(MP)
    if MP == 0:
        NP = multiprocessing.cpu_count()
    elif isinstance(MP,int):
        NP = MP
    else:
        raise ValueError('MP is wrong type: %s, integer type' % MP)
    return NP

def create_local_archive(local_archive):
    
    import os
    """ Creates the local cache for the desar archive """
    if not os.path.exists(local_archive):
        print "# Will create LOCAL ARCHIVE at %s" % local_archive
        os.mkdir(local_archive)
    return


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

def dict2arrays(dictionary):
    """
    Re-cast list in contained in a dictionary as numpy arrays
    """
    import numpy
    for key, value in dictionary.iteritems():
        if isinstance(value, list):
            dictionary[key] = numpy.array(value)
    return dictionary


def set_BANDS(ctx,detname='det',detBANDS=[]):

    import numpy

    """
    Generic function to set up the band from the context information
    into the context in case they are missing. This function defines
    how the BAND names are to be setup at every step into the context
    in case they are not present
    """

    if not ctx.get('BANDinfo'):
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
