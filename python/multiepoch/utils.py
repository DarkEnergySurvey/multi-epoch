
"""
Set of utility functions used across different multi-epoch tasks
Felipe Menanteau, NCSA Jan 2015.

"""

# Check if database handle is in the context
def check_dbh(ctx):

    from despydb import desdbi

    """ Check if we have a valid database handle (dbh)"""
    
    if 'dbh' not in ctx:
        try:
            db_section = ctx.get('db_section')
            desservicesfile = ctx.get('desservicesfile',
                    os.path.join(os.environ['HOME'], '.desservices.in'))
            print "# Creating db-handle to section: %s" % db_section
            ctx.dbh = desdbi.DesDbi(desservicesfile, section=db_section)
        except:
            # you want to see the true cause of the error here
            raise
            #raise ValueError('ERROR: Database handler could not be provided for context.')
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
    """ Creates the local cache directory for the desar archive data to be transfered"""
    if not os.path.exists(local_archive):
        print "# Will create LOCAL ARCHIVE at %s" % local_archive
        os.mkdir(local_archive)
    return

def dict2arrays(dictionary):
    """
    Re-cast list in contained in a dictionary as numpy arrays
    """
    import numpy
    for key, value in dictionary.iteritems():
        if isinstance(value, list):
            dictionary[key] = numpy.array(value)
    return dictionary


def arglist2dict(inputlist,separator='='):
    """
    Re-shape a list of items ['VAL1=value1', 'VAL2=value2', etc]  into a dictionary
    dict['VAL1'] = value1, dict['VAL2'] = values, etc
    This is used to pass optional command-line argument option to the astromatic codes.
    
    We Re-pack as a dictionary the astromatic extras fron the command-line, if run as script
    """
    return dict( [ inputlist[index].split(separator) for index, item in enumerate(inputlist) ] )


def inDESARcluster(domain_name='cosmology.illinois.edu'):

    import os,re
    """ Figure out if we are in the cosmology.illinois.edu cluster """
    
    uname    = os.uname()[0]
    hostname = os.uname()[1]
    mach     = os.uname()[4]
    
    pattern = r"%s$" % domain_name
        
    if re.search(pattern, hostname) and uname == 'Linux':
        LOCAL = True
        print "# Found hostname: %s, running:%s" % (hostname,uname)
        print "# In %s cluster -- will NOT transfer files" % domain_name
    else:
        LOCAL = False
        print "# Found hostname: %s, running:%s" % (hostname,uname)
        print "# NOT in %s cluster -- will try to transfer files" % domain_name

    return LOCAL


"""
A collection of utilities to call subprocess from multiprocess in python.
F. Menanteau, NCSA, Dec 2014
"""

def work_subprocess(cmd):

    import subprocess
    """ Dummy function to call in multiprocess with shell=True """
    return subprocess.call(cmd,shell=True) 

def work_subprocess_logging(tup):

    import subprocess
    """
    Dummy function to call in multiprocess with shell=True and a
    logfile using a tuple as inputs
    """
    cmd,logfile = tup
    log = open(logfile,"w")
    print "# Will write to logfile: %s" % logfile

    status = subprocess.call(cmd,shell=True ,stdout=log, stderr=log)
    if status > 0:
        raise RuntimeError("\n***\nERROR while running SExpsf, check logfile: %s\n***" % logfile)
    return status
