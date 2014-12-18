"""
Builds a dictionary/header with all of the information on the COADDTILE_XXXX
table for a given TILENAME (input). It store the RAC[1,4] and DECC[1,4] corners
of the TILENAME an computes and stores the: (TILE_RACMIN, TILE_RACMAX) and
(TILE_DECCMIN,TILE_DECCMAX) for that tile.

We will use this dictionary to make the head file later.

-- We won't need the *.head file in the future --

"""


import numpy

from mojo.jobs.base_job import BaseJob

from multiepoch.exceptions import exceptions

from despydb import desdbi


QUERY = '''
    SELECT *
    FROM {tablename}
    WHERE tilename='{tilename}'
    '''

class Job(BaseJob):
    '''
    DESDM multi-epoch pipeline : QUERY TILEINFO JOB
    ===============================================
    This job queries the DESDM desoper database for information about a given
    tile and write this information into ctx.tileinfo.

    Required INPUT from context (ctx):
    ``````````````````````````````````
    - tilename : string, default_value=None
    - tablename : string, default_value='felipe.coaddtile_new'

    Writes as OUTPUT to context (ctx):
    ``````````````````````````````````
    - tileinfo : dictionary

    '''

    def get_query(self, **kwargs):
        '''
        Get the database query that returns DES tile information.

        kwargs
        ``````
            - tablename
            - tilename
        '''

        tablename = kwargs.get('coaddtile_table', None)
        tilename = kwargs.get('tilename', None)

        if not tablename or not tilename:
            raise ValueError('tablename and tilename need to be provided as kwargs')

        return QUERY.format(tablename=tablename, tilename=tilename)


    def __call__(self):

        # CHECK IF DATABASE HANDLER IS PRESENT
        if 'dbh' not in self.ctx:
            try:
                from despydb import desdbi
                self.ctx.dbh = desdbi.DesDbi(section='db-desoper')
            except:
                raise exceptions.NoDBHError(('Database handler could not be '
                    ' provided for context.'))
        # EXECUTE THE QUERY
        cur = self.ctx.dbh.cursor()
        cur.execute(self.get_query(**self.ctx.get_kwargs_dict()))
        desc = [d[0] for d in cur.description]
        # cols description
        line = cur.fetchone()
        cur.close()

        # Make a dictionary/header for the all columns from COADDTILE table
        # FIXME ?? return of query to COADDTILE dict in ctx, rest into tileinfo
        self.ctx.tileinfo = dict(zip(desc, line))

        # Lower-case for compatibility with wcsutils
        for k, v in self.ctx.tileinfo.items():
            self.ctx.tileinfo[k.lower()] = v

        # The minimum values for the tilename
        ras  = numpy.array([
            self.ctx.tileinfo['RAC1'], self.ctx.tileinfo['RAC2'],
            self.ctx.tileinfo['RAC3'], self.ctx.tileinfo['RAC4']
            ])
        decs = numpy.array([
            self.ctx.tileinfo['DECC1'], self.ctx.tileinfo['DECC2'],
            self.ctx.tileinfo['DECC3'], self.ctx.tileinfo['DECC4']
            ])

        ### TODO : add alls the following infered parameters to tileinfo?
        ### TODO @ Felipe : add TILE_RACMIN xxx into database schema for COADDTILE
        if self.ctx.tileinfo['CROSSRAZERO'] == 'Y':
            # Maybe we substract 360?
            self.ctx.tileinfo['TILE_RACMIN'] = ras.max()
            self.ctx.tileinfo['TILE_RACMAX'] = ras.min()
        else:
            self.ctx.tileinfo['TILE_RACMIN'] = ras.min()
            self.ctx.tileinfo['TILE_RACMAX'] = ras.max()
            
        self.ctx.tileinfo['TILE_DECCMIN'] = decs.min()
        self.ctx.tileinfo['TILE_DECCMAX'] = decs.max()

        # Store the packed corners of the COADDTILES for plotting later
        self.ctx.tileinfo['TILE_RACS'] = ras  #numpy.append(ras,ras[0])
        self.ctx.tileinfo['TILE_DECCS'] = decs #numpy.append(decs,decs[0])

        self.ctx.tile_edges = (
                self.ctx.tileinfo['TILE_RACMIN'], self.ctx.tileinfo['TILE_RACMAX'],
                self.ctx.tileinfo['TILE_DECCMIN'],self.ctx.tileinfo['TILE_DECCMAX']
                )
        
    def __str__(self):
        return 'query tileinfo'
