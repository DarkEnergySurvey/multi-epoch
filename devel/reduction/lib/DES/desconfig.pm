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
#  with useful conversations with Jordi Cohen and Jim Phillips from the
#  Theoretical and Computation Biophysics Group at the University
#  of Illinois at Urbana-Champaign.
#
#  Copyright (C) 2007 Board of Trustees of the University of Illinois. 
#  All rights reserved.
#
#  DESCRIPTION:
#
#######################################################################

package DES::desconfig;

use warnings;
use strict;

use Switch;
###use File::Basename;
use Cwd;

use FindBin;
use lib "$FindBin::Bin/../lib";
use DES::desDBI;
use DES::utils::misc;


use Exporter   ();
our (@ISA, @EXPORT);
@ISA         = qw(Exporter);
@EXPORT      = qw($SUCCESS $REPEAT $FAILURE);

# exit codes especially useful for nodes in DAG
our $SUCCESS = 0;
our $REPEAT = 100;
our $DONTREPEAT = 1;
our $FAILURE = 1;

# flags for whether to allow spaces in values
my $NOSPACE = 1;
my $ALLOWSPACE = 0;
my $NUMERIC = 2;

my %defs;
my %uservars;    # extra keys/values that are user defined
my @deforder;    # order in which to search for values 
my @stageorder;  # user-specified order in which stages are executed 

##########################################################################
sub new 
{
  my $this = shift;
  my $class = ref($this) || $this;
  my $self = {};
  bless $self, $class;
  $self->initialize();
  return $self;
}

# set defaults
sub initialize
{
  my $self = shift;

  $self->{"defs"}{"global"}{"__numstage"} = 0;
  $self->{"defs"}{"global"}{"__numsite"} = 0;
  $self->{"defs"}{"global"}{"__numarchive"} = 0;
  $self->{"defs"}{"global"}{"__numsoftware"} = 0;

  $self->{"defs"}{"global"}{"verbose"} = 0;

  my $homedir = $ENV{"HOME"};
  $self->{"defs"}{"global"}->{"usercfgdir"} = "$homedir/.des";

#  @{$self->{"deforder"}} = ("stage", "archive", "software", "site");
  @{$self->{"deforder"}} = ("stage", "archive", "site");
}

sub fillBlanks 
{
  my $self = shift;

  # uniqname - nite_runid
  $self->{"defs"}{"global"}->{"uniqname"} = $self->getValue("nite")."_".$self->getValue("runid");

  # TODO - set default port values

  my $uniqname = $self->getValue("uniqname");
  my $cwd = cwd();
  my $internaldir = $cwd."/DESjobs/$uniqname/internal";
  my $outputdir = $cwd."/DESjobs/$uniqname/output";
  $self->{"defs"}{"global"}->{"submitdir"} = $cwd;
  $self->{"defs"}{"global"}->{"internaldir"} = $internaldir;
  $self->{"defs"}{"global"}->{"outputdir"} = $outputdir;
  $self->{"defs"}{"global"}->{"daglog"} = "$internaldir/$uniqname.log";
  $self->{"defs"}{"global"}->{"debuglog"} = "$outputdir/${uniqname}_debug.log";
  $self->{"defs"}{"global"}->{"allcondorlogs"} = "$outputdir/${uniqname}_condor.log";
}



##########################################################################
sub writeConfig
{
   my $self = shift;
   my $fh = shift;

   my ($d, $dt);

   local * writeConfigVals = sub
   {
     my $hashref = shift;
     my $hashrefvars = shift;
     my $fh = shift;
     my $k;


     foreach $k (sort keys %{$hashref})
     {
        if ($k !~ /^__/)
        {
           print $fh "\t$k = ", $hashref->{"$k"}, "\n";
        }
     }
     foreach $k (sort keys %{$hashrefvars})
     {
        print $fh "\tVAR $k = ", $hashrefvars->{"$k"}, "\n";
     }
   };

   # global doesn't have multiple defs
   writeConfigVals(\%{$self->{"defs"}{"global"}}, \%{$self->{"uservars"}{"global"}}, $fh);

   foreach $d (@{$self->{"deforder"}})
   {
      foreach $dt (sort keys %{$self->{"defs"}{"$d"}})
      {
         print $fh "def $d $dt\n";
         writeConfigVals(\%{$self->{"defs"}{"$d"}{"$dt"}}, \%{$self->{"uservars"}{"$d"}{"$dt"}}, $fh);
         print $fh "enddef\n";
      }
   }

   for (my $i = 0; $i < scalar(@{$self->{"stageorder"}}); $i++)
   {
      print $fh "stage ", $self->{"stageorder"}[$i], "\n";
   }

}


##########################################################################
sub readConfig
{
  my $self = shift;
  my $desconfigfile = shift;
  my $checkkeys = shift;
  my $errfh = shift;

  my %tempdef;
  my %tempvars;
  my ($def, $defname);

  local * saveDef = sub
  {
     my ($k);

#print "saveDef def = $def, defname = $defname\n";

     if ((scalar(keys %tempdef) > 0) || (scalar(keys %tempvars) > 0))
     {
        if (defined($defname))
        {
           if (!defined($self->{"defs"}{"$def"}{"$defname"}))
           {
#print "defs{$def}{$defname} not defined\n";
              $self->{"defs"}{"global"}{"__num$def"}++;
           }
           $self->{"defs"}{"$def"}{"$defname"}{"__name"} = $defname;
           foreach $k (keys %tempdef)
           {
#print "saving val $k\n";
              $self->{"defs"}{"$def"}{"$defname"}{"$k"} = $tempdef{"$k"};
           }
           foreach $k (keys %tempvars)
           {
#print "saving var $k\n";
              $self->{"uservars"}{"$def"}{"$defname"}{"$k"} = $tempvars{"$k"};
           }
        }
     }
     %tempdef = ();
     undef %tempdef;
     %tempvars = ();
     undef %tempvars;

     $def = "global";
  };

  local * checkSyntax = sub
    {
       my $k = shift;
       my $v = shift;
       my $sp = shift;

       if (($sp != $ALLOWSPACE) && ($v =~ /\s/))
       {
          print $errfh "ERROR: Whitespace not allowed in $k ($v)\n";
          exit $FAILURE;
       }
       if (($sp == $NUMERIC) && ($v =~ /\D/))
       {
          print $errfh "ERROR: $k is required to be a numeric value ($v)\n";
          exit $FAILURE;
       }
    };

  local * storeVal = sub
    {
      my $k = shift;
      my $v = shift;
      my $t = shift;
      my $sp = shift;
      my $ref;


      checkSyntax($k, $v, $sp);

#print "storeVal: k = $k, v = $v, t = $t, def = $def\n";
      if ($k eq "defname")
      {
         $defname = $v; 
      }
      elsif ($t eq "var")
      {
         if ($def eq "global")
         {
            $ref = \%{$self->{"uservars"}{"global"}};
         }
         else
         {
            $ref = \%tempvars;
         }
      }
      else
      {
         if ($def eq "global")
         {
            $ref = \%{$self->{"defs"}{"global"}};
         }
         else
         {
            $ref = \%tempdef;
         }
      }
#      if (defined($ref->{"$k"}))
#      {
#         print "ERROR: Duplicate $def key: '$k'\n";
#         print "Found in config '$desconfigfile'\n";
#
#         print "\tFirst: ", $ref->{"$k"}, "\n";
#         print "\tSecond: ", $v, "\n";
#         exit $FAILURE;
#      }
#      else
#      {
         $ref->{"$k"} = $v;
#      }
    };


  local * addStageOrder = sub
  {
     my $stagename = shift;
     my $s;
     my $found = 0;
  
     foreach $s (@{$self->{"stageorder"}})
     {
        if ($s eq $stagename)
        {
           $found = 1;
           last;
        } 
     } 

     if ($found)
     {
        print $errfh "WARNING: Duplicate stage '$stagename'\n";
     }

     $self->{"defs"}{"global"}{"__numstage"}++;
     push(@{$self->{"stageorder"}}, $stagename); 
  };

  #####
  my ($line, $linenum);
  my ($left, $right);

  my $opensuccess = open CONFIG,"< $desconfigfile";
  if (!$opensuccess)
  {
    print STDERR "ERROR: Can't open DES config/stagefile '$desconfigfile'\n";
    exit $FAILURE;
  }

  $def = "global";
  $linenum = 0;
  while ($line = <CONFIG>)
  {
    $linenum++;
    $line =~ s/^\s+//g;
    $line =~ s/\s+$//g;
    if (($line =~ /\S/) && ($line !~ /^#/))
    {
      $line =~ s/=/ /;
#      print "line: $line\n";
      ($left, $right) = $line =~ m/^(\S+)\s*(.*)$/;

      $left = lc $left;
      switch ($left)
      {
        case /^archive$/ { saveDef(); storeVal($left,$right, "val", $NOSPACE); 
                           $def = "archive";  
                           storeVal("defname", $right, "val", $NOSPACE); }
        case /^def$/ { saveDef(); 
                       my ($dt, $dn) = ($right =~ m/^\s*(\S+)\s+(\S.*)$/); 
                       $def = $dt; 
                       storeVal("defname", $dn, "val", $NOSPACE); }
        case /^archive_name$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^archive_root$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^batch_type$/ { storeVal($left, $right, "val", $NOSPACE); }
	case /^compute_ppn$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^ccd_start$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^ccd_stop$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^dagman_max_idle$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^dagman_max_jobs$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^dagman_max_post$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^dagman_max_pre$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^dest_archive_loc$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^detector$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^email$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^enddef$/ { saveDef(); }
        case /^launch_cmd$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^launch_args$/ { storeVal($left,$right, "val", $ALLOWSPACE); }
        case /^location_name$/ { storeVal($left,$right, "val", $ALLOWSPACE); }
        case /^location_id$/ { storeVal($left,$right, "val", $ALLOWSPACE); }
        case /^grid_host$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^grid_port$/ { storeVal($left,$right, "val", $NUMERIC); }
        case /^grid_type$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^gridftp_host$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^gridftp_port$/ { storeVal($left,$right, "val", $NUMERIC); }
        case /^grid_resource$/ { storeVal($left,$right, "val", $ALLOWSPACE); }
        case /^login_host$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^login_gsissh_port$/ { storeVal($left,$right, "val", $NUMERIC); }
        case /^max_mem$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^max_retries$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^num_jobs$/ { storeVal($left,$right, "val", $NUMERIC); }
        case /^output_base$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^nite$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^num_procs$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^phot_flag$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^pipeline_root$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^platform$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^project$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^queue$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^run_archive_loc$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^run_site$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^run_software_loc$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^runid$/ { storeVal($left, $right, "val", $NOSPACE); }
#        case /^site$/ { saveDef(); storeVal($left,$right, "val", $NOSPACE); 
#                        $def = "site";
#                        storeVal("defname",$right, "val", $NOSPACE); }
        case /^site_name$/ { storeVal($left,$right,"val",$NOSPACE); }
        case /^src_archive_loc$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^software_name$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^stage$/ { saveDef(); $def = "stage"; 
                         storeVal("defname",$right, "val", $NOSPACE); 
                         addStageOrder($right); } 
        case /^stage_args$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^stage_cmd$/ { storeVal($left,$right, "val", $NOSPACE); }
        case /^stage_list$/ { storeVal($left, $right, "val", $ALLOWSPACE); }
        case /^use_db_config$/ { 

           # Create a new DES database connection object:
           my $desdb = new DES::desDBI;
#           my ($d, @rows, $rowref, $col);

           foreach my $d (@{$self->{"deforder"}})
           {
              if (($d ne "global") && ($d ne "stage"))
              {
                 print "Grabbing $d config info. from database...\n";
                 my @rows;
                 my $pkey = $desdb->getDBTableInfo($d,\@rows);
                 foreach my $rowref (@rows)
                 {
                    $def = $d;  
                    storeVal("defname", $$rowref{"$pkey"}, "val", $NOSPACE);
                    foreach my $col (keys %$rowref)
                    {
                       my $val = $$rowref{"$col"};
                       $col = lc $col;
                       if($val) {
                 #        print "col = $col, value = ", $val, "\n";
                       
                 #        if ( ($col eq "site_name") || 
                 #             ($col eq "location_name") || 
                 #             ($col eq "location_id") || 
                 #             ($col eq "archive_root") || 
                 #             ($col eq "software_location_name") || 
                 #             ($col eq "pipeline_root") 
                 #           )
                 #        {
                            # Needed/defined values
                            storeVal($col, $val, "val", $ALLOWSPACE);
                 #        }
                 #        else
                 #        {
                 #           # User variables
                 #           storeVal($col, $val, "var", $ALLOWSPACE);
                 #        }
                       }
                       else {
                 #        print "col = $col, value = NULL\n";
                       }
                    }
                    saveDef();
                 }
              }
           }
        }
        case /^wall_hrs$/ { storeVal($left, $right, "val", $NOSPACE); }

        ### user-defined variables
        case /^var$/ { my ($key, $val) = ($right =~ m/^\s*(\S+)\s+(\S.*)$/); 
                       storeVal($key, $val, "var", $NOSPACE); }

        ### internal values
        case /^allcondorlogs$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^daglog$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^debuglog$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^internaldir$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^outputdir$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^submitdir$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^uniqname$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^usercfgdir$/ { storeVal($left, $right, "val", $NOSPACE); }
        case /^verbose$/ { storeVal($left, $right, "val", $NOSPACE); }
        else {
           if ($checkkeys) {
              print $errfh "ERROR: Unrecognized line in config '$desconfigfile'\n";
              print $errfh "Line number $linenum:  '$line'\n";
              exit $FAILURE;
           }
           else { 
              storeVal($left, $right, "val", $ALLOWSPACE); 
           }
        }
      }
    }
  }
  
  saveDef();

  close CONFIG;
}

sub getValueReq
{
   my $self = shift;
   my $key = shift;
   my $defnames = shift; # hash ref to names for defs to use (deftype=>name)
   my $val = "";

   $val = $self->getValue($key, $defnames);
   if ($val !~ /\S/)
   {
     print STDERR "ERROR: Missing required value: '$key'\n";
     exit $FAILURE;
   }
   
   return $val;
} 

sub getValue
{
   my $self = shift;
   my $key = shift;
   my $defnames = shift;  # hash ref to names for defs to use (deftype=>name)
   my $val = "";
   my ($d, $defname, $ref);

   $key = lc $key;

   my $found = 0;
   if (defined($defnames))
   {
      foreach $d (@{$self->{"deforder"}})
      {
#print "d = $d\n";
         if (defined($defnames->{"$d"}))
         {
            $defname = $defnames->{"$d"};
            if (defined($self->{"defs"}{"$d"}{$defname}))
            {
               $ref = \%{$self->{"defs"}{"$d"}{$defname}};
               if (defined($ref->{"$key"}))
               {
                  $found = 1;
                  $val = $ref->{"$key"};
                  last;
               }
            }
            if (!$found && (defined($self->{"uservars"}{"$d"}{$defname})))
            {
               $ref = \%{$self->{"uservars"}{"$d"}{$defname}};
               if (defined($ref->{"$key"}))
               {
                  $found = 1;
                  $val = $ref->{"$key"};
                  last;
               }
            }
         }
      }
   }

   # check global values if haven't already found it
   if (!$found)
   {
#print "getValue: checking global values for $key\n";
      $ref = \%{$self->{"defs"}{"global"}};
      if (defined($ref->{"$key"}))
      {
         $found = 1;
         $val = $ref->{"$key"};
      }
      else
      {
         $ref = \%{$self->{"uservars"}{"global"}};
         if (defined($ref->{"$key"}))
         {
            $found = 1;
            $val = $ref->{"$key"};
         }
      }
   }

   # do variable substitution for ${*}
   while ($val =~ /\${/)
   {
#print "before val = '$val'\n";
   
     if ($val =~ /\${([^}]+)}/)
     {
        my $match = $1;
        print "matched '$match'\n";
        if ($match eq $key)
        {
           print STDERR "ERROR: recursive value definition for '$key'\n";
           print STDERR "ABORTING\n";
           exit $FAILURE;
        }
        my $matchval = $self->getValue($match, $defnames);
        if ($matchval !~ /\S/)
        {
           print STDERR "ERROR: value for '$match' doesn't exist\n";
           print STDERR "ABORTING\n";
           exit $FAILURE;
        }
        $val =~ s/\${$match}/$matchval/g;
#print "After val = '$val'\n";
     }
   }

   return $val;
} 

sub resetStageOrder
{
   my $self = shift;

   $self->{"values"}->{"__numstage"} = 0;
   for (my $i = 0; $i <= scalar(@{$self->{"stageorder"}}); $i++)
   {
      pop(@{$self->{"stageorder"}});
   }
   undef($self->{"stageorder"});
}



sub getStageList
{
  my $self = shift;
  my $arrayref = shift;
 
  for (my $i = 0; $i < scalar(@{$self->{"stageorder"}}); $i++)
  {
     push (@$arrayref, $self->{"stageorder"}[$i]);
  }
}

1;
