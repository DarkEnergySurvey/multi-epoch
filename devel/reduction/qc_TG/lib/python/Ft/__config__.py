# Configuration variables
NAME     = '4Suite-XML'
VERSION  = '1.0.2'
FULLNAME = '4Suite-XML-1.0.2'
URL      = 'http://4suite.org/'

import sys
if getattr(sys, 'frozen', False):
    # "bundled" installation locations (e.g., py2exe, cx_Freeze)
    RESOURCEBUNDLE = True
    PYTHONLIBDIR   = '/'
    BINDIR         = None
    DATADIR        = '/Share'
    SYSCONFDIR     = None
    LOCALSTATEDIR  = None
    LIBDIR         = None
    LOCALEDIR      = '/Share/Locale'
else:
    # standard distutils installation directories
    RESOURCEBUNDLE = False
    PYTHONLIBDIR   = sys.prefix+'/lib/python/'
    BINDIR         = sys.prefix+'/bin'
    DATADIR        = sys.prefix+'/share/4Suite'
    SYSCONFDIR     = sys.prefix+'/share/etc/4Suite'
    LOCALSTATEDIR  = sys.prefix+'/share/var/4Suite'
    LIBDIR         = sys.prefix+'/lib/4Suite'
    LOCALEDIR      = sys.prefix+'/share/locale'
del sys
