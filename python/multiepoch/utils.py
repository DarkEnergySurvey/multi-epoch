
"""
Set of utility functions used across different multi-epoch tasks
"""

# Check if database handle is in the context
def check_dbh(ctx):

    from despydb import desdbi

    """ Check if we have a valid database handle (dbh)"""
    
    if 'dbh' not in ctx:
        try:
            db_section = ctx.get('db_section','db-desoper')
            print "# Creating db-handle to section: %s" % db_section
            ctx.dbh = desdbi.DesDbi(section=db_section)
        except:
            raise ValueError('ERROR: Database handler could not be provided for context.')
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


