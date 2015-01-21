"""
Check that the tile directory exists and setup directory
"""

from mojo.jobs.base_job import BaseJob

import os


class Job(BaseJob):

    def __call__(self):

        kwargs = self.ctx.get_kwargs_dict()
        self.ctx.outputpath = kwargs.get('outputpath', './TILEBUILDER')
        self.ctx.tiledir = os.path.join(self.ctx.outputpath, self.ctx.tilename)
        if not os.path.exists(self.ctx.tiledir):
            print "# Creating %s" % self.ctx.tiledir
            os.makedirs(self.ctx.tiledir)
        else:
            print "# Will write output files to: %s" % self.ctx.tiledir

    def __str__(self):
        return 'setup tile directory'
