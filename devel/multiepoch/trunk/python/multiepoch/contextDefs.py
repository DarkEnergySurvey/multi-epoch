
"""
Set of functions to add defintions to the context
"""

import querylibs 
import utils
import os
import copy
import numpy


def create_local_archive(local_archive):
    import os
    """ Creates the local cache for the desar archive """
    if not os.path.exists(local_archive):
        print "# Will create LOCAL ARCHIVE at %s" % local_archive
        os.mkdir(local_archive)
    return

# Soon to be deprecarted -- this is the particular case where we do this for the ctx.assoc alone
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


def define_https_by_name(ctx,name='assoc',logger=None):
    
    """
    Define FILEPATH_HTTPS using the information on FILEPATH_LOCAL and the context for a name in the context
    """

    if 'root_https' not in ctx[name].keys():
        ctx = utils.check_dbh(ctx, logger=logger)
        ctx.root_https = querylibs.get_root_https(ctx.dbh,logger=logger, archive_name=ctx.archive_name)

    filepath_https = ctx[name]['FILEPATH_LOCAL']
    filepath_https = [f.replace(ctx.local_archive,'{path}'.format(path=ctx.root_https)) for f in filepath_https]

    return filepath_https


def extract_flabel(files):

    """ Extract and f_label from file and make sures it is unique"""

    # Remove .fz from all files -- if present
    files = [f.replace('.fits.fz','.fits') for f in files]

    flabel_cat = [os.path.splitext(os.path.basename(f))[0].split('_')[-1] for f in files]
    flabs = numpy.unique(flabel_cat)
    if len(flabs) > 1:
        raise ValueError("ERROR: F_LABEL not unique from file list")
    elif len(flabs) == 0:
        raise ValueError("ERROR: F_LABEL not found")
    return flabs[0]

def define_head_names(ctx):

    import file_handler as fh

    """
    A common method to define head names per ccd base on the context
    using the information contained in catlist[FILEPATH_LOCAL]
    """

    # Short-cuts
    ext_me   = ctx.extension_me
    ext_head = fh.OHEAD_EXT


    # Get the f_label for file types
    flabel_cat = ctx.get('flabel_cat',extract_flabel(ctx.catlist['FILEPATH_LOCAL']))
    flabel_red = ctx.get('flabel_red','immasked')
    flabel_me  = "%s-%s" % (flabel_red,ext_me)

    # Build the path for inputs in TILEBUILDER
    # Get the dh object with file_handler struct
    dh = fh.get_tiledir_handler(ctx.tiledir)
    input_path = os.path.join(ctx.tiledir,dh.subdirs['inputs'])

    # Build the input heads in steps for clarity
    # 1. Strip the path to get the filename and add the tilename_fh to make it unique: TILENAME_FH_filename.fits
    input_head = ["%s_%s" % (ctx.tilename_fh,os.path.basename(f)) for f in ctx.catlist['FILEPATH_LOCAL']]

    # 2. Add the input path
    input_head = [os.path.join(input_path,f) for f in input_head]

    # 3. Remove the "fz" part:  .fits.fz --> .fits 
    input_head = [f.replace('.fits.fz','.fits') for f in input_head]
    
    # 4 Exchange F_LABELS: _red-fullcat.fits --> _immasked_me.head
    input_head = [f.replace(flabel_cat, flabel_me) for f in input_head]

    # 5. replace  .fits --> '.head'
    input_head = [f.replace('fits', ext_head) for f in input_head]
    return numpy.array(input_head)

def define_red_names(ctx):

    import file_handler as fh

    """
    A common method to define head names per ccd base on the context
    using the information contained in assoc[FILEPATH_LOCAL]
    """

    # TODO: Figure out what happend when inputs are fits.fz files instead of .fits
    
    # short-cuts for clarity
    ext_me   = ctx.extension_me
    
    # Build the path for inputs in TILEBUILDER
    # Get the dh object with file_handler struct
    dh = fh.get_tiledir_handler(ctx.tiledir)
    input_path = os.path.join(ctx.tiledir,dh.subdirs['inputs'])

    # Build the input heads in steps for clarity
    # 1. Strip the path to get the filename and add the tilename_fh to make it unique: TILENAME_FH_filename.fits
    filepath_input_red = ["%s_%s" % (ctx.tilename_fh,os.path.basename(f)) for f in ctx.assoc['FILEPATH_LOCAL']]

    # 2. Add the input path
    filepath_input_red = [os.path.join(input_path,f) for f in filepath_input_red]

    # 3. Remove the "fz" part:  .fits.fz --> .fits 
    filepath_input_red = [f.replace('.fits.fz','.fits') for f in filepath_input_red]
    
    # 4. replace  .fits --> -me.fits
    filepath_input_red = [f.replace('.fits','-%s.fits' % ext_me) for f in filepath_input_red]

    return  numpy.array(filepath_input_red)


def get_flabel(assoc, flabel='flabel_red',logger=None):
    """ Return the flabel with new context"""

    if logger: logger.info("Extracting the flabel: %s from information from assoc/catlist" % flabel)
    ctxext = {}
    ctxext[flabel] = extract_flabel(assoc['FILEPATH_LOCAL'])
    if logger: logger.info("Found {%s : %s}" % (flabel,ctxext[flabel]))
    return ctxext

def get_BANDS(assoc, detname='det', logger=None, doBANDS=['all']):

    """
    Generic function to set up the band from the context information
    into the context in case they are missing. This function defines
    how the BAND names are to be setup at every step into the context
    in case they are not present
    """

    # Avoid the Unicode 'u' in detname
    detname = detname.encode('ascii')

    if logger: logger.info("Extracting the BANDs information from assoc/catlist")
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


def get_scamp_expnums(ctx):
    import numpy
    """
    Get the unique set of expnums that for which we like to
    merge ccd-based red catalogs and use as inputs for scamp
    """
    expnums = []
    for BAND in ctx.doBANDS:
        expnums = expnums + numpy.unique(ctx.catlist['EXPNUM'][ctx.catlist['BAND'] == BAND]).tolist()
    return expnums

def get_ccd_catlist(catlist,expnum):

    """ Consistent method to extract ccd catlist from context per expnum"""
    ccd_catlist = catlist['FILEPATH_LOCAL'][catlist['EXPNUM'] == expnum]
    ccd_catlist.sort() # Make sure that they are sorted
    return ccd_catlist

def get_ccd_headlist(catlist,expnum):

    """ Consistent method to extract the ccd head list from context per expnum"""
    ccd_headlist = catlist['FILEPATH_INPUT_HEAD'][catlist['EXPNUM'] == expnum]
    ccd_headlist.sort() # Make sure that they are sorted
    return ccd_headlist



