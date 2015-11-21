
"""
Set of utility functions used across different multi-epoch tasks
Felipe Menanteau, NCSA Jan 2015.

"""

import os

# Define order of HDU for MEF
SCI_HDU = 0
MSK_HDU = 1
WGT_HDU = 2

def read_tileinfo(geomfile,logger=None):
    import json
    mess = "Reading the tile Geometry from file: %s" % geomfile
    if logger: logger.info(mess)
    else: print mess
    with open(geomfile, 'rb') as fp:
        json_dict = json.load(fp)
    return json_dict


# Check if database handle is in the context
def check_dbh(ctx, logger=None):

    from despydb import desdbi
    import os

    """ Check if we have a valid database handle (dbh)"""
    
    if 'dbh' not in ctx:
        try:
            db_section = ctx.get('db_section')
            mess = "Creating db-handle to section: %s" % db_section
            if logger: logger.info(mess)
            else: print mess
            try:
                desservicesfile = ctx.get('desservicesfile',
                                          os.path.join(os.environ['HOME'],'.desservices.ini'))
                ctx.dbh = desdbi.DesDbi(desservicesfile, section=db_section)
            except:
                mess = "Cannot find des service file -- will try none"
                if logger: logger.warning(mess)
                else: print mess
                ctx.dbh = desdbi.DesDbi(section=db_section)
        except:
            raise
    else:
        mess = "Will recycle existing db-handle"
        if logger: logger.debug(mess)
        else: print mess
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

def create_local_archive(local_archive,logger=None):
    
    import os
    """ Creates the local cache directory for the desar archive data to be transfered"""
    if not os.path.exists(local_archive):
        message = "Will create LOCAL ARCHIVE at %s" % local_archive
        if logger: logger.info(message)
        else: print message
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


def parse_comma_separated_list(inputlist):

    if inputlist[0].find(',') >= 0:
        return inputlist[0].split(',')
    else:
        return inputlist

def inDESARcluster(domain_name='cosmology.illinois.edu',logger=None,verb=False):

    import os,re
    """ Figure out if we are in the cosmology.illinois.edu cluster """
    
    uname    = os.uname()[0]
    hostname = os.uname()[1]
    mach     = os.uname()[4]
    
    pattern = r"%s$" % domain_name
        
    if re.search(pattern, hostname) and uname == 'Linux':
        LOCAL = True
        message = "Found hostname: %s, running:%s --> in %s cluster." % (hostname, uname, domain_name)
    else:
        LOCAL = False
        message = "Found hostname: %s, running:%s --> NOT in %s cluster." % (hostname, uname, domain_name)
                
    if logger: logger.debug(message)
    else:
        if verb: print message

    return LOCAL


def check_filepath_exist(filepath,logger=None):

    import os

    if not os.path.isdir(filepath):
        mess = "Filepath: %s is not a directory" % os.path.split(filepath)[0]
        if logger: logger(mess)
        return

    if not os.path.exists(os.path.split(filepath)[0]):
        mess = "Making: %s" % os.path.split(filepath)[0]
        os.makedirs(os.path.split(filepath)[0])
    else:
        mess = "Filepath: %s already exists" % os.path.split(filepath)[0]

    if logger: logger(mess)
    return

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
    #print "# Will write to logfile: %s" % logfile
    status = subprocess.call(cmd,shell=True ,stdout=log, stderr=log)
    if status > 0:
        raise RuntimeError("\n***\nERROR while running, check logfile: %s\n***" % logfile)
    return status


def checkTABLENAMEexists(tablename,dbh=None,db_section=None,verb=False,logger=None):

    from despydb import desdbi

    """
    Check if exists. Tablename has to be a full owner.table_name format
    """

    mess = "Checking if %s exists." % tablename
    if logger: logger.info(mess)
    elif verb: print mess
    
    # Make sure is all upper case
    tablename = tablename.upper()

    query = """
    select count (*) from all_tables where table_name='%s'""" % tablename

    # Get a dbh if not provided
    if not dbh:
        dbh = desdbi.DesDbi(section=db_section)
        
    cur = dbh.cursor()
    cur.execute(query)
    count = cur.fetchone()[0]
    cur.close()
    
    if count >= 1:
        table_exists = True
    else:
        table_exists = False
    mess = "%s exists: %s " % (tablename,table_exists)
    if logger: logger.info(mess)
    elif verb: print mess

    return table_exists


def grant_read_permission(tablename,dbh, roles=['DES_READER','PROD_ROLE','PROD_READER_ROLE']):

    # Grand permission to a table
    cur = dbh.cursor()
    for role in roles:
        grant = "grant select on %s to %s" % (tablename,role)
        print "# Granting permission: %s" % grant
        cur.execute(grant)
    dbh.commit()
    cur.close()
    return


# Pass mess to debug logger or print
def pass_logger_debug(mess,logger=None):
    if logger: logger.debug(mess)
    else: print mess
    return

# Pass mess to info logger or print
def pass_logger_info(mess,logger=None):
    if logger: logger.info(mess)
    else: print mess
    return


# Update RAs when crosssing RA=0
def update_tileinfo_RAZERO(tileinfo):
    keys = ['RA','RAC1','RAC2','RAC3','RAC4','RACMIN','RACMAX']
    # We move the tile to RA=-180/+180
    if tileinfo['CROSSRAZERO'] == 'Y':
        for key in keys:
            if tileinfo[key] > 180: tileinfo[key] -= 360 
            #else: tileinfo[key] += 180 
    return tileinfo

def update_CCDS_RAZERO(CCDS,crossRAzero=False):
    import numpy
    keys = ['RA_CENT','RAC1','RAC2','RAC3','RAC4']
    # We move the tile to RA=180
    if crossRAzero == 'Y':
        for key in keys:
            CCDS[key] = numpy.where( CCDS[key] > 180, CCDS[key] - 360,  CCDS[key])
    return CCDS


