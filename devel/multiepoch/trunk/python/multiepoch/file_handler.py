
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
PSF_TYPE = 'psfcat'

FITS_EXT = 'fits'
LIST_EXT = 'list'
PSF_EXT  = 'psf'
XML_EXT  = 'xml'

# GENERIC MULTIEPOCH FILENAME GENERATOR 

_GENERIC_FILENAMEPATTERN = "{base}_{band}_{ftype}.{ext}"

def _me_fn(**kwargs):
    ''' the generic multiepoch filename generator '''
    return _GENERIC_FILENAMEPATTERN.format(**kwargs)


# FILENAME GENERATOR FUNCTIONS

#  ***** SWARP FILES *****
# -----------------------------------
# 1. Input List Names (sci/wgt/swg)
# -----------------------------------
def get_sci_list_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':SCI_TYPE, 'ext':LIST_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'aux') 

def get_wgt_list_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':WGT_TYPE, 'ext':LIST_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'aux')

def get_swg_list_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':SWG_TYPE, 'ext':LIST_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'aux')

# ------------------------------------
# 2. Output Coadd Files (sci/wgt/swg)
# ------------------------------------
def get_sci_fits_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':SCI_TYPE, 'ext':FITS_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

def get_wgt_fits_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':WGT_TYPE, 'ext':FITS_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

def get_swg_fits_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':SWG_TYPE, 'ext':FITS_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

# ----------------------------------------------------------------------------
# 2a. Outout Coadd different type (tmp_wgt/tmp_sci)
# ----------------------------------------------------------------------------
def get_WGT_fits_file(tiledir, tilename, band, type=WGT_TYPE):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':type, 'ext':FITS_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

def get_SCI_fits_file(tiledir, tilename, band, type=SCI_TYPE):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':type, 'ext':FITS_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

# ------------------------------------
# 3. Misc files (log/fluxes/command)
# ------------------------------------
def get_flx_list_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':FLX_TYPE, 'ext':LIST_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'aux')

def get_swarp_log_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    filename = "%s_swarp.log" % tilename
    return dh.place_file(filename, 'log')

def get_swarp_cmd_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    filename = "%s_call_swarp.cmd" % tilename
    return dh.place_file(filename, 'aux')

# -----------------------------------------------------------------------------
#    ******* STIFF FILES ********
def get_stiff_cmd_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    filename = "%s_call_stiff.cmd" % tilename
    return dh.place_file(filename, 'aux')

def get_stiff_log_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    filename = "%s_stiff.log" % tilename
    return dh.place_file(filename, 'log')

def get_color_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    filename = "%s.tiff" % tilename
    return dh.place_file(filename, 'products')


# -----------------------------------------------------------------------------
#     ***** SEX PSF FILES *****
def get_psf_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':PSF_TYPE, 'ext':PSF_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

def get_psfcat_file(tiledir, tilename, band):
    dh = get_tiledir_handler(tiledir)
    fnkwargs = {'base':tilename, 'band':band, 'ftype':PSF_TYPE, 'ext':FITS_EXT}
    return dh.place_file(_me_fn(**fnkwargs), 'products')

def get_sexpsf_cmd_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    filename = "%s_call_SExpsf.cmd" % tilename
    return dh.place_file(filename, 'aux')

def get_sexpsf_log_file(tiledir, tilename,band=None):
    dh = get_tiledir_handler(tiledir)
    if band:
        filename = "%s_SExpsf.log" % tilename
    else:
        filename = "%s_%s_SExpsf.log" % (tilename,band)
    return dh.place_file(filename, 'log')


def get_ccd_plot_file(tiledir, tilename):
    dh = get_tiledir_handler(tiledir)
    filename = "%s_overlap.pdf" % tilename
    return dh.place_file(filename, 'aux')

