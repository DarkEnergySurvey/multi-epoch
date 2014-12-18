from mojo.jobs.base_job import BaseJob
import os
import sys
import subprocess
import multiprocessing
import time
from despymisc.miscutils import elapsed_time
from despymisc.miscutils import work_subprocess_logging

class Job(BaseJob):

    """
    SExtractor call for catalog creation

    Inputs:
    - self.ctx.comb_sci[BAND]

    Outputs:
    - self.ctx.cat[BAND]

    """

    # JOB INTERNAL CONFIGURATION
    SEX_EXE = 'sex'
    BKLINE = "\\\n"

    def __call__(self):

        # 0. Do we want full MP SEx
        MP = self.ctx.get('MP_SEx', False)

        # 1. get the update SEx parameters for dual detection  --
        SExDual_parameters = self.ctx.get('SExDual_parameters', {})

        # 2. Build the list of command line for SEx for psf
        cmd_list = self.get_SExDual_cmd_list(SExDual_parameters=SExDual_parameters)

        # 3. check execution mode and write/print/execute commands accordingly --------------
        executione_mode = self.ctx.get('SExDual_execution_mode', 'tofile')
        if executione_mode == 'tofile':
            self.writeCall(cmd_list)

        elif executione_mode == 'dryrun':
            print "# For now we only print the commands (dry-run)"
            for band in self.ctx.BANDS:
                print ' '.join(cmd_list[band])

        elif executione_mode == 'execute':
            self.runSExDual(cmd_list,MP=MP)
        else:
            raise ValueError('Execution mode %s not implemented.' % executione_mode)
        return

    def writeCall(self,cmd_list):

        """ Write the SEx psf call to a file """

        bkline  = self.ctx.get('breakline',self.BKLINE)
        # The file where we'll write the commands
        cmdfile = self.ctx.get('cmdfile', os.path.join(self.ctx.tiledir,"call_SExDual_%s.cmd" % self.ctx.tilename))
        print "# Will write SExDual call to: %s" % cmdfile
        with open(cmdfile, 'w') as fid:
            for band in self.ctx.BANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n')
        return


    def runSExDual(self,cmd_list,MP=False):

        t0 = time.time()
        print "# Will proceed to run the SEx psf call now:"

        # Case A ---- single process MP is False
        if not MP:
            logfile = self.ctx.get('logfile', os.path.join(self.ctx.tiledir,"SExDual_%s.log" % self.ctx.tilename))
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
            for band in self.ctx.BANDS:
                cmds.append(' '.join(cmd_list[band]))
                logfile = os.path.join(self.ctx.tiledir,"SExDual_%s_%s.log" % (self.ctx.tilename,band))
                logs.append(logfile)
                
            pool = multiprocessing.Pool(processes=NP)
            pool.map(work_subprocess_logging, zip(cmds,logs))

        print "# Total SExtractor Dual time %s" % elapsed_time(t0)
        return


    def get_SExDual_parameter_set(self,**kwargs):

        """
        Set the SEx default options for dual SExtrator run and have the
        options to overwrite them with kwargs to this function.
        """

        # General pars, BAND-independent
        SExDual_parameters = {
            'CATALOG_TYPE'    : "FITS_LDAC",
            'FILTER_NAME'     : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.conv'),
            'STARNNW_NAME'    : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.nnw'),
            'CATALOG_TYPE'    :   'FITS_1.0',
            'WEIGHT_TYPE'     : 'MAP_WEIGHT',
            'MEMORY_BUFSIZE'  : 2048,
            'CHECKIMAGE_TYPE' : 'SEGMENTATION',
            'DETECT_THRESH'   : 1.5,
            'DEBLEND_MINCONT' : 0.001, 
            #'PARAMETERS_NAME' : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param'),
            'PARAMETERS_NAME' : os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','sex.param_psfonly'), # Faster!!!
            'VERBOSE_TYPE'    : 'NORMAL',
            }

        # Now update pars with kwargs
        SExDual_parameters.update(kwargs)
        return SExDual_parameters

    def get_SExDual_cmd_list(self,SExDual_parameters={}):
        
        """ Build/Execute the SExtractor call for psf on the detection image"""

        # SEx default configuration
        sex_conf = os.path.join(os.environ['MULTIEPOCH_DIR'],'etc','default.sex')
        
        # The updated parameters set for SEx
        pars = self.get_SExDual_parameter_set(**SExDual_parameters)

        SExDual_cmd = {}
        # Loop over all bands 
        dBAND = self.ctx.detBAND
        for BAND in self.ctx.BANDS:
            
            pars['MAG_ZEROPOINT']   =  30.0000 
            pars['CATALOG_NAME']    =  self.ctx.cat[BAND]           
            pars['PSF_NAME']        =  self.ctx.psf[BAND]
            pars['CHECKIMAGE_NAME'] =  self.ctx.checkimage[BAND]
            pars['WEIGHT_IMAGE']    =  "%s,%s" % (self.ctx.comb_sci[dBAND],self.ctx.comb_sci[BAND])
            
            # Build the call for the band, using dband as detection image
            cmd = []
            cmd.append("%s" % self.SEX_EXE)
            cmd.append("%s,%s" % (self.ctx.comb_sci[dBAND],self.ctx.comb_sci[BAND]))
            cmd.append("-c %s" % sex_conf)
            for param,value in pars.items():
                cmd.append("-%s %s" % (param,value))
            SExDual_cmd[BAND] = cmd

        return SExDual_cmd


    def __str__(self):
        return 'Creates the SExtractor call for dual detection'


