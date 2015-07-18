
from mojo.utils import directory_handler

# DIRECTORIES
# -----------------------------------------------------------------------------
# - tiledir 
#   |+ aux_dir 
#   |+ ctx_dir 
#   |+ log_dir 
#   |+ products_dir 
#   `- ..

TILEDIR_SUBDIRECTORIES = {
        'aux': 'aux',
        'ctx': 'ctx',
        'log': 'log',
        'products': 'products',
        }

def get_tiledir_handler(tiledir, logger=None):
    ''' Provides a DirectoryHandler for the tile directory
    '''
    dh = directory_handler.DirectoryHandler([tiledir,],
            subdirs=TILEDIR_SUBDIRECTORIES, logger=logger,)
    return dh


# FILENAMES 
# -----------------------------------------------------------------------------
SCI_TYPE = 'sci'
WGT_TYPE = 'wgt'
FLX_TYPE = 'flx'
MEF_TYPE = 'mef'
SWG_TYPE = 'swg'

FITS_EXT = 'fits'
LIST_EXT = 'list'


# GENERIC MULTIEPOCH FILENAME GENERATOR 

_GENERIC_FILENAMEPATTERN = "{base}_{band}_{ftype}.{ext}"

def _me_fn(**kwargs):
    ''' the generic multiepoch filename generator '''
    return _GENERIC_FILENAMEPATTERN.format(**kwargs)


# FILENAME GENERATOR FUNCTIONS

def get_sci_list_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':SCI_TYPE, 'ext':LIST_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'aux') 

def get_sci_fits_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':SCI_TYPE, 'ext':FITS_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

def get_wgt_list_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':WGT_TYPE, 'ext':LIST_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'aux')

def get_wgt_fits_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':WGT_TYPE, 'ext':FITS_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

def get_flx_list_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':FLX_TYPE, 'ext':LIST_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'aux')

def get_swarp_cmd_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    # TODO : do we want this to be constructed the same way like the other
    # filenames
    filename = "%s_call_swarp.cmd" % tilename
    return dh.place_file(filename, 'products')

def get_swarp_log_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    # TODO : do we want this to be constructed the same way like the other
    # filenames
    filename = "%s_swarp.log" % tilename
    return dh.place_file(filename, 'log')

def get_ccd_plot_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    # TODO : do we want this to be constructed the same way like the other
    # filenames
    filename = "%s_overlap.pdf" % tilename
    return dh.place_file(filename, 'aux')
