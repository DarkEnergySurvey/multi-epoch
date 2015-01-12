"""


INPUTS:

 - selt.ctx.sci
 - selt.ctx.wgt
 - clobber_MED   : Defined local clobber (to self.ctx.clobber_MEF)
 - MP_MEF        : Run the process in MP

OUTPUTS:
 - self.ctx.combMEF

"""

from mojo.jobs.base_job import BaseJob
import os,sys
from despymisc.miscutils import elapsed_time
import despyfits
import time
import multiprocessing

class Job(BaseJob):

    def __call__(self):

        t0 = time.time()

        # Get all of the relevant kwargs
        kwargs = self.ctx.get_kwargs_dict()
        clobber = kwargs.get('clobber_MEF', False)

        self.create_MEFs(clobber)
        print "# MEFs Creation Total time: %s" % elapsed_time(t0)
        return

    def create_MEFs(self,clobber):

        """ Create the MEF files using despyfits"""

        self.ctx.combMEF = {}
        for BAND in self.ctx.dBANDS:

            # Inputs
            filenames = [self.ctx.comb_sci[BAND],
                         self.ctx.comb_wgt[BAND]]
            
            # Set up the MEF output name
            outname = os.path.join(self.ctx.tiledir,"%s_%s.fits" %  (self.ctx.tilename, BAND))

            # Call it
            t0 = time.time()
            print "# Making MEF file for BAND:%s --> %s" % (BAND,outname)
            despyfits.makeMEF(filenames=filenames,outname=outname,clobber=clobber,extnames=['SCI','WGT'])
            print "# Done in %s\n" % elapsed_time(t0)
            
            # Pass it to the context
            self.ctx.combMEF[BAND] = outname

        return

    def __str__(self):
        return 'Create MEF file for a TILE'


# In case we want to run it as multi-process
# def runMakeMEF(args):
#     filenames,outname,clobber,extnames = args
#     """
#     Simple call despyfits.makeMEF
#     """
#     # Get the start time
#     t0 = time.time()
#     despyfits.makeMEF(filenames=filenames,outname=outname,clobber=clobber,extnames=extnames)
#     print "# Done in %s\n" % elapsed_time(t0)
#     return

