########################################################################
#
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
################################################################################
################################################################################
package NCSA::wrapDBI;
use strict;
use warnings;
use Switch;

use DBI;
use vars qw(@ISA);
our @ISA = qw(DBI);


################################################################################
#
################################################################################
sub connectFromConfigFile {
  my $this = shift;
  my $dbcfgfile = shift;

#  my $class = ref($this) || $this;

  my ($db_type, $db_server, $db_user, $db_pass, $db_name);
 
  my ($line, $left, $right);

  if (! -r $dbcfgfile)
  {
     print STDERR "ERROR: Unable to read db configuration file: '$dbcfgfile'\n";
     exit 1;
  }

  # Knock permissions back to 0600 to keep passwords safe.
  # TODO - Check current permissions and warn user if not strong enough
  chmod 0600, $dbcfgfile;

  open(FH, "< $dbcfgfile") or die "ERROR: Unable to open configuration file: '$dbcfgfile'.\n";

  my $linenum = 0;
  while($line = <FH>) {
    $linenum++;
    $line =~  s/^\s+//g;
    $line =~ s/\s+$//g;
    if (($line =~ /\S/) && ($line !~ /^#/))
    {
       if ($line =~ /=/)
       {
          ($left, $right) = $line =~ m/^(\S+)\s*=\s*(\S.*)$/;
       }
       else
       {
          ($left, $right) = $line =~ m/^(\S+)\s*(.*)$/;
       }
       $left =~ tr/A-Z/a-z/;
       switch ($left)
       {
          case /^db_type$/i { $db_type = $right; }
          case /^db_server$/i { $db_server = $right; }
          case /^db_name$/i { $db_name = $right; }
          case /^db_user$/i { $db_user = $right; }
          case /^db_pass$/i { $db_pass = $right; }
          case /^DB_PASSWD$/i { $db_pass = $right; }
          else  
          {
             print STDERR "ERROR: Unrecognized line in db config ('$dbcfgfile')\n";
             print STDERR "Line number $linenum: '$line'\n";
             exit 1;
          }
       }
    }
  }
  close FH;

  # check that have all important data
#  my $key;
#  #foreach $key (("db_type", "db_server", "db_name", "db_user", "db_pass"))
#  foreach $key (("db_server", "db_name", "db_user", "db_pass"))
#  {
#    if (!defined($self->{"$key"}))
#    {
#       print STDERR "ERROR: Missing value for $key\n";
#       exit 1;
#    }
#  }

  # Connect

  my $self = $this->SUPER::connect("DBI:Oracle:host=$db_server;sid=$db_name",$db_user,$db_pass)
            or die "Database connection error: $DBI::errstr\n";
  print "\nSuccessfully connected to the '$db_name' database on $db_server.\n";
  
 #bless $self, $class;
  return $self;
}


package NCSA::wrapDBI::db;
use vars qw(@ISA);
our @ISA = qw(DBI::db);

################################################################################
# SUBROUTINE: get_row
# Returns a string that is the result of supplied query.  Use when expecting 
# only one field from the query.
################################################################################
sub get_row {
  my $dbh = shift;
  my $dbq = shift;

#  my $sth = $dbh->prepare($dbq);
#  $sth->execute();
#  my $row = $sth->fetchrow_array();
#  $sth->finish();
  my $row = $dbh->selectrow_array($dbq);
  chomp $row  if $row;               # Remove trailing newline.
  $row =~ s/^\s*//g if $row;         # Remove leading whitespace.
  $row =~ s/\s*$//g if $row;         # Remove trailing whitespace.
  return $row;
}



#sub setVerbose {
#  my $self = shift;
#  my $verbose = shift;
#  $self->{"verbose"} = $verbose;
#}
#
#sub connectUsingVars {
#  my $self = shift;
#  my $db_type = shift;
#  my $db_server = shift;
#  my $db_name = shift;
#  my $db_user = shift;
#  my $db_pass = shift;
#
#  $self->{"db_type"} = $db_type;
#  $self->{"db_server"} = $db_server;
#  $self->{"db_name"} = $db_name;
#  $self->{"db_user"} = $db_user;
#  $self->{"db_pass"} = $db_pass;
#
#  $self->connectDB();
#}
#
#sub connectUsingCfgFile {
#  my $self = shift;
#  my $dbcfgfile = shift;
#
#  $self->read_db_config_file($dbcfgfile);
#  $self->connectDB();
#}
#
#sub connectDB {
#  my $this = shift;
#
#  my $db_type; 
#  if (defined $this->{"db_type"}) {
#    $db_type = $this->{"db_type"}; 
#  }
#  else {
#    $db_type='oracle';
#  }
#  my $db_server = $this->{"db_server"};
#  my $db_name = $this->{"db_name"};
#  my $db_user = $this->{"db_user"};
#  my $db_pass = $this->{"db_pass"};
#
#  if ($db_type eq 'mysql') {
#    # Connect to mysql database.
#    $this->{'dbh'} = DBI->connect("DBI:mysql:$db_name:$db_server",$db_user,$db_pass)
#            or die "Database connection error: $DBI::errstr\n";
#    if ($this->{"verbose"} > 0) {
#      print "\nSuccessfully connected to the \"$this->{'db_name'}\" database.\n";
#    }
#  }
#  elsif ($db_type eq 'oracle') {
#    # Connect to Oracle database.
#    #$this->{'dbh'} = DBI->connect("DBI:Oracle:$db_name:$db_server",$db_user,$db_pass)
#    $this->{'dbh'} = DBI->connect("DBI:Oracle:host=$db_server;sid=$db_name",$db_user,$db_pass)
#            or die "Database connection error: $DBI::errstr\n";
#    if ($this->{"verbose"} > 0) {
#      print "\nSuccessfully connected to the '$db_name' database on $db_server.\n";
#    }
#  } 
#  else {
#    die "\nUnable to connect to database type: 'db_type'.\nExiting...\n";
#  }
#}
#
#
#################################################################################
## SUBROUTINE: read_db_config_file
##
#################################################################################
#sub read_db_config_file {
#  my $self = shift;
#  my $dbcfgfile = shift;
#  my ($line, $left, $right);
#
#  if (! -r $dbcfgfile)
#  {
#     print STDERR "ERROR: Unable to read db configuration file: '$dbcfgfile'\n";
#     exit 1;
#  }
#
#  # Knock permissions back to 0600 to keep passwords safe.
#  # TODO - Check current permissions and warn user if not strong enough
#  chmod 0600, $dbcfgfile;
#
#  open(FH, "< $dbcfgfile") or die "ERROR: Unable to open configuration file: '$dbcfgfile'.\n";
#
#  my $linenum = 0;
#  while($line = <FH>) {
#    $linenum++;
#    $line =~  s/^\s+//g;
#    $line =~ s/\s+$//g;
#    if (($line =~ /\S/) && ($line !~ /^#/))
#    {
#       if ($line =~ /=/)
#       {
#          ($left, $right) = $line =~ m/^(\S+)\s*=\s*(\S.*)$/;
#       }
#       else
#       {
#          ($left, $right) = $line =~ m/^(\S+)\s*(.*)$/;
#       }
#       $left =~ tr/A-Z/a-z/;
#       switch ($left)
#       {
#          case /^db_type$/i { $self->{"db_type"} = $right; }
#          case /^db_server$/i { $self->{"db_server"} = $right; }
#          case /^db_name$/i { $self->{"db_name"} = $right; }
#          case /^db_user$/i { $self->{"db_user"} = $right; }
#          case /^db_pass$/i { $self->{"db_pass"} = $right; }
#          case /^DB_PASSWD$/i { $self->{"db_pass"} = $right; }
#          else  
#          {
#             print STDERR "ERROR: Unrecognized line in db config ('$dbcfgfile')\n";
#             print STDERR "Line number $linenum: '$line'\n";
#             exit 1;
#          }
#       }
#    }
#  }
#  close FH;
#
#  # check that have all important data
#  my $key;
#  #foreach $key (("db_type", "db_server", "db_name", "db_user", "db_pass"))
#  foreach $key (("db_server", "db_name", "db_user", "db_pass"))
#  {
#    if (!defined($self->{"$key"}))
#    {
#       print STDERR "ERROR: Missing value for $key\n";
#       exit 1;
#    }
#  }
#}
#
#

################################################################################
# SUBROUTINE: get_single_result
#
################################################################################
sub getSingleResult {
  my $this = shift;
  my $dbq = shift;

  my $Result = $this->get_row($dbq);
  return $Result;
}
#
#
#
package NCSA::wrapDBI::st;
use vars qw(@ISA);
our @ISA = qw(DBI::st);
1;
