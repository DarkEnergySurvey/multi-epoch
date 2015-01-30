from mojo.jobs.base_job import BaseJob
import os
import sys
import subprocess
import time
from despymisc.miscutils import elapsed_time

class Job(BaseJob):


    """ Stiff call for the multi-epoch pipeline

    Inputs:
    - self.ctx.comb_sci

    Outputs:
    - self.ctx.color_tile

    """

    # JOB INTERNAL CONFIGURATION
    STIFF_EXE = 'stiff'
    BKLINE = "\\\n"

    def run(self):

        # 1. Set the output name of the color tiff file
        color_tile =  self.ctx.get('color_tile',os.path.join(self.ctx.tiledir,"%s.tif" % self.ctx.tilename))

        # Make color tile visible to the context
        self.ctx.color_tile = color_tile

        # 2. get the update stiff parameters  --
        stiff_parameters = self.ctx.get('stiff_parameters', {})
        cmd_list = self.get_stiff_cmd_list(color_tile,stiff_parameters=stiff_parameters)  

        # 3. check execution mode and write/print/execute commands accordingly --------------
        executione_mode = self.ctx.get('stiff_execution_mode', 'tofile')
        if executione_mode == 'tofile':
            bkline  = self.ctx.get('breakline',self.BKLINE)
            # The file where we'll write the commands
            cmdfile = self.ctx.get('cmdfile', os.path.join(self.ctx.tiledir,"call_stiff_%s.cmd" % self.ctx.tilename))
            print "# Will write stiff call to: %s" % cmdfile
            with open(cmdfile, 'w') as fid:
                fid.write(bkline.join(cmd_list)+'\n')
                fid.write('\n')

        elif executione_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            print ' '.join(cmd_list)

        elif executione_mode == 'execute':
            logfile = self.ctx.get('logfile', os.path.join(self.ctx.tiledir,"stiff_%s.log" % self.ctx.tilename))
            log = open(logfile,"w")
            print "# Will proceed to run the stiff call now:"
            print "# Will write to logfile: %s" % logfile
            t0 = time.time()
            cmd  = ' '.join(cmd_list)
            print "# Executing stiff for tile:%s " % self.ctx.tilename
            print "# %s " % cmd
            subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            print "# Total stiff time %s" % elapsed_time(t0)
        else:
            raise ValueError('Execution mode %s not implemented.' % executione_mode)
        return


    def get_stiff_parameter_set(self,color_tile,**kwargs):

        """
        Set the Stiff default options and have the options to
        overwrite them with kwargs to this function.
        """

        stiff_parameters = {
            "OUTFILE_NAME"     : color_tile,
            "COMPRESSION_TYPE" : "JPEG",
            }

        stiff_parameters.update(kwargs)
        return stiff_parameters


    def get_stiff_cmd_list(self,color_tile,stiff_parameters={}):

        """ Make a color tiff of the TILENAME using stiff"""

        if self.ctx.NBANDS < 3:
            print "# WARINING: Not enough filters to create color image"
            print "# WARINING: No color images will be created"
            return 
        
        
        # The default stiff configuration file
        stiff_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.stiff')

        # The update parameters set
        pars = self.get_stiff_parameter_set(color_tile,**stiff_parameters)
        
        # Define the color filter sets we'd like to use, by priority depending on what BANDS will be combined
        cset1 = ['i','r','g']
        cset2 = ['z','r','g']
        cset3 = ['z','i','g']
        cset4 = ['z','i','r']
        csets = (cset1,cset2,cset3,cset4)
        CSET = False
        for color_set in csets:
            if CSET: break
            inset = list( set(color_set) & set(self.ctx.BANDS))
            if len(inset) == 3:
                CSET = color_set

        if not CSET:
            print "# WARNING: Could not find a suitable filter set for color image"
            return 
        
        cmd_list = []
        cmd_list.append("%s" % self.STIFF_EXE)
        for BAND in CSET:
            cmd_list.append( "%s" % self.ctx.comb_sci[BAND])

        cmd_list.append("-c %s" % stiff_conf)
        for param,value in pars.items():
            cmd_list.append("-%s %s" % (param,value))
        return cmd_list

    def __str__(self):
        return 'Creates the call to Stiff'




