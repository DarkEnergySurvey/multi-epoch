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
package DES::utils::desdbutils;

use Switch;
use FindBin;
use lib "$FindBin::Bin/../lib";
use DES::desconfig;
use DES::utils::dbutils;


use Exporter   ();
our (@ISA, @EXPORT);
@ISA         = qw(Exporter);
@EXPORT      = qw(&connectDESdb &getNiteSrcFileList 
                  &getNiteRawImageList &getDBTableInfo
                  &getFilesTableStats &getObjectsTableStats);

######################################################################
sub connectDESdb
{
   my $config = $_[0];

   my $desdb = new DES::utils::dbutils;
   my $verbose = $config->getValue("verbose");

   if (defined($verbose) && ($verbose != 0))
   {
      $desdb->setVerbose($verbose);
   }

   my $usercfgdir = $config->getValueReq("usercfgdir");
   if (-r "$usercfgdir/.desdb.conf")
   {
      $desdb->connectUsingCfgFile("$usercfgdir/.desdb.conf");
   }
   elsif (-r "$usercfgdir/.desdm")
   {
      $desdb->connectUsingCfgFile("$usercfgdir/.desdm");
   }
   elsif (-r "$ENV{'HOME'}/.desdm")
   {
      $desdb->connectUsingCfgFile("$ENV{'HOME'}/.desdm");
   }
   else
   {
      print STDERR "ERROR: Cannot read DES database config file\n";
      print STDERR "Connection Aborted\n";
      exit 1;
   }

   return $desdb;
}

######################################################################
sub getNiteSrcFileList
{
   my $nite = $_[0];
   my $desdb = $_[1];
   my $namelist = $_[2];
   my $src_image_rows;

   # Query to get nite's src file listing:
   my $dbq = <<SRC;
SELECT IMAGENAME
FROM FILES
WHERE NITE='$nite'
AND LOWER(IMAGETYPE)='src'
SRC

   $src_image_rows = $desdb->get_query_rows_hash($dbq);

   if (defined($verbose) && ($verbose >= 1))
   {
     if (scalar(@$src_image_rows)==0)
     {
        print "\tNo src files for nite: $nite\n";
     }
     else
     {
        print "\tListing of src files for nite: $nite\n";
        foreach my $row (@$src_image_rows)
        {
           print "\t\t", $row->{'IMAGENAME'}, "'\n";
        }
     }
   }

   # Just want image names
   foreach my $row (@$src_image_rows)
   {
      push (@$namelist,$row->{'IMAGENAME'});
   }
}

################################################################################
# SUBROUTINE: get_archive_node_info
################################################################################
#sub get_archive_node_info {
#  my $this = shift;
#  my $archive_node = shift;
#  
#  my $dbq = <<STR;
#  SELECT * FROM ARCHIVE_SITES WHERE LOCATION_NAME='$archive_node'
#STR
#
#  my $sth = $this->{dbh}->prepare($dbq);
#  $sth->execute();
#  my $row = $sth->fetchrow_hashref();
#  return $row;
#}


################################################################################
# SUBROUTINE: getNiteRawImageList
################################################################################

sub getNiteRawImageList  {
   my $nite = $_[0];
   my $desdb = $_[1];
   my $namelist = $_[2];
   my $src_image_rows;

   # Query to get nite's src file listing:
   my $dbq = <<SRC;
SELECT IMAGENAME
FROM FILES
WHERE NITE='$nite'
AND IMAGECLASS='raw'
AND IMAGETYPE!='src'
SRC

   $src_image_rows = $desdb->get_query_rows_hash($dbq);

   if (defined($verbose) && ($verbose >= 1))
   {
     if (scalar(@$src_image_rows)==0)
     {
        print "\tNo raw files for nite: $nite\n";
     }
     else
     {
        print "\tListing of raw files for nite: $nite\n";
        foreach my $row (@$src_image_rows)
        {
           print "\t\t", $row->{'IMAGENAME'}, "'\n";
        }
     }
   }

   # Just want image names
   foreach my $row (@$src_image_rows)
   {
      push (@$namelist,$row->{'IMAGENAME'});
   }

}


################################################################################
# SUBROUTINE: getDBTableInfo
################################################################################
sub getDBTableInfo {
   my $desdb = shift;
   my $tablestr = shift;

   $tablestr =~ tr/A-Z/a-z/;
   switch ($tablestr)
   {
      case /sites/ { $tablestr = "SITES"; }
      case /archive/ { $tablestr = "ARCHIVE_SITES"; }
      case /software/ { $tablestr = "SOFTWARE_LOCATIONS"; }
      else {
         print STDERR "ERROR: unknown table ($tablestr)\n";
         exit $FAILURE;
      }
   }

   my $dbq = <<STR;
SELECT * FROM $tablestr
STR

   my $rows = $desdb->get_query_rows_hash($dbq);

   return $rows
}


################################################################################
# SUBROUTINE: getFilesTableStats
################################################################################
sub getFilesTableStats {
  my $nite = shift;
  my $desdb = shift;

  my %hash;

  # Query to get count of src file entries for a given nite:
  my $dbq = <<STR;
  SELECT COUNT(IMAGEID) FROM FILES WHERE NITE='$nite' AND IMAGETYPE='src'
STR
  $hash{'raw_src'} = $desdb->get_single_result($dbq);

  # Query to get count of converted raw image entries for a given nite:
  $dbq = <<STR;
  SELECT COUNT(IMAGEID) FROM FILES WHERE NITE='$nite' AND IMAGETYPE!='src' AND IMAGECLASS='raw'
STR
  $hash{'raw_raw'} = $desdb->get_single_result($dbq);

  # Query to get count of reduced image entries fro a given nite:
  $dbq = <<STR;
  SELECT COUNT(IMAGEID) FROM FILES WHERE NITE='$nite' AND IMAGETYPE='reduced'
STR
  $hash{'reduced'} = $desdb->get_single_result($dbq);

  return \%hash;

}
################################################################################
# SUBROUTINE: getObjectsTableStats
################################################################################
sub getObjectsTableStats {
  my $nite = shift;
  my $desdb = shift;

  my %hash;

  my $dbq = <<STR;
  SELECT COUNT (OBJECT_ID) FROM OBJECTS,FILES WHERE FILES.IMAGEID=OBJECTS.IMAGEID AND FILES.NITE='$nite'
STR

  $hash{'objects'} = $desdb->get_single_result($dbq);

  return \%hash;
}

1;
