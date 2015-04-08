'''
to run call_SWarp.py

you can execute from the commandline in trunk

$ call_SWarp.py testing/DES2246-4457_ccdinfo_local.json testing/DES2246-4457_tileinfo.json --basename blabla --swarp_execution_mode tofile --stdoutloglevel INFO

alternatively you can use mojo and this config file, like so

$ mojo run_config multiepoch.config.call_SWarp_config

from the same location or replace input file arguments with absolute paths to run from any location

'''

tilename = 'blabla'

stdoutloglevel = 'INFO'

#tilename = 'DES2246-4457' 

assoc_file = '/Users/michael/coding/FHNW/des/svn/multiepoch/trunk/testing/DES2246-4457_ccdinfo_local.json'
tile_geom_input_file = '/Users/michael/coding/FHNW/des/svn/multiepoch/trunk/testing/DES2246-4457_tileinfo.json'

swarp_execution_mode = 'tofile'

jobs = [
        #'multiepoch.tasks.query_database',
        'multiepoch.tasks.call_SWarp_michael_dev',
        ]
