#!/usr/bin/env python

QUERY_TILE_RUNS = '''
  SELECT DISTINCT location.run, location.tilename,tag
  FROM location, runtag
  WHERE
    tag like '{tag}' AND
    location.run = runtag.run AND
    location.tilename = '{tilename}'
    ORDER by tilename
    '''

QUERY_INPUT_FOR_RUN = '''
        SELECT id,exposurename,nite,band
        FROM exposure
        WHERE id in (
            SELECT exposureid
            FROM image WHERE id in (
                    SELECT distinct src_imageid
                    FROM coadd_src WHERE coadd_imageid in (
                        SELECT distinct id
                        FROM coadd WHERE run='{run}' AND imagename not like '%%_det.fits'
            ))
        )
        ORDER by band
        '''
def get_tile_runs(dbh,tilename,tag='Y1A1_COADD'):
    query = QUERY_TILE_RUNS.format(tag=tag,tilename=tilename)
    return despyastro.genutil.query2rec(query, dbhandle=dbh)

def get_inputs_for_run(dbh,run):
    query = QUERY_INPUT_FOR_RUN.format(run=run)
    return despyastro.genutil.query2rec(query, dbhandle=dbh)

def get_inputs_for_tilename(dbh,tilename,tag='Y1A1_COADD'):
    tile_runs = get_tile_runs(dbh,tilename,tag)

    print "# TILENAME: %s" % tilename
    for run in tile_runs['RUN']:
        inputs = get_inputs_for_run(dbh,run)
        N = len(inputs)
        n = len(inputs.dtype.names)
        print "%18s "*n % inputs.dtype.names
        # Now we do pprint
        for k in range(N):
            print "%18s "*n % tuple(inputs[k])
    
if __name__ == '__main__':

    from despydb import desdbi
    import despyastro
    import argparse
    parser = argparse.ArgumentParser(description="Finds the inputs for a TILENAME and TAG in the Old Schema\nfind_exposuresintile.py DES2354+0043")
    # The positional arguments
    parser.add_argument("tilename", action="store",default=None,
                        help="Name of the TILENAME")
    parser.add_argument("--db_section", action="store", default='db-desoper',choices=['db-desoper','db-destest'],
                        help="DB Section to query")
    parser.add_argument("--tag", action="store", default='Y1A1_COADD',
                        help="database release TAG (i.e. Y1A1_COADD")
    args = parser.parse_args()

    # Get a handle and recycle it later
    dbh = desdbi.DesDbi(section=args.db_section)
    # Get the inputs and print
    get_inputs_for_tilename(dbh,args.tilename,tag=args.tag)

