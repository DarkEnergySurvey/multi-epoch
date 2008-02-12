#!/bin/bash

if [ $# -ne 4 ]; then
echo "Expecting 4 command line arguments: <runid> <nite> <ccdstart> <ccdstop>"
echo "Only received: $#"
exit;
fi

# Set command line arguments:
runid=$1
nite=$2
ccdstart=$3
ccdstop=$4

# Some stuff that you may want to change, but would make the command line too crowded:
# Archive node to use at the remote run site:
platform="TG"
site="ncsa_mercury"
archive_node="mercury"
archpathvar="tgrid_archpath_${site}"
archpathvalue='/gpfs_scratch1/bcs/Archive'
reduction_base='/home/bcs/pipeline/trunk/reduction'
orchestration_node='bcs.cosmology.uiuc.edu'

# These are pretty much static:
output="jobs/${runid}_${nite}"
properties_file="edit_"${platform}".properties"
properties_template="edit_"${platform}".properties.template"

# Stage switches:
submit_jobs=0
stage_convert=0
stage_process=0
stage_scamp=0
stage_postscamp=0

# Create properties file from template:
echo "Creating properties file with RUNID="${runid}" and NITE="${nite}.
sed -e "s,nite=,nite=${nite}," \
    -e "s,site=,site=${site}," \
    -e "s,uid_in=,uid_in=${runid}," \
    -e "s,ccd.start=,ccd.start=${ccdstart}," \
    -e "s,ccd.stop=,ccd.stop=${ccdstop}," \
    -e "s,archive.node=,archive.node=${archive_node}," \
    -e "s,tgrid_archpath_ncsa_mercury=,tgrid_archpath_ncsa_mercury=${archpathvalue}," \
    -e "s,mypath=,mypath=${reduction_base}," \
    -e "s,orchestration.node=,orchestration.node=${orchestration_node}," \
     ${properties_template} > ${properties_file}

# Submit stages:
if [ ${stage_convert} -eq 1 ]; then
  # Set output file for this stage:
  thisoutput="${output}_ingest.out"
  # Stage specific settings:
  maxwalltime=830
  maxcputime=830
  ingestion_jobs=60
  # Edit stage specific settings in properties file:
  sed -e "s,maxwalltime=.*,maxwalltime=${maxwalltime}," \
      -e "s,maxcputime=.*,maxcputime=${maxcputime}," \
      -e "s,ingestion.jobs=,ingestion.jobs=${ingestion_jobs}," \
       ${properties_file} > ${properties_file}.tmp
  mv ${properties_file}.tmp ${properties_file}
  if [ ${submit_jobs} -eq 1 ]; then
    echo "Executing Stage_Convert_Ingest and redirecting output to ${thisoutput}"
    ogrelaunch workflow_${platform}.xml Stage_Convert_Ingest > $thisoutput 2>&1
  fi
fi

if [ ${stage_process} -eq 1 ]; then
  # Set output file for this stage:
  thisoutput="${output}.out"
  # Stage specific settings:
  maxwalltime=830
  maxcputime=830
  # Edit stage specific settings in properties file:
  sed -e "s,maxwalltime=.*,maxwalltime=${maxwalltime}," \
      -e "s,maxcputime=.*,maxcputime=${maxcputime}," \
      -e "s|stage.list=|stage.list=Stage_makedirs, Stage_linkraw, Stage_copycal, Stage_zerocombine, Stage_flatcombine, Stage_imcorrect, Stage_catforscamp, Stage_fileingest_scamp, Stage_shapelet_measure_stars|" \
       ${properties_file} > ${properties_file}.tmp
  mv ${properties_file}.tmp ${properties_file}
  if [ ${submit_jobs} -eq 1 ]; then
    echo "Executing Stage_WS and redirecting output to ${thisoutput}"
    ogrelaunch workflow_${platform}.xml Stage_${platform} > $thisoutput 2>&1
  fi
fi

if [ ${stage_scamp} -eq 1 ]; then
  # Set output file for this stage:
  thisoutput="${output}_scamp.out"
  # Stage specific settings:
  maxwalltime=830
  maxcputime=830
  scamplist_number=30
  # Edit stage specific settings in properties file:
  sed -e "s,maxwalltime=.*,maxwalltime=${maxwalltime}," \
      -e "s,maxcputime=.*,maxcputime=${maxcputime}," \
      -e "s,scamplist.number=30,scamplist.number=${scamplist_number}," \
      -e "s|stage_postscamp.list=|stage_postscamp.list=Stage_remap, Stage_runcat, Stage_cat4reduce, Stage_qc, Stage_runfitscombine, Stage_fileingest, Stage_done|" \
       ${properties_file} > ${properties_file}.tmp
  mv ${properties_file}.tmp ${properties_file}
  if [ ${submit_jobs} -eq 1 ]; then
    echo "Executing Stage_scamp and redirecting output to ${thisoutput}"
    ogrelaunch workflow_${platform}.xml Stage_scamp > $thisoutput 2>&1
  fi
fi

if [ ${stage_postscamp} -eq 1 ]; then
  # Set output file for this stage:
  thisoutput="${output}_postscamp.out"
  # Stage specific settings:
  maxwalltime=830
  maxcputime=830
  # Edit stage specific settings in properties file:
  sed -e "s,maxwalltime=.*,maxwalltime=${maxwalltime}," \
      -e "s,maxcputime=.*,maxcputime=${maxcputime}," \
       ${properties_file} > ${properties_file}.tmp
  mv ${properties_file}.tmp ${properties_file}
  if [ ${submit_jobs} -eq 1 ]; then
    echo "Executing Stage_postscamp and redirecting output to ${thisoutput}"
    ogrelaunch workflow_${platform}.xml Stage_postscamp > $thisoutput 2>&1
  fi
fi

exit;
