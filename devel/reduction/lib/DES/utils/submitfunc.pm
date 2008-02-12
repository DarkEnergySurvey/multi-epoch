########################################################################
#  $Id$
#
#  $Rev::                                  $:  # Revision of last commit.
#  $LastChangedBy::                        $:  # Author of last commit. 
#  $LastChangedDate::                      $:  # Date of last commit.
#
#  Authors: 
#         Michelle Gower (mgower@ncsa.uiuc.edu)
#         Darren Adams (dadams@ncsa.uiuc.edu)
#
#  Developed at: 
#  The National Center for Supercomputing Applications (NCSA).
#
#  Copyright (C) 2007 Board of Trustees of the University of Illinois. 
#  All rights reserved.
#
#  DESCRIPTION:
#
#######################################################################

package DES::utils::submitfunc;

use Exporter   ();
our (@ISA, @EXPORT);
@ISA         = qw(Exporter);
@EXPORT      = qw(&addInfoToDag &addInfoToCondor &runChecks);

# For status commands, add some attributes to DAGMan submit file 
# before submiting
sub addInfoToDag
{
  my $Config = $_[0];
  my $dagfile = $_[1];
  my $debugfh = $_[2];
  my ($maxpre, $maxpost, $maxidle, $maxjobs);
  my ($stage, $cmd);

  $stage = "";
  if ($dagfile =~ /(\S+)_mngr.dag/)
  {
    $stage = $1;
  }

  my %searchdefs = { "stage" => "$stage"};
  $maxpre = $Config->getValue("dagman_max_pre", \%searchdefs);
  $maxpost = $Config->getValue("dagman_max_post", \%searchdefs);
  $maxidle = $Config->getValue("dagman_max_idle", \%searchdefs);
  $maxjobs = $Config->getValue("dagman_max_jobs", \%searchdefs);

  $cmd = "condor_submit_dag -f -no_submit -notification never ";
  if ($maxpre =~ /\d/)
  {
     $cmd .= " -MaxPre $maxpre ";
  }
  if ($maxpost =~ /\d/)
  {
     $cmd .= " -MaxPost $maxpost ";
  }
  if ($maxjobs =~ /\d/)
  {
     $cmd .= " -maxjobs $maxjobs ";
  }
  if ($maxidle =~ /\d/)
  {
     $cmd .= " -maxidle $maxidle ";
  }

  $out = `$cmd ${dagfile} 2>&1`;
  print $debugfh "cmd> $cmd ${dagfile}\n";
  print $debugfh "$out\n";

  $dagfile .= ".condor.sub";

  print $debugfh "Calling addInfoToCondor with '$dagfile'\n";

  addInfoToCondor($Config, $dagfile, $stage, "mngr", $debugfh);

  return $dagfile;
}

################################################################
# add some attributes to DAGMan submit file before submitting
sub addInfoToCondor
{
  my $Config = $_[0];
  my $condorfile = $_[1];
  my $stage = $_[2];
  my $substage = $_[3];
  my $debugfh = $_[4];
  

  my %searchdefs = { "stage" => "$stage"};
  my $uniqname = $Config->getValue("uniqname", \%searchdefs);
  my $nite = $Config->getValue("nite", \%searchdefs);
  my $runid = $Config->getValue("runid", \%searchdefs);
  my $RunSite = $Config->getValue("run_site", \%searchdefs);
  my $outputdir = $Config->getValue("outputdir", \%searchdefs);

  open(CONDOR, "<${condorfile}") || die "can't open submission file ${condorfile}: $!";
  undef $/;
  $data = <CONDOR>;
  close CONDOR;
  $/ = "\n";

  #  Work around condor_submit_dag bug (6.7.20, 6.8.0-6.8.3, 6.9.1)
  # "The OnExitRemove expression generated for DAGMan by condor_submit_dag evaluated 
  #  to UNDEFINED for some values of ExitCode, causing condor_dagman to go on hold."
  if ($data =~ /on_exit_remove/) 
  {
     if ($data =~ /on_exit_remove\s*=\s*\(\s*ExitSignal\s*==\s*11\s*||\s*\(ExitCode\s*>=0\s*&&\s*ExitCode\s*<=\s*2\)\)/)
     {
         $data =~ s/on_exit_remove\s+=[^\n]+\n/on_exit_remove  = ( ExitSignal =?= 11 || (ExitCode =!= UNDEFINED && ExitCode >=0 && ExitCode <= 2))\n/; 
###         $data =~ s/queue/on_exit_remove  = ( ExitSignal =?= 11 || (ExitCode =!= UNDEFINED && ExitCode >=0 && ExitCode <= 2))\nqueue/;
     }
  }

  $data =~ s/\nqueue$/\n+des_isdesjob=TRUE\n+des_nite="${nite}"\n+des_runid="${runid}"\n+des_stage="$stage"\n+des_substage="$substage"\n+des_runsite="$RunSite"\nqueue/;

  open(CONDOR,">${condorfile}") || die("Cannot open file for writing");
  print CONDOR $data,"\n";
  close CONDOR;
}

######################################################################
sub runChecks {
   my $Config = $_[0];
   my $Out;

   print "Checking for valid proxy....";
   $Out = `grid-proxy-info -exists -valid 0:30 > /dev/null 2>&1; echo \$?`;
   chop($Out);
   if ($Out !~ /^0$/) {
     print "\n\nERROR: Couldn't find a proxy that will be valid for at least 30 minutes.\n";
     print "First run grid-proxy-init or myproxy-logon\n";
     exit 1;
   }
   else {
     print "PASSED\n";
   }


   ### Check for Condor in path as well as daemons running
   print "Checking for Condor....";
   $Out = `condor_version 2>&1`;
   if ($Out !~ /\$CondorVersion: (\d+.\d+)/) {
     print "ERROR\nCouldn't figure out Condor version\n";
     print "Make sure Condor binaries are in your path\n";
     exit 1;
   }

   $Out = `ps -ef | grep -i condor_master | grep -v grep 2>&1`;
   if ($Out !~ /condor_master/) {
     print "ERROR\nCondor is not running on this machine\n";
     print "Contact your condor administrator\n";
     exit 1;
   }
   else {
     print "PASSED\n";
   }

#   print "Checking for run_site definition....";
#   my $RunSite = $Config->getValue("run_site");
#   if ($RunSite !~ /\S/) {
#      print "ERROR\n\tMissing run_site value\n";
#      print "\nSUBMISSION ABORTED\n\n";
#      exit 1;
#   }
#
#   my %SearchDefs;
#   $SearchDefs{"site"} = $RunSite;
#   $checkrunsite = $Config->getValue("__name", \%SearchDefs);
#   if ($checkrunsite ne $RunSite) {
##print "checkrunsite = '$checkrunsite'\n";
##print "runsite = '$RunSite'\n";
#      print "ERROR\n\tMissing runsite definition for runsite '$RunSite'\n";
#      print "\nSUBMISSION ABORTED\n\n";
#      exit 1;
#   }
#   else {
#      print "PASSED\n";
#   }
#
#   # verify remote runsite and globus scheduler (catching typos)
#   print "Checking remote run site and globus scheduler....";
#   my $GridHost = $Config->getValue("grid_host", \%SearchDefs);
#   my $GridPort = $Config->getValue("grid_port", \%SearchDefs);
#   my $BatchType = $Config->getValue("batch_type", \%SearchDefs);
#
#   if ($GridHost !~ /\S/) {
#      print "ERROR\n\tgrid_host is not defined for run site '$RunSite'.\n";
#      print "\nSUBMISSION ABORTED\n\n";
#      exit 1;
#   }
#   if ($BatchType !~ /\S/) {
#      print "ERROR\n\tbatch_type is not defined for run site '$RunSite'.\n";
#      print "\nSUBMISSION ABORTED\n\n";
#      exit 1;
#   }
#
#   my $GlobSched = "$GridHost:$GridPort/jobmanager-$BatchType";
#   $Out = `globusrun -a -r $GlobSched 2>&1; echo \$?`;
#   if ($Out !~ /\n0$/) {
#      print "ERROR\nCannot authenticate to '$GlobSched'\n";
#      print "First check for typos.  ";
#      print "If no typos, then the globus scheduler is down.\n\n";
#      print "\nSUBMISSION ABORTED\n\n";
#      exit 1;
#   }
#   else {
#      print "PASSED\n";
#   }
#
#   my $GridftpHost = $Config->getValue("gridftp_host", \%SearchDefs);
#   my $GridftpPort = $Config->getValue("gridftp_port", \%SearchDefs);
#   if ($GridftpHost !~ /\S/) {
#      print "ERROR\n\tftlocalremotehost is not defined for runsite '$RunSite'.\n";
#      print "\nSUBMISSION ABORTED\n\n";
#      exit 1;
#   }
#  
#   print "Checking GridFTP to remote runsite....";
#   $Out = `uberftp -P $GridftpPort $GridftpHost "pwd" 2>&1; echo \$?`;
#   if ($Out !~ /\n0$/) {
#      print "ERROR\n\tCannot contact GridFTP server on '$GridftpHost'\n";
#      print "\tFirst check for typos.  ";
#      print "\tIf no typos, then the GridFTP server is down.\n\n";
#      print "\nSUBMISSION ABORTED\n\n";
#      exit 1;
#   }
#   else {
#      print "PASSED\n";
#   }
}


1;
