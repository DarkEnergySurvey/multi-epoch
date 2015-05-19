

from mojo.utils import directory_handler


# GENERIC MULTIEPOCH DIRECTORY HANDLER
# -----------------------------------------------------------------------------

def get_tiledir_handler(tiledir, logger=None):
    ''' Provides a DirectoryHandler for the tile directory
    '''
    dh = directory_handler.DirectoryHandler([tiledir,],
            subdirs = {
                'aux': 'aux',
                'log': 'log',
                'ctx': 'ctx',
                'products': 'products',
                },
            logger=logger,
            )

    return dh


# GENERIC MULTIEPOCH FILENAME HANDLER
# -----------------------------------------------------------------------------

GENERIC_FILENAMEPATTERN = "{base}_{band}_{ftype}.{ext}"

def me_filename(**kwargs):
    return GENERIC_FILENAMEPATTERN.format(**kwargs)
