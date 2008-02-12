# Configuration variables
NAME     = '4Suite-XML'
VERSION  = '1.0.2'
FULLNAME = '4Suite-XML-1.0.2'
URL      = 'http://4suite.org/'

import sys
import os
qchome=''
try:
	qchome = os.environ['QC_HOME']
except:
	pass

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
    PYTHONLIBDIR   = qchome+'/lib64/python/'
    BINDIR         = qchome+'/bin'
    DATADIR        = qchome+'/share/4Suite'
    SYSCONFDIR     = qchome+'/share/etc/4Suite'
    LOCALSTATEDIR  = qchome+'/share/var/4Suite'
    LIBDIR         = qchome+'/lib/4Suite'
    LOCALEDIR      = qchome+'/share/locale'
del sys
