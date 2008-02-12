########################################################################
#  $Id: desutils.pm 552 2007-12-05 18:52:48Z dadams $
#
#  $Rev:: 552                              $:  # Revision of last commit.
#  $LastChangedBy:: dadams                 $:  # Author of last commit. 
#  $LastChangedDate:: 2007-12-05 12:5      $:  # Date of last commit.
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
package DES::utils::email;

use strict;
use warnings;

use FindBin;
use lib "$FindBin::Bin/../lib";
use DES::desDBI;
use DES::utils::misc;
use DES::desconfig;

use Exporter   ();
our (@ISA, @EXPORT);
@ISA         = qw(Exporter);
@EXPORT      = qw(&sendEmail);



#################################################################
sub sendEmail {
   my $Config = shift;
   my $desdb = shift;
   my $Stage = shift;
   my $Status = shift;
   my $Msg1 = shift;
   my $Msg2 = shift;
 
   my $Email = $Config->getValue("email");
   if ($Email =~ /\S/) {
      my $MailFile = "email_$Stage.txt";
      open MAIL, "> $MailFile";

      print MAIL "**************************************************************\n";
      print MAIL "*                                                            *\n";
      print MAIL "*  This is an automated message from DES.  Do not reply.     *\n";
      print MAIL "*                                                            *\n";
      print MAIL "**************************************************************\n";
      print MAIL "\n";

      print MAIL "$Msg1\n";

      my $user = $ENV{"USER"};
      my $localmachine = `/bin/hostname -f`;
      chomp($localmachine);
      print MAIL "User = $user\@$localmachine\n";
      print MAIL "Local directory = ", $Config->getValue("submitdir"),"\n\n";

      my %SearchDefs;
      $SearchDefs{"stage"} = $Stage;
      my $RunSite = $Config->getValue("run_site", \%SearchDefs);
      $SearchDefs{"site"} = $RunSite;
      my $RunArchiveLoc = $Config->getValue("run_archive_loc", \%SearchDefs);
      $SearchDefs{"archive"} = $RunArchiveLoc;
      my $RunSoftwareLoc = $Config->getValue("run_software_loc", \%SearchDefs);
      $SearchDefs{"software"} = $RunSoftwareLoc;

      my $Nite = $Config->getValue("nite");
      print MAIL "Nite = $Nite\n";
      print MAIL "Runid = ", $Config->getValue("runid"), "\n";
      print MAIL "Run site = $RunSite\n";
      print MAIL "Run archive location = $RunArchiveLoc\n";
      print MAIL "Archive root = ", $Config->getValue("archive_root", \%SearchDefs), "\n";
      print MAIL "\n\n";

      # print DB stats
      if (!defined($desdb))
      {
         $desdb = new DES::desDBI;
      }
      print MAIL "DATABASE STATS\n";
      my %FStats;
      print MAIL "\nFILES Table:\n";
      $desdb->getFilesTableStats($Nite, \%FStats);
      while ((my $key, my $value) = each %FStats) {
         print MAIL "$value $key entries.\n";
      }
      print MAIL "\nOBJECTS Table:\n";
      my %OStats;
      $desdb->getObjectsTableStats($Nite, \%OStats);
      while ((my $key, my $value) = each %OStats) {
         print MAIL "$value $key entries.\n";
      }
      print MAIL "\n\n";

      if (defined($Msg2)) {
         print MAIL "$Msg2\n";
      }

      close MAIL;

      my $UniqName = $Config->getValue("uniqname");
      my $Subject = "DES: $UniqName $Stage";
      if ($Status != $SUCCESS) {
         $Subject .= " [FAILED]";
      }

      debug("$Stage", "Sending email to $Email");
      `/bin/mail -s "$Subject" $Email < $MailFile`;
      open MAIL, "< $MailFile";
      debug("$Stage", "Subject: $Subject");
      while (<MAIL>) {
         debug("$Stage", "$_");
      }
      close MAIL;
      unlink "$MailFile";
   }
}


1;
