'''
DES MULTIEPOCH configration file.

'''

# JOB SPECS
# -----------------------------------------------------------------------------

# query_tileinfo
tilename = 'DES2246-4457'



# JOBS TO RUN
# -----------------------------------------------------------------------------

jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.find_ccds_in_tile',
        ]


# RUN CONFIG 
# -----------------------------------------------------------------------------

# where to write the ctx at the end of the run
json_dump_file = 'two_task_test.json'

# logging
stdoutloglevel = 'DEBUG'
fileloglevel = 'DEBUG'
logfile = tilename+'_mojo.log'
