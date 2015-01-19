"""
Collect the files and their absolute paths to start the SWarping process,
it creates a dictionary: self.swarp_inputs[BAND] keyed to the BAND and a
numpy str-array: self.ccdimages['FILEPATH'] that cointains the filepath for each entry.

Useful to testing:

We can change the archive_root from the default=/archive_data/Archive
to any custom place we made. 


To get the URL for http access, the sql queries are:

 1) http for FR:
 select val from ops_archive_val where name='desar2home' and key='root_http';

 2) https for science
 select val from ops_archive_val where name='desar2home' and key='root_https';

 INPUTS:

 - archive_name  (usually desar2home)
 - self.ctx.CCDS

 OUTPUTS:

 - self.ctx.FILEPATH : The relative path of the fits files
 - self.ctx.FILEPATH_ARCHIVE : The full path in the cosmology archive
 - self.ctx.FILEPATH_HTTPS:    The full URL for download

"""

from mojo.jobs.base_job import BaseJob
import numpy

npadd = numpy.core.defchararray.add


class Job(BaseJob):

    def __call__(self):


        # Get all of the relevant kwargs
        kwargs = self.ctx.get_kwargs_dict()
        archive_name = kwargs.pop('archive_name', 'desar2home') # or get?

        # Number of images/filenames
        Nimages = len(self.ctx.CCDS['FILENAME'])

        # 1. Figure out the archive root and root_http variables, it returns
        # self.ctx.archive_root and self.ctx.root_https
        self.get_archive_root(archive_name)

        # 2. Construnct the relative path for each file as a combination of PATH + FILENAME
        self.ctx.FILEPATH = npadd(self.ctx.CCDS['PATH'],"/")
        self.ctx.FILEPATH = npadd(self.ctx.FILEPATH,self.ctx.CCDS['FILENAME'])

        # 3. Create the archive locations for each file
        path = [self.ctx.archive_root+"/"]*Nimages
        self.ctx.FILEPATH_ARCHIVE = npadd(path,self.ctx.FILEPATH)

        # 4. Create the https locations for each file
        path = [self.ctx.root_https+"/"]*Nimages
        self.ctx.FILEPATH_HTTPS   = npadd(path,self.ctx.FILEPATH)

    def __str__(self):
        return "Find the location of the fits files"


    def get_archive_root(self,archive_name='desar2home'):

        """
        Get the archive root and root_https fron the database
        """

        # archive root
        query = "select root from ops_archive where name='%s'" % archive_name
        print "# Getting the archive root name for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur = self.ctx.dbh.cursor()
        cur.execute(query)
        self.ctx.archive_root = cur.fetchone()[0]
        print "# archive_root: %s" % self.ctx.archive_root

        # root_https
        query = "select val from ops_archive_val where name='%s' and key='root_https'" % archive_name
        print "# Getting root_https for section: %s" % archive_name
        print "# Will execute the SQL query:\n********\n** %s\n********" % query
        cur.execute(query)
        self.ctx.root_https = cur.fetchone()[0]
        print "# root_https: %s" % self.ctx.root_https

        cur.close()
        return
