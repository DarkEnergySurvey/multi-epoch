'''
to run call_SWarp.py

you can execute from the commandline in trunk

$ call_SWarp.py testing/DES2246-4457_ccdinfo_local.json testing/DES2246-4457_tileinfo.json --basename blabla --swarp_execution_mode tofile --stdoutloglevel INFO

alternatively you can use mojo and this config file, like so

$ mojo run_config multiepoch.config.call_SWarp_config

from the same location or replace input file arguments with absolute paths to run from any location

'''

stdoutloglevel = 'INFO'

assoc_file = 'testing/DES2246-4457_ccdinfo_local.json'
tile_geom_input_file = 'testing/DES2246-4457_tileinfo.json'

basename = 'blabla'

swarp_execution_mode = 'tofile'

jobs = [
        'multiepoch.tasks.call_SWarp',
        ]
