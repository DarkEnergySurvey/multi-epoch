'''
DES MULTIEPOCH configration file.


'''

# USE ONE DATABASE HANDLER FOR THE ENTIRE PIPELINE
# -----------------------------------------------------------------------------

try:
    from despydb import desdbi
    #dbh = desdbi.DesDbi(section='db-desoper')
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
tilename = 'DES2246-4457'
tablename = 'felipe.coaddtile_new'

# find_ccds_in_tile
## optional parameters
select_extras = "felipe.extraZEROPOINT.IMAGENAME, felipe.extraZEROPOINT.MAG_ZERO,"
from_extras   = "felipe.extraZEROPOINT"
and_extras    = """image.run IN ('20140421093250_20121207','20140421090850_20121124')
                    AND image.IMAGETYPE='red'"""

and_extras = and_extras + " AND felipe.extraZEROPOINT.IMAGENAME = image.IMAGENAME"


#outputpath = '/path/to/my/output//'
#archive_root = '/home/felipe/work/NCSA_posters_images'
#prefix = 'imskTR_'

# TODO : UNIVERSAL PIPELINE PARAMETER vs ARGUMENTS TO TASKS /
# PARAMETERS


# THE JOBS
# -----------------------------------------------------------------------------

jobs = [
        'multiepoch.jobs.query_tileinfo',
        'multiepoch.jobs.find_ccds_in_tile',
        'multiepoch.jobs.setup_tile_directory',
#       'multiepoch.jobs.plot_ccd_corners_destile',
#       'multiepoch.jobs.collect_files_for_swarp',
        ]


'''

define tile:
    - user defined : provide center (ra, dec) + x&y size + pixelsize
    - des database defined : goes into ctx.COADDTILE
    : defines ctx.tile_edges! rest goes into COADDTILE


find ccds in tile:
    - needs ctx.tile_edges 
    - needs ctx.dbh
    : provides ctx.CCDS -> dict of arrays with key 'PATH'll
    : provides ctx.ccdinfo with 'BANDS' & 'NBANDS'

    TODO  archive_root ??


'''
