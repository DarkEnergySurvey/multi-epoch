########################################################################
#  $Id: descondorutils.pm 503 2007-11-08 21:37:32Z dadams $
#
#  $Rev:: 503                              $:  # Revision of last commit.
#  $LastChangedBy:: dadams                 $:  # Author of last commit. 
#  $LastChangedDate:: 2007-11-08 15:3      $:  # Date of last commit.
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

package NCSA::utils::condor;

use Switch;

use Exporter   ();
our (@ISA, @EXPORT);

@ISA         = qw(Exporter);
@EXPORT      = qw(&getCondorVersion  &parseCondorUserLog
                  &parseCondorQOutput  &createGlideinSubmitScript);

# returns the version in string format easy to compare
sub getCondorVersion
{
  $out = `condor_version 2>&1`;
  if ($out =~ /CondorVersion: (\d+)\.(\d+)\.(\d+)/)
  {
     $version = sprintf("%03d.%03d.%03d", $1, $2, $3);
  }
  print "version = '$version'\n";
}

sub parseCondorUserLog
{
  local *FH = shift;
  my $line;
  %jobs = ();

  $/ = "\n...";
  while ($line = <FH>)
  {
     $line =~ s/^\n//;
     if ($line =~ /\S/)
     {
        @line = split /\n/, $line;
        ($code, $jobnum, $time, $desc) = $line[0] =~ m/(\d+)\s+\((\d+).\d+.\d+\)\s+(\d+\/\d+\s+\d+:\d+:\d+)\s+(.+)/;

        $jobnum =~ s/^0+//;

        switch ($code)
        {
           case "000"  {
              $jobs{"$jobnum"}{"jobid"} = $jobnum;
              $jobs{"$jobnum"}{"clusterid"} = $jobnum;
              ($jobs{"$jobnum"}{"dagnode"}) = $line[1] =~ m/DAG Node: (\S+)/;
              $jobs{"$jobnum"}{"machine"} = "";
              $jobs{"$jobnum"}{"jobstat"} = "UNSUB";
              $jobs{"$jobnum"}{"submittime"} = $time;
           }
           case "001"  { # Job executing on host:
              $jobs{"$jobnum"}{"jobstat"} = "RUN";
              $jobs{"$jobnum"}{"starttime"} = $time;
           }
           #case "002"  { } # Error in executable
           #case "003"  { } # Job was checkpointed
           #case "004"  { } # Job evicted from machine
           case "005"  {
              $jobs{"$jobnum"}{"jobstat"} = "DONE";
              ($jobs{"$jobnum"}{"retval"}) = $line[1] =~ m/return value (\d+)/;
              $jobs{"$jobnum"}{"endtime"} = $time;
           }
           #case "006"  { } # Image size of job updated
           #case "007"  { } # Shadow threw an exception
           #case "008"  { } # Generic Log Event
           case "009"
           {
              $jobs{"$jobnum"}{"jobstat"} = "FAIL";
              $jobs{"$jobnum"}{"endtime"} = $time;
           }
           #case "010"  { } # Job was suspended
           #case "011"  { } # Job was unsuspended
           case "012"
           {
              $jobs{"$jobnum"}{"jobstat"} = "ERR";
              $jobs{"$jobnum"}{"holdreason"} = $line[1];
              $jobs{"$jobnum"}{"holdreason"} =~ s/^\s+//;
           }
           case "013"  { $jobs{"$jobnum"}{"jobstat"} = "UNSUB";}
           #case "014"  { } # Parallel Node executed
           #case "015"  { } # Parallel Node terminated

           case "016"
           {
         #016 (471.000.000) 04/11 11:48:08 POST Script terminated.
         #        (1) Normal termination (return value 100)
         #    DAG Node: fail
         #...
              $jobs{"$jobnum"}{"endtime"} = $time;
              ($retval) = $line[1] =~ m/return value\s+(\d+)/;
              if ($retval == 100)
              {
                $jobs{"$jobnum"}{"jobstat"} = "FAIL";
              }
              else
              {
                $jobs{"$jobnum"}{"jobstat"} = "DONE";
              }
           }
           case "017"  { #  Job submitted to Globus
              #  Beware of out of order log entries
              if (!exists($jobs{"$jobnum"}{"starttime"}) || ($jobs{"$jobnum"}{"starttime"} ne $time))
              {
                 $jobs{"$jobnum"}{"jobstat"} = "PEND";
              }
              ($jobs{"$jobnum"}{"gridresource"}) =
                                     $line[1] =~ m/RM-Contact:\s+(\S+)/;
           }
           #case "018"  { } # Globus Submit failed
           #case "019"  { } # Globus Resource Up
           #case "020"  { } # Globus Resource Down
           #case "021"  { } # Remote Error
           case "027"  { } # Job submitted to grid resource, same info as case 017
           else { $jobs{"$jobnum"}{"jobstat"} = "U$code"; }
        } # end switch
     }
  } # end reading file
  $/ = "\n";
  return \%jobs;
}

# parse condor_q output into hash table qjobs
sub parseCondorQOutput
{
  my $condorq_out = $_[0];

  $condorq_out =~ s/^\s+(\S)/$1/g;     # removing leading blank lines
  $condorq_out =~ s/^--[^\n]+\n//g;    # remove line starting with --

  @lines = split /\n/, $condorq_out;
  foreach $line (@lines)
  {
  #  print "$line\n";
    if ($line !~ /\S/)
    {
      # blank lines separate jobs
      foreach $k (keys %job)
      {
        $qjobs{"$id"}{"$k"}=$job{"$k"};
      }
      undef %job;
    }
    else
    {
      ($left,$right) = split / = /, $line;
      $left =~ tr/A-Z/a-z/; # convert key to all lowercase
      $right =~ s/"//g;
      if (($left =~ /args/) && ($right =~ /^-f/))
      {
        $left =~ "condorargs";
      }
      $job{"$left"} = $right;
      if ($left =~ /clusterid/)
      {
        $id = $right;
      }
    }
  }
  # don't forget to save the last job into big hash table
  foreach $k (keys %job)
  {
    $qjobs{"$id"}{"$k"}=$job{"$k"};
  }
  undef %job;
  return %qjobs;
}


sub createGlideinSubmitScript
{
   my ($args) = @_;
  
   my $file = $args->{"file"};
   my $psn = $args->{"psn"};
   my $queue = $args->{"queue"};
   my $useppn = $args->{"useppn"};
   my $totalncpus = $args->{"totalncpus"};
   my $condorconfig = $args->{"condorconfig"};
   my $glideinexe = $args->{"glideinexe"};
   my $sbinpath = $args->{"sbinpath"};
   my $localdir = $args->{"localdir"};
   my $minutes = $args->{"minutes"};
   my $idle = $args->{"idle"};
   my $jobmanager = $args->{"jobmanager"};
 
   my $hostname = `hostname -f 2>&1`;
   chomp $hostname;
 
   # use IP address instead of name to avoid rare problem where compute node
   # can't look up the IP address 
   $out = `host $hostname 2>&1`;
   my ($hostip) = $out =~ m/has address ([\d\.]+)/;
 
   # restrict this glidein to only run jobs submitted by same user
   # otherwise, sharing access to remote machine similar to sharing password
   my $owner = $ENV{"USER"};
 
   my $rsl = "";

   # if using machine where ppn is applicable
   if (($useppn != 0) && ($totalncpus != 1))
   {
      $host_xcount = ceil($totalncpus/$useppn);
      $rsl = "(host_xcount = $host_xcount)(xcount = 1)";
      if ($host_xcount > 1)
      {
         $rsl .= "(jobtype=multiple)";
      }
      else
      {
         $rsl .= "(jobtype=single)";
      }
   }
   else
   {
      $rsl = "(count = $totalncpus)(jobtype=single)";
   }
 
   if ($queue =~ /\S/)
   {
      $rsl .= "(queue=$queue)";
   }
   if ($psn =~ /\S/)
   {
      $rsl .= "(project=$psn)";
   }
   if ($minutes =~ /\d/)
   {
     $rsl .= "(maxwalltime=$minutes)";
   }

   # Need local version of condor.  Will use it to use matching version glidein executables
   $out = `condor_version 2>&1`;
   if ($out =~ /CondorVersion: (\d+)\.(\d+)\.(\d+)/)
   {
      $version = sprintf("%d.%d.%d", $1, $2, $3);
   }
   print "Local Condor version = '$version'\n";
 
#   # Assumes softline/renaming of full arch name directory on remote machine that is default
#   $binpath = $args->{"binpath"};
#   $sbinpath = $binpath."/".$version;
 
   # Have daemons shut down gracefully 1 minute before hit wallclock limit
   $runmin = $minutes - 1;
 
   $environment = "CONDOR_CONFIG=$condor_config;_condor_CONDOR_HOST=$hostip;_condor_GLIDEIN_HOST=$hostip;_condor_LOCAL_DIR=$localdir;_condor_NUM_CPUS=$useppn;_condor_SBIN=$sbinpath;_condor_START_owner=$owner;_condor_DaemonStopTime=DaemonStartTime+".($runmin*60).";_condor_STARTD_NOCLAIM_SHUTDOWN=$idle";
 
   open FILE, "> $filename";
   print FILE <<EOF;
universe = grid
grid_type = gt2
globusscheduler = $jobmanager
globusrsl = $rsl
executable = $glideinexe
arguments = -dyn -f -r $runmin
transfer_executable = false
environment = $environment
notification = error
EOF
 
  if (defined($debug))
  {
    print FILE "output = $filename.\$(Cluster).out\n";
    print FILE "error = $filename.\$(Cluster).err\n";
    print FILE "log = $filename.log\n";
  }
 
  print FILE "queue\n";
  close FILE;
}


1;
