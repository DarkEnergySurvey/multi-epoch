"""
Check that the tile directory exists and setup directory
"""

from mojo.jobs.base_job import BaseJob

import os


class Job(BaseJob):

    def run(self):

        kwargs = self.ctx.get_kwargs_dict()
        self.ctx.outputpath = kwargs.get('outputpath', './TILEBUILDER')
        #self.ctx.tiledir  = os.path.join(self.ctx.outputpath, self.ctx.tilename)
        self.ctx.basedir  = os.path.join(self.ctx.outputpath, self.ctx.tilename)
        self.ctx.basename = os.path.join(self.ctx.basedir, self.ctx.tilename)
        if not os.path.exists(self.ctx.basedir):
            print "# Creating %s" % self.ctx.basedir
            os.makedirs(self.ctx.basedir)
        else:
            print "# Will write output files to: %s" % self.ctx.basedir

    def __str__(self):
        return 'setup tile directory'
