# working directories
archive.dir=${arch.dir}/raw/${nite}
data.dir=${job.id}/data
nite.dir=${data.dir}/${nite}
original.data=${nite.dir}/raw

# script/bin location
bin.path=${job.id}/bin
etc.path=${job.id}/etc
script.path=${job.id}/etc
qc.path=${job.id}/qc

# directory structures
raw.dir=raw
scig.dir=g
scir.dir=r
scii.dir=i
sciz.dir=z
calib.dir=cal
log.dir=log

calib.bias=bias
calib.flatg=flat_g
calib.flatr=flat_r
calib.flati=flat_i
calib.flatz=flat_z
calib.bpm=bpm
calib.pupil=pupil

local.raw=${nite.dir}/${raw.dir}
local.calib=${nite.dir}/${calib.dir}
local.out=${nite.dir}/${log.dir}

local.workg=${nite.dir}/${scig.dir}
local.workr=${nite.dir}/${scir.dir}
local.worki=${nite.dir}/${scii.dir}
local.workz=${nite.dir}/${sciz.dir}

# extension 
extension=_${ccd.num}

# list of scinece/bias/flat fits 
scig.list=${local.out}/science_g_${ccd.num}.list
scigout.list=${local.out}/scienceout_g_${ccd.num}.list
scir.list=${local.out}/science_r_${ccd.num}.list
scirout.list=${local.out}/scienceout_r_${ccd.num}.list
scii.list=${local.out}/science_i_${ccd.num}.list
sciiout.list=${local.out}/scienceout_i_${ccd.num}.list
sciz.list=${local.out}/science_z_${ccd.num}.list
scizout.list=${local.out}/scienceout_z_${ccd.num}.list

bias.list =${local.out}/bias_${ccd.num}.list
flatg.list=${local.out}/flat_g_${ccd.num}.list 
flatr.list=${local.out}/flat_r_${ccd.num}.list 
flati.list=${local.out}/flat_i_${ccd.num}.list 
flatz.list=${local.out}/flat_z_${ccd.num}.list 

# astrometry refinement
ast.log=astrometry_${ccd.num}.log
ast.exec=${bin.path}/astrometry
ast.command_g=${scigout.list} -binpath ${bin.path} -etcpath ${etc.path} -weight_threshold 0.0 -detect_threshold 2.5 -delete_wat -saturation -crossid_radius 2.0
ast.command_r=${scirout.list} -binpath ${bin.path} -etcpath ${etc.path} -weight_threshold 0.0 -detect_threshold 2.5 -delete_wat -saturation -crossid_radius 2.0
ast.command_i=${sciiout.list} -binpath ${bin.path} -etcpath ${etc.path} -weight_threshold 0.0 -detect_threshold 2.5 -delete_wat -saturation -crossid_radius 2.0
ast.command_z=${scizout.list} -binpath ${bin.path} -etcpath ${etc.path} -weight_threshold 0.0 -detect_threshold 2.5 -delete_wat -saturation -crossid_radius 2.0

# copycal

bpm.dir=${bpmlocation}
bpm.name=bpm_${ccd.num}.fits
bpm.fits.name=${local.calib}/${calib.bpm}/bpm_${ccd.num}.fits

pupil.dir=${pupillocation}
pupil.name=pupil_${ccd.num}.fits
pupil.fits.name=${local.calib}/${calib.pupil}/pupil_${ccd.num}.fits

done.log=done_${ccd.num}.log

# create flat
flatcombine.log=flatcombine_${ccd.num}.log
flatcombine.exec=${bin.path}/flatcombine
flatcombine.args=$R{alist} $R{afile} -median -bias $R{abias} -bpm $R{abpm} -variance 
#flatcombine.args=$R{alist} $R{afile} -median -bias $R{abias} -bpm $R{abpm} -pupil $R{apupil} -variance 
flatg.fits.path=${local.calib}/${calib.flatg}
flatg.fits.name=flat_g_${ccd.num}.fits
flatr.fits.path=${local.calib}/${calib.flatr}
flatr.fits.name=flat_r_${ccd.num}.fits
flati.fits.path=${local.calib}/${calib.flati}
flati.fits.name=flat_i_${ccd.num}.fits
flatz.fits.path=${local.calib}/${calib.flatz}
flatz.fits.name=flat_z_${ccd.num}.fits

# running imcorrect
imcorr.log=imcorrect_${ccd.num}.log
imcorr.exec=${bin.path}/imcorrect
imcorr.scig=${scig.list} -output ${scigout.list} -overscan -bias ${bias.fits.path}/${bias.fits.name} -flatten ${flatg.fits.path}/${flatg.fits.name} -bpm ${bpm.fits.name} -variance -interpolate_col 2.0 -updatekeyword SKYBRITE
imcorr.scir=${scir.list} -output ${scirout.list} -overscan -bias ${bias.fits.path}/${bias.fits.name} -flatten ${flatr.fits.path}/${flatr.fits.name} -bpm ${bpm.fits.name} -variance -interpolate_col 2.0 -updatekeyword SKYBRITE
imcorr.scii=${scii.list} -output ${sciiout.list} -overscan -bias ${bias.fits.path}/${bias.fits.name} -flatten ${flati.fits.path}/${flati.fits.name} -bpm ${bpm.fits.name} -variance -interpolate_col 2.0 -updatekeyword SKYBRITE
imcorr.sciz=${sciz.list} -output ${scizout.list} -overscan -bias ${bias.fits.path}/${bias.fits.name} -flatten ${flatz.fits.path}/${flatz.fits.name} -bpm ${bpm.fits.name} -variance -interpolate_col 2.0 -updatekeyword SKYBRITE

#imcorr.scig=${scig.list} -output ${scigout.list} -overscan -bias ${bias.fits.path}/${bias.fits.name} -flatten ${flatg.fits.path}/${flatg.fits.name} -bpm ${bpm.fits.name} -pupil ${pupil.fits.name} -variance -interpolate_col 2.0 -updatekeyword SKYBRITE
#imcorr.scir=${scir.list} -output ${scirout.list} -overscan -bias ${bias.fits.path}/${bias.fits.name} -flatten ${flatr.fits.path}/${flatr.fits.name} -bpm ${bpm.fits.name} -pupil ${pupil.fits.name} -variance -interpolate_col 2.0 -updatekeyword SKYBRITE
#imcorr.scii=${scii.list} -output ${sciiout.list} -overscan -bias ${bias.fits.path}/${bias.fits.name} -flatten ${flati.fits.path}/${flati.fits.name} -bpm ${bpm.fits.name} -pupil ${pupil.fits.name} -variance -interpolate_col 2.0 -updatekeyword SKYBRITE
#imcorr.sciz=${sciz.list} -output ${scizout.list} -overscan -bias ${bias.fits.path}/${bias.fits.name} -flatten ${flatz.fits.path}/${flatz.fits.name} -bpm ${bpm.fits.name} -pupil ${pupil.fits.name} -variance -interpolate_col 2.0 -updatekeyword SKYBRITE

# ingestion part (Files Table)

input.list=${local.out}/files_${ccd.num}.list
output.dat=${local.out}/files_indat_${ccd.num}.txt

fitspattern1=*_${ccd.num}.*.fits
fitspattern0=*_${ccd.num}_cat.fits

inputscamp.list=${local.out}/files_scamp_${ccd.num}.list
outputscamp.dat=${local.out}/files_indat_scamp_${ccd.num}.txt

fitspattern_scamp=*_${ccd.num}_im.fits

files.table=Files
files.index=files

imageingest.exec=${bin.path}/image_ingest
imageingest.args=${input.list} ${output.dat} ${archive.node} -ccd ${ccd.num} 
imageingest.log=${local.out}/image_ingest_${ccd.num}.log

imageingest_scamp.args=${inputscamp.list} ${outputscamp.dat} ${archive.node} -ccd ${ccd.num} 
imageingest_scamp.log=${local.out}/image_ingest_scamp_${ccd.num}.log

param1=pipeline
param2=dc01user

# sqlplus calling
sql.exec=sqlplus 
sql.command=-S ${param1}/${param2}@des1/DES @ $R{sql.script}

# sqlldr input/output
file.control.file=${etc.path}/files.ctr
file.sqlldr.log=${nite.dir}/${log.dir}/files_sqlldr_${ccd.num}.log
file.sqlldr.bad=${nite.dir}/${log.dir}/files_sqlldr_${ccd.num}.bad


# sqlldr calling
sqlldr.exec=sqlldr
sqlldr.command=${param1}/${param2}@des1/DES readsize=20000000 bindsize=20000000 rows=10000 control=$R{control.file} data=$R{ingest.dat} log=$R{sqlldr.log} bad=$R{sqlldr.bad}

# make subdirs in raw, g, r, i and z from the list

makedirs.log=makedirs_${ccd.num}.log
makedirs.exec=${bin.path}/makedirs

makedirs.bias=${bias.list}
makedirs.flatg=${flatg.list}
makedirs.flatr=${flatr.list}
makedirs.flati=${flati.list}
makedirs.flatz=${flatz.list}
makedirs.scig=${scig.list}
makedirs.scir=${scir.list}
makedirs.scii=${scii.list}
makedirs.sciz=${sciz.list}
makedirs.outscig=${scigout.list}
makedirs.outscir=${scirout.list}
makedirs.outscii=${sciiout.list}
makedirs.outsciz=${scizout.list}


# move raw data
log.name=copycal_${ccd.num}.log

moveraw.exec=${bin.path}/getfitsfromlist
moveraw.command1=$R{listname} 
moveraw.command2=$R{listname} -num $R{i} -gz

# --- no common properties for Stage_pubentry ---

# --- no common properties for Stage_pubexit ---

# remap using swarp
remap.log=remap_${ccd.num}.log
remap.exec=${bin.path}/runSWarp_remap
remap.args=$R{input.list} -nite ${nite} -band $R{band} -project ${project} -binpath ${bin.path} -etcpath ${etc.path} -imagepath ${nite.dir}  -outlist $R{output.list} -runid ${jid}

# clean up raw
cleanup.exec=/bin/rm
cleanup.command=-r ${local.raw}

# adaptive moment
adap.log=adaptive_${ccd.num}.log
adaptive.exec=${bin.path}/runadaptive
adaptive.command_g=${scigout.list} ${bin.path}
adaptive.command_r=${scirout.list} ${bin.path}
adaptive.command_i=${sciiout.list} ${bin.path}
adaptive.command_z=${scizout.list} ${bin.path}

checkfile=*/*adaptive.fits

# cataloguer
cat.log=cataloguer_${ccd.num}.log
cat.exec=${bin.path}/runSExtractor
cat.command_g=${scigout.list} ${bin.path} ${etc.path} -nite ${nite} -band g -project ${project} -remap -no_bpm -no_psfex
cat.command_r=${scirout.list} ${bin.path} ${etc.path} -nite ${nite} -band r -project ${project} -remap -no_bpm -no_psfex
cat.command_i=${sciiout.list} ${bin.path} ${etc.path} -nite ${nite} -band i -project ${project} -remap -no_bpm -no_psfex
cat.command_z=${scizout.list} ${bin.path} ${etc.path} -nite ${nite} -band z -project ${project} -remap -no_bpm -no_psfex

# running fitscombine
fits.log=fitscombine_${ccd.num}.log
fits.exec=${bin.path}/runFitscombine
fits.command_g=${scigout.list} ${local.out}/remap_g_${ccd.num}.list -bin ${bin.path} -cleanup
fits.command_r=${scirout.list} ${local.out}/remap_r_${ccd.num}.list -bin ${bin.path} -cleanup
fits.command_i=${sciiout.list} ${local.out}/remap_i_${ccd.num}.list -bin ${bin.path} -cleanup
fits.command_z=${scizout.list} ${local.out}/remap_z_${ccd.num}.list -bin ${bin.path} -cleanup

# create bias
zerocombine.log=zerocombine_${ccd.num}.log
zerocombine.exec=${bin.path}/zerocombine
zerocombine.args=$R{alist} $R{afile} -average
bias.fits.path=${local.calib}/${calib.bias}
bias.fits.name=bias_${ccd.num}.fits

inputcat.list=${local.out}/files_cat_${ccd.num}.list

catpattern1=*_${ccd.num}.*_cat.fits
catpattern0=*_${ccd.num}_cat.fits

object.table=objects
object.index=objects

objs.control.file=${etc.path}/objects.ctr
objs.data.file=${nite.dir}/${log.dir}/objects_${ccd.num}.data

catingest.exec=${bin.path}/runCatalog_ingest
catingest.args=${inputcat.list} -bin ${bin.path} -sqlldr_ctrl ${objs.control.file} -sqlldr_data ${objs.data.file} -sqlldr_multiingest 4 -ccd ${ccd.num}
catingest.log=${local.out}/catalog_ingest_${ccd.num}.log
 
#QA_astrometry
qa_ast.exec=${bin.path}/QA_astrometry
qa_ast.command=log/${ast.log}
qa_ast.log=metadata_astrometry_${ccd.num}.properties

# catalogue for scamp 
catforscamp.log=catforscamp_${ccd.num}.log
catforscamp.exec=${bin.path}/runSExtractor 
catforscamp.command_g=${scigout.list} ${bin.path} ${etc.path} -scamp -weight_threshold 0.0 -detect_threshold 2.5 -saturation
catforscamp.command_r=${scirout.list} ${bin.path} ${etc.path} -scamp -weight_threshold 0.0 -detect_threshold 2.5 -saturation
catforscamp.command_i=${sciiout.list} ${bin.path} ${etc.path} -scamp -weight_threshold 0.0 -detect_threshold 2.5 -saturation
catforscamp.command_z=${scizout.list} ${bin.path} ${etc.path} -scamp -weight_threshold 0.0 -detect_threshold 2.5 -saturation

# Mkae Fringe and Illumination correction
mkillum.log=mkillum_${ccd.num}.log
mkillum.exe1=${bin.path}/sciencecombine
mkillum.exe2=${bin.path}/mkillumcor

mkillum.args1_g= ${scigout.list} cal/scicombine_g/scicombine_g_${ccd.num}.fits -srcgrowrad 20
mkillum.args2_g= cal/scicombine_g/scicombine_g_${ccd.num}.fits -minsize 5 -maxsize 100 -ranseed -301 -output_illum cal/illum_g/illum_g_${ccd.num}.fits -output_fringe cal/fringe_g/fringe_g_${ccd.num}.fits

mkillum.args1_r= ${scirout.list} cal/scicombine_r/scicombine_r_${ccd.num}.fits -srcgrowrad 20
mkillum.args2_r= cal/scicombine_r/scicombine_r_${ccd.num}.fits -minsize 5 -maxsize 100 -ranseed -302 -output_illum cal/illum_r/illum_r_${ccd.num}.fits -output_fringe cal/fringe_r/fringe_r_${ccd.num}.fits

mkillum.args1_i= ${sciiout.list} cal/scicombine_i/scicombine_i_${ccd.num}.fits -srcgrowrad 20
mkillum.args2_i= cal/scicombine_i/scicombine_i_${ccd.num}.fits -minsize 5 -maxsize 100 -ranseed -303 -output_illum cal/illum_i/illum_i_${ccd.num}.fits -output_fringe cal/fringe_i/fringe_i_${ccd.num}.fits

mkillum.args1_z= ${scizout.list} cal/scicombine_z/scicombine_z_${ccd.num}.fits -srcgrowrad 20
mkillum.args2_z= cal/scicombine_z/scicombine_z_${ccd.num}.fits -minsize 5 -maxsize 100 -ranseed -304 -output_illum cal/illum_z/illum_z_${ccd.num}.fits -output_fringe cal/fringe_z/fringe_z_${ccd.num}.fits


# running imcorrect_withcorr
imcorr_withcorr.log=imcorrect_withcorr_${ccd.num}.log
imcorr_withcorr.exec=${bin.path}/imcorrect

imcorr_withcorr.scig=${scigout.list} -output ${scigout.list} -illumination ${local.calib}/illum_g/illum_g_${ccd.num}.fits -nooverscan -updatekeyword SKYBRITE
imcorr_withcorr.scir=${scirout.list} -output ${scirout.list} -illumination ${local.calib}/illum_r/illum_r_${ccd.num}.fits -nooverscan -updatekeyword SKYBRITE
#imcorr_withcorr.scii=${sciiout.list} -output ${sciiout.list} -illumination ${local.calib}/illum_i/illum_i_${ccd.num}.fits -nooverscan -updatekeyword SKYBRITE
imcorr_withcorr.scii=${scizout.list} -output ${scizout.list} -illumination ${local.calib}/illum_i/illum_i_${ccd.num}.fits  -fringe ${local.calib}/fringe_i/fringe_i_${ccd.num}.fits  -nooverscan -updatekeyword SKYBRITE
imcorr_withcorr.sciz=${scizout.list} -output ${scizout.list} -illumination ${local.calib}/illum_z/illum_z_${ccd.num}.fits  -fringe ${local.calib}/fringe_z/fringe_z_${ccd.num}.fits  -nooverscan -updatekeyword SKYBRITE

# for Erin's shapelet code
impattern=$R{dirlist$I{$R{w}}}_${ccd.num}_im.fits
#scamppattern=$R{dirlist$I{$R{w}}}_${ccd.num}_scamp.fits
scamppattern=$R{dirlist$I{$R{w}}}_${ccd.num}_cat.fits

shapelet.exec=${bin.path}/measure_stars.py
shapelet.args=-f ${etc.path}/FindStars.conf -m ${etc.path}/MeasureStars.conf $R{image_in} $R{scamp_in}
shapelet.log=${local.out}/shapelet_measure_stars_${ccd.num}.log

# catalogue for reduced images 
cat4reduce.log=cat4reduce_${ccd.num}.log
cat4reduce.exec=${bin.path}/runSExtractor 
cat4reduce.command_g=${scigout.list} ${bin.path} ${etc.path} -reduce 
cat4reduce.command_r=${scirout.list} ${bin.path} ${etc.path} -reduce
cat4reduce.command_i=${sciiout.list} ${bin.path} ${etc.path} -reduce
cat4reduce.command_z=${scizout.list} ${bin.path} ${etc.path} -reduce

# quality assurance
qc.log=qc_${ccd.num}.log
qc.exec=${qc.path}/bin/qcrun
qc.args= --qchome=${qc.path} --g_list=remap_g_${ccd.num}.list --r_list=remap_r_${ccd.num}.list --i_list=remap_i_${ccd.num}.list --z_list=remap_z_${ccd.num}.list


# psfex for reduced
psfex_reduced.log=psfex_reduced_${ccd.num}.log
psfex_reduced.exec=${bin.path}/runSExtractor 
psfex_reduced.command_g=${scigout.list} ${bin.path} ${etc.path} -psfex 
psfex_reduced.command_r=${scirout.list} ${bin.path} ${etc.path} -psfex 
psfex_reduced.command_i=${sciiout.list} ${bin.path} ${etc.path} -psfex 
psfex_reduced.command_z=${scizout.list} ${bin.path} ${etc.path} -psfex 

# psfex for remap
psfex_remap.log=psfex_remap_${ccd.num}.log
psfex_remap.exec=${bin.path}/runSExtractor 
psfex_remap.command_g=${local.out}/remap_g_${ccd.num}.list ${bin.path} ${etc.path} -psfex 
psfex_remap.command_r=${local.out}/remap_r_${ccd.num}.list ${bin.path} ${etc.path} -psfex 
psfex_remap.command_i=${local.out}/remap_i_${ccd.num}.list ${bin.path} ${etc.path} -psfex 
psfex_remap.command_z=${local.out}/remap_z_${ccd.num}.list ${bin.path} ${etc.path} -psfex 
