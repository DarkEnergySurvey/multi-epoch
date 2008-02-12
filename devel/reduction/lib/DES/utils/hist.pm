########################################################################
#  $Id: deshistutils.pm 510 2007-11-12 14:45:59Z dadams $
#
#  $Rev:: 510                              $:  # Revision of last commit.
#  $LastChangedBy:: dadams                 $:  # Author of last commit. 
#  $LastChangedDate:: 2007-11-12 08:4      $:  # Date of last commit.
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
package DES::utils::hist;

use strict;
use warnings;

use FindBin;
use lib "$FindBin::Bin/../lib";
use DES::utils::misc;
use DES::desconfig;

use Exporter   ();
our (@ISA, @EXPORT);
@ISA         = qw(Exporter);
@EXPORT      = qw(&logEvent  &parseHistLog  &makeKeyStr);

sub logEvent {
   my $Config = shift;
   my $Stage = shift;
   my $SubStageType = shift;
   my $SubStage = shift;

   $Stage =~ s/"//g;
   if (defined($SubStageType)) {
      $SubStageType =~ s/"//g;
   }
   else {
      $SubStageType = '';
   }
   if (defined($SubStage)) {
      $SubStage =~ s/"//g;
   }
   else {
      $SubStage= '';
   }
  
   my %SearchDefs;
   $SearchDefs{"stage"} = $Stage;

   my $RunSite = $Config->getValue("run_site", \%SearchDefs);
   my $Nite = $Config->getValue("nite", \%SearchDefs);
   my $RunID = $Config->getValue("runid", \%SearchDefs);

   my $UniqName = $Config->getValue("uniqname", \%SearchDefs);
   my $OutputDir = $Config->getValue("outputdir", \%SearchDefs);

   my $DagID = 0;
   if (defined($ENV{"CONDOR_ID"})) {
      $DagID = $ENV{"CONDOR_ID"};
   }
   $DagID = sprintf("%d", $DagID);

   open LOG, ">> $OutputDir/$UniqName.deslog";
   print LOG getTimeStamp(), ",$DagID,$Nite,$RunID,$RunSite,$Stage,$SubStageType,$SubStage";

   # print rest of arguments to log
   for (my $i = 0; $i < scalar(@_); $i++) {
      print LOG ",",$_[$i];
   }
   print LOG "\n";
   close LOG;
}

#==============================================================================
#==============================================================================
sub parseHistLog {
   my $UniqName = shift;
   my $Iwd = shift;
   my $StageInfoArrRef = shift;

   my ($Date, $DagID, $Nite, $RunID, $RunSite, $Stage, $SubStageType, $SubStage, $Type);
   my (%StageNum, $InfoNum, $NextInfoNum);


   my $Config2 = new DES::desconfig;
   my $ConfigFile = "$Iwd/$UniqName.descfg";
   $Config2->readConfig($ConfigFile, 0, \*STDERR);

   my $Outputdir = $Config2->getValue("outputdir");

   $NextInfoNum = 0;

   # read .deslog
   my $DesLog = "$Outputdir/$UniqName.deslog";
   my $OpenSuccess = open DESLOG, "< $DesLog";
   if (!$OpenSuccess) {
      print STDERR "Could not open $DesLog for reading\n";
   }
   while (my $Line = <DESLOG>) {
      chomp($Line);
      $Line =~ s/,,/, ,/g;
      my @Info = split /,/, $Line;
      $Date = $Info[0];
      $DagID = $Info[1];
      $Nite = $Info[2];
      $RunID = $Info[3];
      $RunSite = $Info[4];
      $Stage = $Info[5];

      $UniqName = $Nite."_".$RunID;
      my $KeyStr = $UniqName."_".$RunSite;

      if (($Stage !~ /submit/i) && ($Stage !~ /complete/i) && ($Stage !~ /restart/i)) {
         # 11/07/2007 21:11:33,0,mmg2,test2,ncsa_mercury,analysis,j,mngr,pretask
         $SubStageType = $Info[6];
         $SubStage = $Info[7];
         $Type = $Info[8];

         $KeyStr = makeKeyStr("",$Nite, $RunID, $RunSite, $Stage, $SubStage, $SubStageType);
         if ($Type =~ /pretask/) {
            $StageNum{$KeyStr} = $NextInfoNum;
            $InfoNum = $NextInfoNum;

            $$StageInfoArrRef[$InfoNum]{"starttime"} = $Date;
            $$StageInfoArrRef[$InfoNum]{"uniqname"} = $UniqName;
            $$StageInfoArrRef[$InfoNum]{"substagetype"} = $SubStageType;
            $$StageInfoArrRef[$InfoNum]{"substage"} = $SubStage;
            $$StageInfoArrRef[$InfoNum]{"nite"} = $Nite;
            $$StageInfoArrRef[$InfoNum]{"runid"} = $RunID;
            $$StageInfoArrRef[$InfoNum]{"runsite"} = $RunSite;
            $$StageInfoArrRef[$InfoNum]{"stage"} = $Stage;
            $$StageInfoArrRef[$InfoNum]{"parent"} = $DagID;
            $$StageInfoArrRef[$InfoNum]{"jobstat"} = "PRE";
            $$StageInfoArrRef[$InfoNum]{"clusterid"} = "";
            $$StageInfoArrRef[$InfoNum]{"jobid"} = "";
            $$StageInfoArrRef[$InfoNum]{"exitval"} = $FAILURE;
            $$StageInfoArrRef[$InfoNum]{"endtime"} = "";

            $NextInfoNum++;
         }
         elsif ($Type =~ /cid/) {
            $InfoNum = $StageNum{$KeyStr};
            $$StageInfoArrRef[$InfoNum]{"clusterid"} = $Info[9];
         }
         elsif ($Type =~ /jobid/) {
            $InfoNum = $StageNum{$KeyStr};
            $$StageInfoArrRef[$InfoNum]{"jobid"} = $Info[9];
         }
         elsif ($Type =~ /posttask/) {
            $InfoNum = $StageNum{$KeyStr};
            $$StageInfoArrRef[$InfoNum]{"exitval"} = $Info[9];
            $$StageInfoArrRef[$InfoNum]{"endtime"} = $Date;
         }
      }
   }
   close DESLOG;
}

#==============================================================================
#==============================================================================
sub makeKeyStr {
   my $Num = shift;
   my $Nite = shift;
   my $RunID = shift;
   my $RunSite = shift;
   my $Stage = shift;
   my $SubStage = shift;
   my $SubStageType = shift;

   my $Key = "";

   if (defined($Num)) {
      $Key .= $Num;
   }
   $Key .= "__";
   if (defined($Nite)) {
      $Key .= $Nite;
   }
   $Key .= "__";
   if (defined($RunID)) {
      $Key .= $RunID;
   }
   $Key .= "__";
   if (defined($RunSite)) {
      $Key .= $RunSite;
   }
   $Key .= "__";
   if (defined($Stage)) {
      $Key .= $Stage;
   }
   $Key .= "__";
   if (defined($SubStage)) {
      $Key .= $SubStage;
   }
   $Key .= "__";
   if (defined($SubStageType)) {
      $Key .= $SubStageType;
   }

   return $Key;
}


1;
