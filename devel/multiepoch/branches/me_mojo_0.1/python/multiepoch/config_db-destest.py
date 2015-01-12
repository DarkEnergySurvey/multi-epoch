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
        'multiepoch.jobs.query_tileinfo',
        'multiepoch.jobs.find_ccds_in_tile',
        'multiepoch.jobs.set_tile_directory',
        'multiepoch.jobs.plot_ccd_corners_destile',
        'multiepoch.jobs.find_fitsfiles_location',
        'multiepoch.jobs.get_fitsfiles',
        'multiepoch.jobs.make_SWarp_weights',
        #'multiepoch.jobs.call_SWarp',
        'multiepoch.jobs.call_SWarp_CustomWeights',
        'multiepoch.jobs.call_Stiff',
        'multiepoch.jobs.set_catNames',
        'multiepoch.jobs.call_SExpsf',
        'multiepoch.jobs.call_psfex',
        'multiepoch.jobs.call_SExDual',
        'multiepoch.jobs.make_MEFs',
        ]

