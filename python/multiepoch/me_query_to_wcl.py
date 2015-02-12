#!/usr/bin/env python
'''
This script is used to return a list of dicts containing the ME association
information as gathered by the multiepoch pipeline over multiple queries to the
wcl based production framework.
'''

import processingfw.pfwfilelist as filelistutils

from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat,\
        CInt, Instance
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError

from multiepoch.tasks import query_database

class Job(query_database.Job):

    class Input(IO):
        """
        Collect the tile geometry information using the DESDM Database
        """

        # We need to redo the input class for some of the input arguments are
        # NOT REQUIRED in this case, argparse can aparently not drop arguments.
        # also, we do add another argument: qoutfile

        
        # Required inputs
        tilename   = CUnicode(None, help="The Name of the Tile Name to query", 
                        argparse={'argtype': 'positional', } )

        # Optional inputs
        db_section = CUnicode("db-destest", help="DataBase Section to connect",
                        argparse={'choices': ('db-desoper','db-destest')} )
        coaddtile_table    = CUnicode("felipe.coaddtile_new",
                                help="Database table with COADDTILE information",)
        archive_name       = CUnicode("desar2home",
                                help="DataBase Archive Name section",)
        select_extras      = CUnicode("felipe.extraZEROPOINT.MAG_ZERO,",
                                help="string with extra SELECT for query",)
        and_extras         = CUnicode("felipe.extraZEROPOINT.FILENAME=image.FILENAME",
                                help="string with extra AND for query",)
        from_extras        = CUnicode("felipe.extraZEROPOINT",
                                help="string with extra FROM for query",)
        tagname            = CUnicode('Y2T1_FIRSTCUT',
                                help="TAGNAME for images in the database",)
        exec_name          = CUnicode('immask',
                                help="EXEC_NAME for images in the database",)
        filepath_local     = CUnicode("",
                                help=("The local filepath where the input "
                                      "fits files (will) live"))
        qoutfile           = CUnicode("", help='name of query output file',
                                argparse={'required':True},)


    def write_assoc_json(self):
        raise NotImplementedError('This method cannot be used in this class.')

    def get_assoc_list_of_dicts(self,
            fields=('FILEPATH_ARCHIVE','FILENAME','BAND','MAG_ZERO')):
        ''' get a list of dictionaries containing the assoc information '''
        assoc = self.ctx.get('assoc', None)
        if assoc is None:
            raise Exception('No assoc information is present at this point.')
        assoc_list = []
        for idx in range(len(assoc['FILENAME'])):
            assoc_list.append(
                    { field : assoc[field][idx] for field in fields })
        return assoc_list


if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)
    assoc_list = job.get_assoc_list_of_dicts()
    qoutfile = job.input.qoutfile
    lines = filelistutils.convert_single_files_to_lines(assoc_list)
    filelistutils.output_lines(qoutfile, lines, outtype='wcl')
