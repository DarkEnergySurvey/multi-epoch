from mojo.jobs.base_job import BaseJob
import os
import sys
import subprocess
import multiprocessing
import time
from despymisc.miscutils import elapsed_time

class Job(BaseJob):

    """
    SExtractor call for psf creation

    Inputs:
    - self.ctx.comb_sci

    Outputs:
    - self.ctx.psfcat

    """

    # JOB INTERNAL CONFIGURATION
    SEX_EXE = 'sex'
    BKLINE = "\\\n"

    def __call__(self):

        # 0. Do we want full MP SEx
        MP = self.ctx.get('MP_SEx', False)

        # 1. get the update SEx parameters for psf --
        SExpsf_parameters = self.ctx.get('SExpsf_parameters', {})

        # 2. Build the list of command line for SEx for psf
        cmd_list = self.get_SExpsf_cmd_list(SExpsf_parameters=SExpsf_parameters)

        # 3. check execution mode and write/print/execute commands accordingly --------------
        executione_mode = self.ctx.get('SExpsf_execution_mode', 'tofile')
        if executione_mode == 'tofile':
            self.writeCall(cmd_list)

        elif executione_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            for band in self.ctx.dBANDS:
                print ' '.join(cmd_list[band])

        elif executione_mode == 'execute':
            self.runSExpsf(cmd_list,MP=MP)
        else:
            raise ValueError('Execution mode %s not implemented.' % executione_mode)
        return

    def writeCall(self,cmd_list):

        """ Write the SEx psf call to a file """

        bkline  = self.ctx.get('breakline',self.BKLINE)
        # The file where we'll write the commands
        cmdfile = self.ctx.get('cmdfile', os.path.join(self.ctx.tiledir,"call_SExpsf_%s.cmd" % self.tilename))
        print "# Will write SExpsf call to: %s" % cmdfile
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.dBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return


    def runSExpsf(self,cmd_list,MP=False):

        t0 = time.time()
        print "# Will proceed to run the SEx psf call now:"

        # Case A ---- single process MP is False
        if not MP:
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
            
        # Case B -- multi-process MP true or interger to decide number of processor to use
        else:
            if type(MP) is bool:
                NP = multiprocessing.cpu_count()
            elif type(MP) is int:
                NP = MP
            else:
                raise ValueError('MP is wrong type: %s, must be bool or integer type' % MP)

            print "# Will Use %s processors" % NP
            cmds = []
            logs = []
            for band in self.ctx.dBANDS:
                cmds.append(' '.join(cmd_list[band]))
                logfile = os.path.join(self.ctx.tiledir,"SExpsf_%s_%s.log" % (self.ctx.tilename,band))
                logs.append(logfile)
                
            pool = multiprocessing.Pool(processes=NP)
            pool.map(work_subprocess_logging, zip(cmds,logs))

        print "# Total SEx psf time %s" % elapsed_time(t0)
        return


    def get_SExpsf_parameter_set(self,**kwargs):

        """
        Set the SEx default options for the psf run and have the
        options to overwrite them with kwargs to this function.
        """

        SExpsf_parameters = {
            'CATALOG_TYPE'    : "FITS_LDAC",
            'WEIGHT_TYPE'     : 'MAP_WEIGHT',
            'PARAMETERS_NAME' : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param_psfex'),
            'FILTER_NAME'     : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.conv'),
            'STARNNW_NAME'    : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.nnw'),
            'SATUR_LEVEL'     : 65000,
            'VERBOSE_TYPE'    : 'NORMAL',
            'DETECT_MINAREA'  : 3, # WHY??????????? son small!!!!
            }

        # Now update pars with kwargs
        SExpsf_parameters.update(kwargs)
        return SExpsf_parameters

    def get_SExpsf_cmd_list(self,SExpsf_parameters={}):
        
        """ Build/Execute the SExtractor call for psf on the detection image"""

        # SEx default configuration
        sex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.sex')
        
        # The updated parameters set for SEx
        pars = self.get_SExpsf_parameter_set(**SExpsf_parameters)

        SExpsf_cmd = {}
        # Loop over all bands and Detection
        for BAND in self.ctx.dBANDS:
            pars['WEIGHT_IMAGE'] = self.ctx.comb_wgt[BAND]
            pars['CATALOG_NAME'] = self.ctx.psfcat[BAND]
            # Build the call
            
            cmd = []
            cmd.append("%s" % self.SEX_EXE)
            cmd.append("%s" % self.ctx.comb_sci[BAND])
            cmd.append("-c %s" % sex_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            SExpsf_cmd[BAND] = cmd

        return SExpsf_cmd


    def __str__(self):
        return 'Creates the SExtractor call for psf'


def work_subprocess(cmd):
        
    """ Dummy function to call in multiprocess with shell=True """
    return subprocess.call(cmd,shell=True) #,stdout=log, stderr=log)

def work_subprocess_logging(tup):

    """ Dummy function to call in multiprocess with shell=True """
    cmd,logfile = tup
    log = open(logfile,"w")
    print "# Will write to logfile: %s" % logfile
    return subprocess.call(cmd,shell=True ,stdout=log, stderr=log)


