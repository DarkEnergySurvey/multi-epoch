#!/usr/bin/env python
help=\
"""
measure_stars.py [options] imagefile catfile 

INPUTS:
    image_file: The image fits file.
    cat_file: The catalog input fits file.

    These files are output with image_file-allout.csv and 
       image_file-starout.csv
    allout: The output csv file containing a row for every
        object in the catalog file. Each row will contain
               id, flags, mag_auto, sigma, shapelet_coefficients
        where id is the SExtractor id, mag_auto is the SExtractor
        mag_auto, flags are the processing flags, and sigma is the
        size. The order of the shapelet decomposition is set by the
            firstpass_order
    starout:  The output csv file for the PSF stars.  Each row
        contains the same as the allout file but with different
        possible number of coeffs:
            id, flags, mag_auto, sigma, shapelet_coefficients
        The order of the shapelet decomposition is set by the
            secondpass_order
        parameter in the MeasureStars config file.

OPTIONS:
    -m msconf: The MeasureStars config file.  If not sent, then
        the environment variable DES_CONFIG_DIR must be set and the
        file MeasureStars.conf must be within that directory.  See
        the config/MeasureStars.conf file for an example config file. 
    -f fsconf: The FindStars config file.  If not sent, then
        the environment variable DES_CONFIG_DIR must be set and the
        file FindStars.conf must be within that directory.  See
        the config/MeasureStars.conf file for an example config file. 

    These are no longer supported, but rather the names are generated
    internally as image_file-sizemag.eps and image_file-measure-stars-exit.log
    -p plotfile: A plot file to contain the size-magnitude diagram for
        The input catalog/image. Stars are overplotted in red. a log file from
        gnuplot is created same nameas the file with .log appended.
    -e exitlog: A log file to contain the exit status and optional
        description. It will contain one line
               exitcode some description

        EXIT CODES:
            0 is success
            READERROR 35
            FORMATERROR 36
            SYNTAXERROR 37
            RANGEERROR 38 
            CATALOGERROR 39 
            PARAMETERERROR 40
            ALGORITHMERROR 41 

External Dependencies:
    python: version 2.1 or later
    gnuplot: I don't know enough about gnuplot to know the required version.

Revision History:
    Created: 2007-10, Erin Sheldon, NYU
    All output files and log names created and written to internally.
"""
import os
import re
import sys

from getopt import getopt

errors = {}
errors[0] = 'SUCCESS'
errors[35] = 'READERROR'
errors[36] = 'FORMATERROR'
errors[37] = 'SYNTAXERROR'
errors[38] = 'RANGEERROR'
errors[39] = 'CATALOGERROR'
errors[40] = 'PARAMETERERROR'
errors[41] = 'ALGORITHMERROR'

starpost  = '_starsh.csv'

# On some systems the exit code returned by os.system is in 
# a position bit-shifted by 8
bitshift = 8


def PlotSizeMag(allfile, psffile, plotfile):
    print 'Plotting size-mag diagram to file:',plotfile
    gp_commands = """
        set terminal postscript eps enhanced color "Courier" 16;
        set datafile separator ",";
        set xrange[5:18];
        set yrange[0:8];
        set mxtics 4;
        set mytics 4;
        set xlabel "mag auto";
        set ylabel "sigma";

        set output "%s";
        plot "%s" using 2:4 title "All" with points lt -1, "%s" using 2:4 title "Stars" with points lt 1;
        """ % (plotfile, allfile, psffile)

    logfile = plotfile+'.log'
    comm = "echo '"+gp_commands+"' | gnuplot &> "+logfile
    res = os.system(comm)

def ProcessOpts(options):
    """
    Use ancient style options returned by getopt. All that is available
    in python 2.1 in TeraGrid
    """
    out = {'msconf':'', 'fsconf':'', 'plotfile':'', 'exitlog':''}
    for opt in options:
        if opt[0] == '-m':
            out['msconf'] = opt[1]
        elif opt[0] == '-f':
            out['fsconf'] = opt[1]
        elif opt[0] == '-p':
            out['plotfile'] = opt[1]
        elif opt[0] == '-e':
            out['exitlog'] = opt[1]
    return out

def mainold(argv):
    # process the options and arguments. Have to use ancient getopt style in
    # ancient python 2.1 on TeraGrid
    options, args = getopt(argv[1:], 'm:f:p:e:')
    if len(args) < 4:
        print help
        sys.exit(45)

    imfile = args[0]
    catfile = args[1]
    allout = args[2]
    starout = args[3]

    odict = ProcessOpts(options)
    msconf = odict['msconf']
    fsconf = odict['fsconf']
    plotfile = odict['plotfile']
    exitlog = odict['exitlog']

    execdir = os.path.dirname(argv[0])
    comm = os.path.join(execdir, 'measure_stars')
    if msconf != '':
        comm = comm + ' -m '+msconf
    if fsconf != '':
        comm = comm + ' -f '+fsconf
    
    comm = comm + " %s %s %s %s" % (imfile, catfile, allout, starout)

    print "Executing command:",comm

    sys.stdout.flush()
    res = os.system(comm)
    # on some systems it is bit-shifted
    if res > 100:
        print 'This system is probably returning bit-shifted exit codes. Old value:',res
        res = res >> bitshift

    print 'Exit status status:',res
    if exitlog != '':
        print 'Writing measure_stars exit status to file:',exitlog
        exit_file = open(exitlog, 'w')
        if errors.has_key(res):
            exit_str = str(res)+' '+errors[res]+'\n'
        else:
            exit_str = '9999 Unknown Error\n'
        exit_file.write(exit_str) 
        exit_file.close()

    if res == 0 and plotfile != '':
        PlotSizeMag(allout, starout, plotfile)

def main(argv):
    # process the options and arguments. Have to use ancient getopt style in
    # ancient python 2.1 on TeraGrid
    options, args = getopt(argv[1:], 'm:f:p:e:')
    if len(args) < 2:
        print help
        sys.exit(45)

    imfile = args[0]
    catfile = args[1]

    tmp = imfile.split('.')
    if len(tmp) > 1:
        front   = '.'.join(tmp[0:-1])
    else:
        front =imfile

    allout   = front+'-allout.csv'
    starout  = front+'-starout.csv'
    logfile  = front+'-measure-stars.log'
    exitlog  = front+'-measure-stars-exit.log'
    plotfile = front+'-sizemag.eps'

    odict = ProcessOpts(options)
    msconf = odict['msconf']
    fsconf = odict['fsconf']

    execdir = os.path.dirname(argv[0])
    comm = os.path.join(execdir, 'measure_stars')
    if msconf != '':
        comm = comm + ' -m '+msconf
    if fsconf != '':
        comm = comm + ' -f '+fsconf
    
    comm = comm + " %s %s %s %s" % (imfile, catfile, allout, starout)
    comm = comm + ' &> '+logfile

    #print "Executing command:",comm

    res = os.system(comm)

    # Now append stdout/stderr to the same log file
    sys.stdout = open(logfile, 'a')
    sys.stderr = sys.stdout

    print 'Checking exit status'

    # on some systems it is bit-shifted
    if res > 100:
        print 'This system is probably returning bit-shifted exit codes. Old value:',res
        res = res >> bitshift

    print 'Exit status status:',res
    if exitlog != '':
        print 'Writing measure_stars exit status to file:',exitlog
        exit_file = open(exitlog, 'w')
        if errors.has_key(res):
            exit_str = str(res)+' '+errors[res]+'\n'
        else:
            exit_str = '9999 Unknown Error\n'
        exit_file.write(exit_str) 
        exit_file.close()

    if res == 0 and plotfile != '':
        PlotSizeMag(allout, starout, plotfile)


if __name__=="__main__":
    main(sys.argv)
