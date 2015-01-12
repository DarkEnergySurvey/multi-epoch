'''
DES MULTIEPOCH configration file.


'''

# USE ONE DATABASE HANDLER FOR THE ENTIRE PIPELINE
# -----------------------------------------------------------------------------

try:
    from despydb import desdbi
    dbh = desdbi.DesDbi(section='db-destest')
except:
    print 'no connection to the database.'


# RUN PARAMETERS
# -----------------------------------------------------------------------------

verbose = True


# JOB SPECS
# -----------------------------------------------------------------------------

# query_tileinfo
## required parameters
tilename  = 'DES2246-4457'
tablename = 'felipe.coaddtile_new'

# THE JOBS
# -----------------------------------------------------------------------------

jobs = [
        'multiepoch.tasks.query_tileinfo',
        'multiepoch.tasks.find_ccds_in_tile',
        'multiepoch.tasks.set_tile_directory',
        'multiepoch.tasks.plot_ccd_corners_destile',
        'multiepoch.tasks.find_fitsfiles_location',
        'multiepoch.tasks.get_fitsfiles',
        'multiepoch.tasks.make_SWarp_weights',
        #'multiepoch.tasks.call_SWarp',
        'multiepoch.tasks.call_SWarp_CustomWeights',
        'multiepoch.tasks.call_Stiff',
        'multiepoch.tasks.set_catNames',
        'multiepoch.tasks.call_SExpsf',
        'multiepoch.tasks.call_psfex',
        'multiepoch.tasks.call_SExDual',
        'multiepoch.tasks.make_MEFs',
        ]

