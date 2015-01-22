from mojo.jobs.base_job import BaseJob
import os
import sys
import time
import subprocess
from despymisc.miscutils import elapsed_time

class Job(BaseJob):

    """
    psfex call for the multi-epoch pipeline
    runs psfex on coadd images and detection

    Inputs:
    - self.ctx.psfcat[BAND] 

    Ouputs
    - self.ctx.psf[BAND] 

    """

    # JOB INTERNAL CONFIGURATION
    PSFEX_EXE = 'psfex'
    BKLINE = "\\\n"

    def __call__(self):


        # 1. get the update SEx parameters for psf --
        psfex_parameters = self.ctx.get('psfex_parameters', {})

        # 2. Build the list of command line for SEx for psf
        cmd_list = self.get_psfex_cmd_list(psfex_parameters=psfex_parameters)

        # 3. check execution mode and write/print/execute commands accordingly --------------
        executione_mode = self.ctx.get('psfex_execution_mode', 'tofile')
        if executione_mode == 'tofile':
            self.writeCall(cmd_list)

        elif executione_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            for band in self.ctx.dBANDS:
                print ' '.join(cmd_list[band])

        elif executione_mode == 'execute':
            self.runpsfex(cmd_list)
        else:
            raise ValueError('Execution mode %s not implemented.' % executione_mode)
        return

    def writeCall(self,cmd_list):

        """ Write the psfex call to a file """

        bkline  = self.ctx.get('breakline',self.BKLINE)
        # The file where we'll write the commands
        cmdfile = self.ctx.get('cmdfile', os.path.join(self.ctx.tiledir,"call_psfex_%s.cmd" % self.ctx.tilename))
        print "# Will write psfex call to: %s" % cmdfile
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return


    def runpsfex(self,cmd_list):

        t0 = time.time()
        print "# Will proceed to run the psfex call now:"

        logfile = self.ctx.get('logfile', os.path.join(self.ctx.tiledir,"SExpsf_%s.log" % self.ctx.tilename))
        log = open(logfile,"w")
        print "# Will write to logfile: %s" % logfile

        for band in self.ctx.dBANDS:
            t1 = time.time()
            cmd  = ' '.join(cmd_list[band])
            print "# Executing SEx/psf for tile:%s, BAND:%s" % (self.ctx.tilename,band)
            print "# %s " % cmd
            subprocess.call(cmd,shell=True,stdout=log, stderr=log)
            print "# Done band %s in %s\n" % (band,elapsed_time(t1))
        print "# Total psfex time %s" % elapsed_time(t0)
        return


    def get_psfex_parameter_set(self,**kwargs):

        """
        Set the psfex default options for the psf run and have the
        options to overwrite them with kwargs to this function.
        """
        psfex_parameters = {
            'WRITE_XML' : 'Y',
        }
        # Now update pars with kwargs
        psfex_parameters.update(kwargs)
        return psfex_parameters

    def get_psfex_cmd_list(self,psfex_parameters={}):
        
        """ Build the psfex call that runs psfex on coadd images and detection"""

        # psfex default configuration
        psfex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.psfex')
        
        # The updated parameters set for SEx
        pars = self.get_psfex_parameter_set(**psfex_parameters)

        # Loop over all bands and Detection
        psfex_cmd = {}
        for BAND in self.ctx.dBANDS:
            pars['XML_NAME'] = self.ctx.psfexxml[BAND]
            # Build the call
            cmd = []
            cmd.append("%s" % self.PSFEX_EXE)
            cmd.append("%s" % self.ctx.psfcat[BAND])
            cmd.append("-c %s" % psfex_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            psfex_cmd[BAND] = cmd

        return psfex_cmd

    def __str__(self):
        return 'Creates the psfex call for multi-epoch pipeline'



