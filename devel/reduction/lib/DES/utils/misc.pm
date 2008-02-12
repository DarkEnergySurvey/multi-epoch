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
package DES::utils::misc;

use strict;
use warnings;

use FindBin;
use lib "$FindBin::Bin/../lib";
use DES::desDBI;

use Exporter   ();
our (@ISA, @EXPORT);
@ISA         = qw(Exporter);
@EXPORT      = qw(&printVersion &getHome &getUserCfgDir &getTimeStamp &debug 
                  &runCmd);



################################################################
sub printVersion {
  print "DES: version 0.1a\n";
}


################################################################
sub getHome {
   my $Home="";

   $Home = $FindBin::RealBin;
   $Home =~ s/\/bin$//;
   $Home =~ s/\/libexec$//;
   $ENV{DES_HOME} = $Home;

   # verify pointing to DES installation
   if (! -r "$Home/bin/desversion")
   {
     print STDERR "Error: invalid DES installation\n";
     print STDERR "\tDES_HOME = '$Home'\n";
     $Home = "";
   }
   return $Home;
}

################################################################
sub getUserCfgDir
{
   return $ENV{"HOME"}."/.des";
}

################################################################
sub getTimeStamp {
  my @DInfo = localtime();
  my $TS = sprintf("%02d/%02d/%4d %02d:%02d:%02d", 
                    $DInfo[4]+1, $DInfo[3], $DInfo[5]+1900, 
                    $DInfo[2], $DInfo[1], $DInfo[0]);

  return $TS;
}

################################################################
sub debug {
  my $Stage = shift;
  my $Str = shift;

  print getTimeStamp(), " - $Stage: $Str\n"; 
}

################################################################
sub runCmd {
  my $Cmd = shift;
  my $TimeStamp = getTimeStamp();
  my $Out = `$Cmd 2>&1`;
  print "$TimeStamp - $Cmd\n";
  print "\t$Out\n";
}

1;
