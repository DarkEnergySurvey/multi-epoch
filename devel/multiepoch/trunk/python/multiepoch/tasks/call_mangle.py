#!/usr/bin/env python

# Mojo imports
from mojo.jobs.base_job import BaseJob
from traitlets import Unicode, Bool, Float, Int, CUnicode, CBool, CFloat, CInt, Instance, Dict, List, Integer
from mojo.jobs.base_job import BaseJob, IO, IO_ValidationError
from mojo.context import ContextProvider

import os
import sys
import subprocess
import time
from despymisc.miscutils import elapsed_time
from despyastro import tableio
import numpy

# Multi-epoch
import multiepoch.utils as utils
import multiepoch.contextDefs as contextDefs
import multiepoch.querylibs as querylibs
from multiepoch import file_handler as fh

# JOB INTERNAL CONFIGURATION
MANGLE_EXE = 'mangle_one_tile'
BKLINE = "\\\n"
MAGBASE = 30

class Job(BaseJob):

    """
    Stiff call for the multi-epoch pipeline
    """

    class Input(IO):

        """
        Mangle call for the multi-epoch pipeline
        """
        ######################
        # Positional Arguments
        # 1. Tilename amd association file and assoc dictionary
        assoc      = Dict(None,help="The Dictionary containing the association file",argparse=False)
        assoc_file = CUnicode('',help="Input association file with CCDs information",input_file=True,
                              argparse={ 'argtype': 'positional', })
        # Optional Arguments
        db_section    = CUnicode("db-destest",help="DataBase Section to connect", 
                                 argparse={'choices': ('db-desoper','db-destest', )} )
        http_section = Unicode('http-desarchive',help='The corresponding section in the .desservices.ini file.')

        tilename     = Unicode(None, help="The Name of the Tile Name to query",argparse=True)
        tilename_fh = CUnicode('',  help="Alternative tilename handle for unique identification default=TILENAME")
        tiledir     = Unicode(None, help='The output directory for this tile')
        tileid      = CInt(help="The COADDTILE_ID for the Tile Name", argparse=True)
        execution_mode_mangle  = CUnicode("tofile",help="Mangle excution mode",
                                         argparse={'choices': ('tofile','dryrun','execute')})
        mangle_conf = CUnicode(help="Optional Mangle configuration file")
        pwf_attempt_id  = CUnicode(1,help="Optional pwf_attemp_id")

        doBANDS  = List(['all'],help="BANDS to processs (default=all)",argparse={'nargs':'+',})
        #magbase  = CFloat(MAGBASE, help="Zero point magnitude base, default=%s." % MAGBASE)
        magbase  = CInt(MAGBASE, help="Zero point magnitude base, default=%s." % MAGBASE)

        # Logging -- might be factored out
        stdoutloglevel = CUnicode('INFO', help="The level with which logging info is streamed to stdout",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )
        fileloglevel   = CUnicode('INFO', help="The level with which logging info is written to the logfile",
                                  argparse={'choices': ('DEBUG','INFO','CRITICAL')} )

        # Function to read ASCII/panda framework file (instead of json)
        # Comment if you want to use json files
        def _read_assoc_file(self):
            mydict = {}
            df = pd.read_csv(self.assoc_file,sep=' ')
            mydict['assoc'] = {col: df[col].values.tolist() for col in df.columns}
            return mydict

        def _validate_conditional(self):
            if self.tilename_fh == '':
                self.tilename_fh = self.tilename

    def prewash(self):

        """ Pre-wash of inputs, some of these are only needed when run as script"""
        # Re-cast the ctx.assoc as dictionary of arrays instead of lists
        self.ctx.assoc  = utils.dict2arrays(self.ctx.assoc)
        # Get the BANDs information in the context if they are not present
        if self.ctx.get('gotBANDS'):
            self.logger.info("BANDs already defined in context -- skipping")
        else:
            self.ctx.update(contextDefs.get_BANDS(self.ctx.assoc, detname=self.ctx.detname,logger=self.logger,doBANDS=self.ctx.doBANDS))

        # Check info OK
        self.logger.info("BANDS:   %s" % self.ctx.BANDS)
        self.logger.info("doBANDS: %s" % self.ctx.doBANDS)
        self.logger.info("dBANDS:  %s" % self.ctx.dBANDS)

        if self.ctx.get('root_https'):
            self.logger.info("Found root_https: %s" % self.ctx.root_https)
        else:
            self.ctx.root_https = querylibs.get_root_https(self.ctx.dbh,logger=self.logger, archive_name=self.ctx.archive_name)

    def run(self):

        execution_mode = self.input.execution_mode_mangle

        # 0. Prepare the context
        self.prewash()

        # Check for the db_handle
        self.ctx = utils.check_dbh(self.ctx, logger=self.logger)

        # 1. Get the configuration file
        if self.input.mangle_conf == '':
            self.ctx.mangle_conf = fh.get_configfile('mangle_Y3A1v1',ext='params')
            self.logger.info("Will use mangle default configuration file: %s" % self.ctx.mangle_conf)

        # 2. Write the lists
        self.write_mangle_input_list_files()

        # 3. Get the pol files location
        self.ctx.polfiles = self.get_polfiles_locations(version='Y3A1v1')

        # 4. Transfer
        # Only transfer and make dire if executing
        if execution_mode == 'execute':
            utils.transfer_input_files(self.ctx.polfiles, clobber=True, section=self.ctx.http_section, logger=self.logger)

        # 4. Get the command list
        cmd_list = self.get_mangle_cmd_list()

        # 5. execute cmd_list according to execution_mode 
        if execution_mode == 'tofile':
            self.writeCall(cmd_list)

        elif execution_mode == 'dryrun':
            self.logger.info("For now we only print the commands (dry-run)")
            [self.logger.info(' '.join(cmd_list[band])) for band in self.ctx.doBANDS]

        #elif execution_mode == 'execute':
        #    raise ValueError('Execution mode %s not implemented.' % execution_mode)
        return



    def write_mangle_input_list_files(self):

        """ Write the input file list for SWarp"""
        self.logger.info('Writing mangle input files')

        for BAND in self.ctx.doBANDS:

            idx = numpy.where(self.ctx.assoc['BAND'] == BAND)[0]
            red_inputs  = self.ctx.assoc['FILEPATH_LOCAL'][idx]
            me_inputs   = self.ctx.assoc['FILEPATH_NWG'][idx]
            magzero     = self.ctx.assoc['MAG_ZERO'][idx]
            
            # Now let's sort them by filename
            isort = numpy.argsort(red_inputs)
            red_inputs = red_inputs[isort]
            me_inputs  = me_inputs[isort]
            magzero    = magzero[isort]
            
            # writing the lists to files using tableio.put_data()
            tableio.put_data(fh.get_mangle_list_red(self.ctx.tiledir, self.ctx.tilename_fh, BAND),(red_inputs,magzero),  format='%s, %s')
            tableio.put_data(fh.get_mangle_list_me(self.ctx.tiledir, self.ctx.tilename_fh, BAND),(me_inputs,),  format='%s')

        return

    def get_mangle_cmd_list(self):

        """ Get the mangle command list"""

        """
        Example call:
        mangle_one_tile  --db_section db-prodbeta
        --band i
        --poltiles mangle_tiles/Y3A1v1_tiles_10s.124050.pol
        --poltolys mangle_tiles/Y3A1v1_tolys_10s.124050.pol
        --paramfile config/20160613_mangle_Y3A1v1.params
        --runn DES0459-5622_r13p26
        --pfw_attempt_id 4016
        --mzpglobal 30
        --list_nwgint list/mangle/DES0459-5622_r13p26_i_mangle-nwgint.list
        --list_redimg list/mangle/DES0459-5622_r13p26_i_mangle-red.list
        --tilename DES0459-5622
        --tileid 124050
        --coadd coadd/DES0459-5622_r13p26_i.fits
        --outputdir mangle/i
        --molysprefix DES0459-5622_r13p26_i_molys
        --polprefix DES0459-5622_r13p26_i
        --compare_plot1 qa/mangle/DES0459-5622_r13p26_i_mangle-1.png
        --compare_plot2 qa/mangle/DES0459-5622_r13p26_i_mangle-2.png
        --cleanup Y
        """

        tiledir     = self.input.tiledir
        tileid      = self.input.tileid
        tilename    = self.input.tilename
        tilename_fh = self.input.tilename_fh

        # 2.Get the pol files

        self.logger.info("assembling commands for mangle call")

        cmd_list = {}
        for BAND in self.ctx.doBANDS:

            # Make sure the outputdir exists
            outputdir = os.path.join(tiledir,'mangle',BAND)
            utils.check_filepath_exist(outputdir,logger=self.logger.info)

            cmd = []
            cmd.append("%s" % MANGLE_EXE)
            cmd.append("--db_section %s" % self.ctx.db_section)
            cmd.append("--band %s" % BAND)
            cmd.append("--poltiles %s" % fh.get_poltiles(tiledir,tileid))
            cmd.append("--poltolys %s" % fh.get_poltolys(tiledir,tileid))
            cmd.append("--paramfile %s" % self.ctx.mangle_conf)
            cmd.append("--runn %s" % tilename_fh)
            cmd.append("--pfw_attempt_id %s" % self.ctx.pwf_attempt_id) 
            cmd.append("--mzpglobal %s" % self.ctx.magbase)
            cmd.append("--list_nwgint %s" % fh.get_mangle_list_me(tiledir, tilename_fh, BAND))
            cmd.append("--list_redimg %s" % fh.get_mangle_list_red(tiledir, tilename_fh, BAND))
            cmd.append("--tilename %s" % tilename)
            cmd.append("--tileid %s" % tileid)
            cmd.append("--coadd %s" % fh.get_mef_file(tiledir, tilename_fh, BAND))
            cmd.append("--outputdir %s" % outputdir) 
            cmd.append("--molysprefix %s_%s_molys" % (tilename_fh,BAND))
            cmd.append("--polprefix %s_%s" % (tilename_fh,BAND))
            cmd.append("--compare_plot1 %s" % fh.get_mangle_plot(tiledir,tilename_fh,BAND,number=1))
            cmd.append("--compare_plot2 %s" % fh.get_mangle_plot(tiledir,tilename_fh,BAND,number=2))
            cmd.append("--cleanup Y")
            cmd_list[BAND] = cmd

        return cmd_list


    def get_polfiles_locations(self,version='Y3A1v1'):

        # Get the locations in the DB
        pols = querylibs.find_mangle_pol_files(self.ctx.tileid,self.ctx.dbh,version=version)
        Nfiles = len(pols['FILENAME'])

        polfiles = {}
        polfiles['FILEPATH_HTTPS'] = [os.path.join(self.ctx.root_https,pols['PATH'][k],pols['FILENAME'][k]) for k in range(Nfiles)]
        polfiles['FILEPATH_LOCAL'] = [fh.get_poltiles(self.ctx.tiledir,self.ctx.tileid),
                                      fh.get_poltolys(self.ctx.tiledir,self.ctx.tileid)]
        polfiles['FILEPATH_HTTPS'].sort()
        polfiles['FILEPATH_LOCAL'].sort()
        return polfiles


    def transfer_polfiles(self,clobber=False):

        """ Transfer the files contained in an info dictionary"""
        
        # Now get the files via http
        polfiles = self.ctx.polfiles
        Nfiles = len(infodict['FILEPATH_HTTPS'])
        for k in range(Nfiles):
            
            url       = infodict['FILEPATH_HTTPS'][k]
            localfile = infodict['FILEPATH_LOCAL'][k]

            # Make sure the file does not already exists exits
            if not os.path.exists(localfile) or clobber:
                
                dirname   = os.path.dirname(localfile)
                if not os.path.exists(dirname):
                    os.makedirs(dirname)
                    
                logger.info("Getting:  %s (%s/%s)" % (url,k+1,Nfiles))
                sys.stdout.flush()
                # Get a file using the $HOME/.desservices.ini credentials
                http_requests.download_file_des(url,localfile,section=section)
            else:
                logger.info("Skipping: %s (%s/%s) -- file exists" % (url,k+1,Nfiles))



    def writeCall(self,cmd_list,mode='w'):

        bkline  = self.ctx.get('breakline',BKLINE)
        # The file where we'll write the commands
        cmdfile = fh.get_mangle_cmd_file(self.input.tiledir, self.input.tilename_fh)
        self.logger.info("Will write mangle call to: %s" % cmdfile)
        with open(cmdfile, mode) as fid:
            for band in self.ctx.doBANDS:
                fid.write(bkline.join(cmd_list[band])+'\n')
                fid.write('\n\n')
        return

    def __str__(self):
        return 'Creates the call to Mangle'

if __name__ == '__main__':
    from mojo.utils import main_runner
    job = main_runner.run_as_main(Job)

