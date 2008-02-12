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

package DES::utils::posttasks;

use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use DES::desconfig;
use DES::utils::misc;
use DES::utils::hist;

use Exporter   ();
our (@ISA, @EXPORT);
@ISA         = qw(Exporter);
@EXPORT      = qw(&posttasks &appendDebugLog);


sub posttasks
{
   my $debugfh;
   open $debugfh, "> posttask.out";
   local *STDOUT = $debugfh;
   local *STDERR = $debugfh;

   my $config = $_[0];
   my $stage = $_[1];
   my $substagetype = $_[2];
   my $substage = $_[3];
   my $retcode = $_[4];

   debug("posttask", "stage = $stage");
   debug("posttask", "substagetype = $substagetype");
   debug("posttask", "substage = $substage");
   debug("posttask", "retcode = $retcode");

   my $str = sprintf("%d", $retcode);
   logEvent($config, $stage, $substagetype, $substage, "posttask", $str);

#   debug("posttask", "Checking to see if need to copy $stage.out");
#   if (-s "$stage.out")
#   {
#      appendDebugLog($config, "$stage.out", $stage);
#      if (-r "$stage.out")
#      {
##         unlink "$stage.out";
#      }
#   }
#
#   debug("posttask", "Checking to see if need to copy $stage.err");
#   if (-s "$stage.err")
#   {
#      appendDebugLog($config, "$stage.err", $stage);
#      if (-r "$stage.err")
#      {
##         unlink "$stage.err";
#      }
#   }
#
#   debug("posttask", "Checking to see if need to copy $stage.log");
#   if (-s "$stage.log")
#   {
#      $alllogs = $config->getValue("allcondorlogs");
#      debug("posttask", "alllogs = $alllogs");
#      open ALLLOGS, ">> $alllogs";
#      open LOG, "< $stage.log";
#      while ($line = <LOG>)
#      {
#         print ALLLOGS $line;
#      }
#      close LOG;
#      close ALLLOGS;
#   }

   close $debugfh;
   #unlink "desposttask.out";

   return $retcode;
}

sub appendDebugLog
{
   my $config = $_[0];
   my $file = $_[1];
   my $header = $_[2];

   my $uniqsimname = $config->getValue("uniqsimname");
   my $debuglog = $config->getValue("debuglog");

   if (-r $file)
   {
      my $out = `cat $file 2>&1`;
      my $err = open FILE, ">> $debuglog";
      if ($err == 0)
      {
         print STDERR "ERROR: Could not write to debug log '$debuglog'\n";
         exit $FAILURE;
      }
      print FILE "==========================================================================================\n";
      print FILE "== $header \n";
      print FILE "==========================================================================================\n";
      print FILE getTimeStamp(), "\n";
      print FILE "$out\n";
      print FILE "\n\n";
      close FILE;
   }
}


1;
