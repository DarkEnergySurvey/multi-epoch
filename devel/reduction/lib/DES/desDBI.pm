########################################################################
#  $Id:                                    $
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

package DES::desDBI;
use strict;
use warnings;
use Switch;

use NCSA::wrapDBI;
use vars qw(@ISA);
our @ISA = qw(NCSA::wrapDBI);

sub new {
  my $this = shift;
  my $class = ref($this) || $this;
  my $self = $class->SUPER::connectFromConfigFile($ENV{"HOME"}.'/.desdm');
  return $self;
}

sub DESTROY {
  my $self = shift;
  warn "DESTROYING $self";
  $self->disconnect();
}

package DES::desDBI::db;
use vars qw(@ISA);
our @ISA = qw(NCSA::wrapDBI::db);

################################################################################
# SUBROUTINE: getNiteRawImageList
################################################################################
sub getNiteRawImageList  {
   my $desdb = shift;
   my $Nite = shift;
   my $NameListRef = shift;

   my $Verbose = 1;

   # Query to get nite's src file listing:
   my $dbq = <<SRC;
SELECT IMAGENAME
FROM FILES
WHERE NITE='$Nite'
AND IMAGECLASS='raw'
AND IMAGETYPE!='src'
SRC

   my $Rows = $desdb->selectall_arrayref($dbq,{Slice => {}});

   if (defined($Verbose) && ($Verbose >= 1))
   {
     if (scalar(@$Rows)==0)
     {
        print "\tNo raw files for nite: $Nite\n";
     }
     else
     {
        print "\tListing of raw files for nite: $Nite\n";
        foreach my $Row (@$Rows)
        {
           print "\t\t", $Row->{'IMAGENAME'}, "'\n";
        }
     }
   }

   # Just want image names
   foreach my $Row (@$Rows)
   {
      push (@$NameListRef,$Row->{'IMAGENAME'});
   }

}

######################################################################
#
######################################################################
sub getNiteSrcImageList {
   my $desdb = shift;
   my $Nite = shift;
   my $NameListRef = shift;

   my $Verbose = 1;

   # Query to get nite's src file listing:
   my $dbq = <<SRC;
SELECT IMAGENAME
FROM FILES
WHERE NITE='$Nite'
AND LOWER(IMAGETYPE)='src'
SRC
 
#   my $Rows = $desdb->getAllRowsHashRefs($dbq);
   my $Rows = $desdb->selectall_arrayref($dbq,{Slice => {}});
 
   if (defined($Verbose) && ($Verbose >= 1))
   {
     if (scalar($Rows)==0)
     {
        print "\tNo src files for nite: $Nite\n";
     }
     else
     {
        print "\tListing of src files for nite: $Nite\n";
        foreach my $Row (@$Rows)
        {
           print "\t\t'", $Row->{'IMAGENAME'}, "'\n";
        }
     }
   }
 
   # Just want image names
   foreach my $Row (@$Rows)
   {
      push (@$NameListRef,$Row->{'IMAGENAME'});
   }
}

################################################################################
# SUBROUTINE: getDBTableInfo
#
# To make this generic actually look up the primary key from the DB 
# rather than hard coding what we know it to be
################################################################################
sub getDBTableInfo {
   my $desdb = shift;
   my $tablestr = shift;
   my $RowsRef = shift;

   my $pkey;
   $tablestr =~ tr/A-Z/a-z/;
   switch ($tablestr) {
      case /site/ { 
        $tablestr = "SITES"; 
        $pkey = 'SITE_NAME';
      }
      case /archive/ { 
        $tablestr = "ARCHIVE_SITES"; 
        $pkey = 'LOCATION_NAME';
      }
      case /software/ { 
        $tablestr = "SOFTWARE_LOCATIONS"; 
        $pkey = 'LOCATION_NAME';
      }
      else {
         print STDERR "ERROR: unknown table ($tablestr)\n";
      }
   }

   my $dbq = <<STR;
SELECT * FROM $tablestr
STR

   @$RowsRef = @{$desdb->selectall_arrayref($dbq,{Slice => {}})};
  return $pkey;
}


################################################################################
# SUBROUTINE: getFilesTableStats
################################################################################
sub getFilesTableStats {
  my $desdb = shift;
  my $Nite = shift;
  my $Stats = shift;

  # Query to get count of src file entries for a given nite:
  my $dbq = <<STR;
  SELECT COUNT(IMAGEID) FROM FILES WHERE NITE='$Nite' AND IMAGETYPE='src'
STR
  $$Stats{'raw_src'} = $desdb->getSingleResult($dbq);

  # Query to get count of converted raw image entries for a given nite:
  $dbq = <<STR;
  SELECT COUNT(IMAGEID) FROM FILES WHERE NITE='$Nite' AND IMAGETYPE!='src' AND IMAGECLASS='raw'
STR
  $$Stats{'raw_raw'} = $desdb->getSingleResult($dbq);

  # Query to get count of reduced image entries fro a given nite:
  $dbq = <<STR;
  SELECT COUNT(IMAGEID) FROM FILES WHERE NITE='$Nite' AND IMAGETYPE='reduced'
STR
  $$Stats{'reduced'} = $desdb->getSingleResult($dbq);

}

################################################################################
# SUBROUTINE: getObjectsTableStats
################################################################################
sub getObjectsTableStats {
  my $desdb = shift;
  my $Nite = shift;
  my $Stats = shift;
  
  # Query to get count of objects for a particular nite:  
  my $dbq = <<STR;
  SELECT COUNT (OBJECT_ID) FROM OBJECTS,FILES WHERE FILES.IMAGEID=OBJECTS.IMAGEID AND FILES.NITE='$Nite'
STR
  
  $$Stats{'objects'} = $desdb->getSingleResult($dbq);
  
}



################################################################################
################################################################################

sub ingestSrcFileInfo {
  my $desdb = shift;
  my $Nite = shift;
  my $ImageListRef = shift;

  my $Verbose = 1;



  # Prepare Insert command:
  my $dbi = "INSERT into FILES (IMAGEID, IMAGETYPE, NITE, IMAGECLASS, IMAGENAME) VALUES ( files_seq.nextval,'src','$Nite','raw',?)";

  if (defined($Verbose)) {
     print "Using:\n$dbi\nFor imagenames:\n";
     foreach(@$ImageListRef) {
        print "\t$_\n";
     }
  }

  $desdb->{AutoCommit} = 0;
  my $sth = $desdb->prepare($dbi);

  # Execute insert for all image names:
  my $Tuples = $sth->execute_array(
  { ArrayTupleStatus => \my @TupleStatus },
  $ImageListRef
  );
  $desdb->commit();
  $desdb->{AutoCommit} = 1;

  return $Tuples;
}

################################################################################
# SUBROUTINE: ingestRawFileInfo
################################################################################
sub ingestRawFileInfo {
  my $desdb = shift;
  my $Detector = shift;
  my $ArchiveSitesStr = shift;
  my $Values = shift;

print "ingestRawFileInfo:\n";
for (my $j = 0; $j < scalar(@$Values); $j++) {
   print "$j: ";
   my $arrref = $$Values[$j];
   for (my $k = 0; $k < scalar(@$arrref); $k++) {
      print "\t'", $$arrref[$k], "'";
   }
   print "\n\n";
}

  my $dbi;
  if ($Detector eq 'DECam') {
  $dbi = <<STR;
INSERT INTO FILES (
RA        ,
DEC       ,
RADECEQ   ,
File_Date  ,
GAIN_A    ,
RDNOISE_A ,
GAIN_B    ,
RDNOISE_B ,
Airmass   ,
Band       ,
IMAGECLASS  ,
ImageType  ,
IMAGENAME  ,
Nite       ,
CCD_Number ,
exptime    ,
DARKTIME   ,
OBJECT       ,
OBSERVATORY  ,
TELESCOPE    ,
HOURANGLE    ,
ZENITHD      ,
DETECTOR     ,
OBSERVER     ,
PROPID       ,
WEATHERDATE  ,
WINDSPD      ,
WINDDIR      ,
AMBTEMP      ,
HUMIDITY     ,
PRESSURE     ,
DIMMSEEING   ,
EQUINOX   ,
WCSDIM    ,
CTYPE1   ,
CTYPE2   ,
CRVAL1    ,
CRVAL2    ,
CRPIX1    ,
CRPIX2    ,
CD1_1    ,
CD2_1    ,
CD1_2    ,
CD2_2    ,
PV1_0    ,
PV1_1    ,
PV1_2    ,
PV1_3    ,
PV1_4    ,
PV1_5    ,
PV1_6    ,
PV1_7    ,
PV1_8    ,
PV1_9    ,
PV1_10   ,
PV2_0    ,
PV2_1    ,
PV2_2    ,
PV2_3    ,
PV2_4    ,
PV2_5    ,
PV2_6    ,
PV2_7    ,
PV2_8    ,
PV2_9    ,
PV2_10   ,
NPIX1   ,
NPIX2   ,
NEXTEND ,
PHOTFLAG ,
ARCHIVESITES ,
IMAGEID ,
HTMID ,
CX    ,
CY    ,
CZ    )

VALUES (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11,:12,:13,:14,:15,:16,:17,:18,:19,:20,:21,:22,:23,:24,:25,:26,:27,:28,:29,:30,:31,:32,:33,:34,:35,:36,:37,:38,:39,:40,:41,:42,:43,:44,:45,:46,:47,:48,:49,:50,:51,:52,:53,:54,:55,:56,:57,:58,:59,:60,:61,:62,:63,:64,:65,:66,:67,:68,:69,:70,'$ArchiveSitesStr',files_seq.nextval,fEqToHtm(:1,:2),fEqToX(:1, :2),fEqToY(:1, :2),fEqToZ(:1, :2))
STR
}
elsif ($Detector eq 'Mosaic2') {
$dbi = <<STR;
INSERT INTO FILES (
RA        ,
DEC       ,
RADECEQ   ,
File_Date ,
GAIN_A    ,
RDNOISE_A ,
GAIN_B    ,
RDNOISE_B ,
Airmass   ,
Band      ,
IMAGECLASS  ,
ImageType  ,
IMAGENAME  ,
Nite       ,
CCD_Number ,
exptime    ,
DARKTIME   ,
OBJECT      ,
OBSERVATORY ,
TELESCOPE   ,
HOURANGLE   ,
ZENITHD     ,
DETECTOR    ,
OBSERVER    ,
PROPID      ,
WEATHERDATE ,
WINDSPD     ,
WINDDIR     ,
AMBTEMP     ,
HUMIDITY    ,
PRESSURE    ,
DIMMSEEING  ,
EQUINOX   ,
WCSDIM    ,
CTYPE1  ,
CTYPE2  ,
CRVAL1    ,
CRVAL2    ,
CRPIX1    ,
CRPIX2    ,
CD1_1    ,
CD2_1    ,
CD1_2    ,
CD2_2    ,
NPIX1   ,
NPIX2   ,
NEXTEND ,
PHOTFLAG ,
ARCHIVESITES ,
IMAGEID ,
HTMID   ,
CX      ,
CY      ,
CZ      )

VALUES (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11,:12,:13,:14,:15,:16,:17,:18,:19,:20,:21,:22,:23,:24,:25,:26,:27,:28,:29,:30,:31,:32,:33,:34,:35,:36,:37,:38,:39,:40,:41,:42,:43,:44,:45,:46,:47,:48,'$ArchiveSitesStr',files_seq.nextval,fEqToHtm(:1,:2),fEqToX(:1, :2),fEqToY(:1, :2),fEqToZ(:1, :2))

STR

}

# Turn off outo commit
$desdb->{AutoCommit} = 0;

# Prepare Insert statement:
my $sth = $desdb->prepare($dbi);

# Execute insert:
my $t1 = time;
my $tuples = $sth->execute_array({ ArrayTupleStatus => \my @tuple_status },@$Values);
$desdb->commit();
my $t2 = time;
if ($tuples) {
  printf ("\nSuccessfully inserted %s rows in %s seconds.\n",$tuples,$t2-$t1);
}

$desdb->{AutoCommit} = 1;
#$desdb->disconnect;

return $tuples
}

package DES::desDBI::st;
use vars qw(@ISA);
our @ISA = qw(NCSA::wrapDBI::st);
