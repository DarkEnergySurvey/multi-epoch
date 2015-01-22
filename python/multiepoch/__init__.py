__author__  = "Felipe Menanteau"
__version__ = '0.1'
version = __version__


"""
The DESDM multiepoch image coadd pipeline
"""


from . import coaddlib
from . import descoords
from . import destiling

# To make the classes visible outside, directly as:
# multiepoch.DEStiling,  multiepoch.DEScoords, etc
from .destiling import DEStiling
from .descoords import DEScoords, DEScoordsOLD
from .coaddlib  import DEScoadd

# Load some functions
#from .mutiepoch_utils import some_functions
