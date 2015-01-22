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
        ]
