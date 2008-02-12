################################################################################
# Makefile fo DES ImagProc codes.
#
# Modified 10/2007 by Darren Adams.
#
#  The implit rule is for every *.o object file in the OBJ list, a cooresponding
#  *.c or *.cc file exists.  Executable targets are stil explixitly defined and
#  a complete list of working targets should be maintained in the EXE variable.
#
#  A future simplification would be to group list of main targes with the same
#  command and the same dependencies--reducing the number of explicit target definitions.
#
################################################################################


################################################################################
# Install and include locations.
################################################################################
FITSIO :=       ../lib/libcfitsio.a
NR2 :=          ../lib/libpress2ndkr.a
NR2D :=          ../lib/libpress2ndkrD.a
BINDIR := 	../bin
INCLUDE :=       ../include
vpath %.h $(INCLUDE)
IFLAGS := -I. -I$(INCLUDE)
################################################################################
# gcc
################################################################################
#CC :=		gcc
#CPPC := 		g++ -g3
#CPPCOPTS :=	-c -Wall -Wno-deprecated -funroll-loops -O3
#CPPLINK :=	g++ -g3
#FC :=		f77
#CFLAGS := 	-O $(IFLAGS)

################################################################################
# Intel
################################################################################
CC :=		icc
CPPC := 		icpc
CPPCOPTS :=	-c -Wall -Wno-deprecated -funroll-loops -O3
CPPLINK :=	-static
FC :=		ifort
#CFLAGS := 	-g -w -no-prec-div -funroll-loops -Wall -Wno-deprecated $(IFLAGS)
#CFLAGS := 	-O2 -g -w -no-prec-div -funroll-loops -Wall -Wno-deprecated $(IFLAGS)
CFLAGS := 	-O2 -g -w -Wall -Wno-deprecated $(IFLAGS)
CFLAGS := 	-O2 -w -Wall -Wno-deprecated $(IFLAGS)





################################################################################
# Object Files and executable targets:
################################################################################
# Broken objects:
#   sexcombine.o correcctRADEC.o DC0truth_convert.o 
# Dead targets:
#   TNXtoTANPV correctRADEC overscan sexcombine coadd_zp binary2ascii (dependancy problems)
#   DC0truth_convert USNOB_convert (c errors)
################################################################################

################################################################################
# Convert_ingest codes
################################################################################
# Objects:
OBJ1 = Mosaic2_convert.o DECam_convert.o makeingestlist.o convert_ingest.o src_ingest.o
# Targets:
EXE1 = Mosaic2_convert DECam_convert makeingestlist convert_ingest src_ingest

################################################################################
# Rest.
################################################################################
# Objects:
OBJ2 = archivesubsnew.o archivesubs.o imarithsubs.o imreadsubs.o  \
      imcorrect.o fitsheaderfix.o zerocombine.o editbpm.o mkbpm.o splitimage.o imreadRADEC.o \
      sciencecombine.o correctRADEC.o mkillumcor.o SExcat_convert.o catsubs.o update_zeropoint.o \
      mosaictile.o check_hdu.o check_keyword.o checklist.o getfitsfromlist.o makedirs.o makelist.o \
      runSWarp_remap.o runSExtractor.o runFitscombine.o runCatalog_ingest.o runSCAMP.o listconvert.o \
      coadd_catalogingest.o image_ingest.o keyword_ingest.o usno_input.o catalog_ingest.o fitscombine.o \
      db_utils.o coadd_grabmatches.o coadd_fluxscale.o coadd_fmatch.o skycoverage.o check_scamp.o \
      coadd_select.o SPT_ingest.o copy_objects.o htm_aux.o get_FWHM.o get_ellipticity.o get_skybrite.o \
      check_PSM.o ar_filterduplicates.o grabmatches_test.o coadd_grabmatches_test check_remap.o

# Targets:
EXE2 =   sciencecombine zerocombine flatcombine mkbpm editbpm fitscombine mkillumcor imsim1_convert imreadRADEC \
        splitimage fitsheaderfix SPT_ingest reset_zeropoint \
        SExcat_convert update_zeropoint select_remaps grabmatches update_zeropoint_fl findtile findtile_allccd \
        ascii2ldac catalog_ingest fwhm fixradec test_filename_resolve test_filename_construct \
        mosaictile ar_find ar_filterduplicates ar_remove ar_removefile check_hdu check_keyword checklist \
        getfitsfromlist makedirs makelist runSWarp_remap runSExtractor runFitscombine runCatalog_ingest runSCAMP \
        image_ingest keyword_ingest usno_input quick_coadd astrometry insert_head QA_astrometry image_chi2 \
        scampmkcat select_astrometric_standards listconvert coadd_fluxscale coadd_chi2detection coadd_catalogingest \
	coadd_grabmatches coadd_fmatch coadd_select copy_objects get_FWHM get_ellipticity get_skybrite skycoverage \
	check_scamp check_PSM grabmatches_test coadd_grabmatches_test check_remap


OBJ = ${OBJ1} ${OBJ2}
EXE = ${EXE1} ${EXE2}

################################################################################
# Main targets
################################################################################
all: $(EXE)
convert_ingest: $(EXE1)

################################################################################
# Header file dependancies.
################################################################################
HEADER = imageproc.h fitsio2.h fitsio.h
${OBJ}: ${HEADER}


################################################################################
# Target build Rules:
################################################################################

##########################################################
# Convert_ingest rules:
##########################################################
Mosaic2_convert: Mosaic2_convert.o imreadsubs.o archivesubsnew.o
	$(CC) $(CFLAGS) -o Mosaic2_convert Mosaic2_convert.o \
	archivesubsnew.o imreadsubs.o $(FITSIO) -lm
DECam_convert: DECam_convert.o imreadsubs.o archivesubsnew.o
	$(CC) $(CFLAGS) -o DECam_convert DECam_convert.o \
	archivesubsnew.o imreadsubs.o $(FITSIO) -lm
makeingestlist: makeingestlist.o db_utils.o
	$(CC) $(CFLAGS) -o makeingestlist makeingestlist.o \
	db_utils.o -lm
convert_ingest:  convert_ingest.o db_utils.o
	$(CC) $(CFLAGS) -o convert_ingest convert_ingest.o db_utils.o -lm
src_ingest:  src_ingest.o db_utils.o
	$(CC) $(CFLAGS) -o src_ingest src_ingest.o db_utils.o -lm
##########################################################
# Rest.
##########################################################
imcorrect: imcorrect.o imreadsubs.o imarithsubs.o
	$(CC) $(CFLAGS) -o imcorrect imcorrect.o \
	imreadsubs.o imarithsubs.o $(NR2) $(FITSIO) -lm
skycombine: skycombine.o imreadsubs.o imarithsubs.o
	$(CC) $(CFLAGS) -o skycombine skycombine.o \
	imreadsubs.o imarithsubs.o $(NR2) $(FITSIO) -lm
sciencecombine: sciencecombine.o imreadsubs.o imarithsubs.o
	$(CC) $(CFLAGS) -o sciencecombine sciencecombine.o \
	imreadsubs.o imarithsubs.o $(NR2) $(FITSIO) -lm
zerocombine: zerocombine.o imreadsubs.o imarithsubs.o
	$(CC) $(CFLAGS) -o zerocombine zerocombine.o \
	imreadsubs.o imarithsubs.o $(NR2) $(FITSIO) -lm
flatcombine: flatcombine.o imreadsubs.o imarithsubs.o
	$(CC) $(CFLAGS) -o flatcombine flatcombine.o \
	imreadsubs.o imarithsubs.o $(NR2) $(FITSIO) -lm
mkbpm: mkbpm.o imreadsubs.o 
	$(CC) $(CFLAGS) -o mkbpm mkbpm.o \
	imreadsubs.o $(NR2) $(FITSIO) -lm
editbpm: editbpm.o imreadsubs.o 
	$(CC) $(CFLAGS) -o editbpm editbpm.o \
	imreadsubs.o $(NR2) $(FITSIO) -lm
fitscombine: fitscombine.o imreadsubs.o
	$(CC) $(CFLAGS) -o fitscombine fitscombine.o \
	imreadsubs.o $(FITSIO) -lm
mkillumcor: mkillumcor.o imreadsubs.o
	$(CC) $(CFLAGS) -o mkillumcor mkillumcor.o \
	imreadsubs.o $(FITSIO) $(NR2) -lm
imsim1_convert: imsim1_convert.o imreadsubs.o
	$(CC) $(CFLAGS) -o imsim1_convert imsim1_convert.o \
	imreadsubs.o $(NR2) $(FITSIO) -lm
imreadRADEC:  imreadRADEC.o imreadsubs.o
	$(CC) $(CFLAGS) -o imreadRADEC imreadRADEC.o \
	imreadsubs.o $(NR1) $(FITSIO) -lm
TNXtoTANPV: TNXtoTANPV.o imreadsubs.o
	$(CC) $(CFLAGS) -o TNXtoTANPV TNXtoTANPV.o imreadsubs.o \
	$(FITSIO) -lm
splitimage: splitimage.o imreadsubs.o 
	$(CC) $(CFLAGS) -o splitimage splitimage.o \
	imreadsubs.o $(NR2) $(FITSIO) -lm
correctRADEC: correctRADEC.o imsubs.o
	$(CC) $(CFLAGS) -o correctRADEC correctRADEC.o imsubs.o \
	$(NR2) $(FITSIO) -lm
fitsheaderfix: fitsheaderfix.o imreadsubs.o
	$(CC) $(CFLAGS) -o fitsheaderfix fitsheaderfix.o \
	imreadsubs.o $(NR2) $(FITSIO) -lm
overscan:  overscan.o
	$(CC) $(CFLAGS) -o overscan overscan.o $(NR2) $(FITSIO) -lm
SPT_ingest:  SPT_ingest.o db_utils.o
	$(CC) $(CFLAGS) -o SPT_ingest SPT_ingest.o db_utils.o -lm
sexcombine: sexcombine.o 
	$(CC) $(CFLAGS) -o sexcombine sexcombine.o \
	$(NR2) $(FITSIO) -lm
correcctRADEC: correcctRADEC.o 
	$(CC) $(CFLAGS) -o correcctRADEC correcctRADEC.o \
	$(NR2) $(FITSIO) -lm
DC0truth_convert: DC0truth_convert.o catsubs.o
	$(CC) $(CFLAGS) -o DC0truth_convert DC0truth_convert.o \
	catsubs.o \
	$(NR2) $(FITSIO) -lm
USNOB_convert: USNOB_convert.o catsubs.o
	$(CC) $(CFLAGS) -o USNOB_convert USNOB_convert.o \
	catsubs.o $(NR2) $(FITSIO) -lm
SExcat_convert: SExcat_convert.o catsubs.o
	$(CC) $(CFLAGS) -o SExcat_convert SExcat_convert.o \
	catsubs.o \
	$(NR2) $(FITSIO) -lm
update_zeropoint: update_zeropoint.o
	$(CC) $(CFLAGS) -o update_zeropoint update_zeropoint.o \
	$(NR2) $(FITSIO) -lm
select_remaps: select_remaps.o db_utils.o
	$(CC) $(CFLAGS) -o select_remaps select_remaps.o db_utils.o \
	$(NR2) $(FITSIO) -lm
grabmatches: grabmatches.o db_utils.o
	$(CC) $(CFLAGS) -o grabmatches grabmatches.o db_utils.o \
	$(NR2) $(FITSIO) -lm
reset_zeropoint: reset_zeropoint.o db_utils.o
	$(CC) $(CFLAGS) -o reset_zeropoint reset_zeropoint.o db_utils.o \
	$(NR2) $(FITSIO) -lm
update_zeropoint_fl: update_zeropoint_fl.o db_utils.o
	$(CC) $(CFLAGS) -o update_zeropoint_fl update_zeropoint_fl.o db_utils.o \
	$(NR2) $(FITSIO) -lm
findtile: findtile.o db_utils.o
	$(CC) $(CFLAGS) -o findtile findtile.o db_utils.o \
	$(NR2) $(FITSIO) -lm
findtile_allccd: findtile_allccd.o db_utils.o
	$(CC) $(CFLAGS) -o findtile_allccd findtile_allccd.o db_utils.o \
	$(NR2) $(FITSIO) -lm
ascii2ldac: ascii2ldac.o
	$(CC) $(CFLAGS) -o ascii2ldac ascii2ldac.o \
	$(NR2) $(FITSIO) -lm
catalog_ingest: catalog_ingest.c catalog_ingest.o catsubs.o htm_aux.o imarithsubs.o
	$(CPPC) -o catalog_ingest catalog_ingest.o catsubs.o htm_aux.o \
	imarithsubs.o $(NR2) $(FITSIO) -lm
coadd_zp:  coadd_zp.o
	$(CC) $(CFLAGS) -o coadd_zp coadd_zp.o \
	$(NR2) $(FITSIO) -lm
fwhm:  fwhm.o imreadsubs.o
	$(CC) $(CFLAGS) -o fwhm fwhm.o imreadsubs.o \
	$(NR2) $(FITSIO) -lm
fixradec:  fixradec.o
	$(CC) $(CFLAGS) -o fixradec fixradec.o -lm
test_filename_resolve:  test_filename_resolve.o archivesubsnew.o
	$(CC) $(CFLAGS) -o test_filename_resolve test_filename_resolve.o \
	archivesubsnew.o -lm
test_filename_construct:  test_filename_construct.o archivesubsnew.o
	$(CC) $(CFLAGS) -o test_filename_construct test_filename_construct.o \
	archivesubsnew.o -lm
mosaictile: mosaictile.o imreadsubs.o
	$(CC) $(CFLAGS) -o mosaictile mosaictile.o imreadsubs.o \
	$(FITSIO) -lm
ar_find:  ar_find.o db_utils.o
	$(CC) $(CFLAGS) -o ar_find ar_find.o db_utils.o -lm
ar_filterduplicates:  ar_filterduplicates.o db_utils.o
	$(CC) $(CFLAGS) -o ar_filterduplicates ar_filterduplicates.o db_utils.o -lm
ar_remove:  ar_remove.o
	$(CC) $(CFLAGS) -o ar_remove ar_remove.o -lm
ar_removefile:  ar_removefile.o archivesubs.o
	$(CC) $(CFLAGS) -o ar_removefile ar_removefile.o \
	archivesubs.o -lm
check_hdu: check_hdu.o imageproc.h
	$(CC) $(CFLAGS) -o check_hdu check_hdu.o \
	$(FITSIO) -lm
check_keyword: check_keyword.o imageproc.h
	$(CC) $(CFLAGS) -o check_keyword check_keyword.o \
	$(FITSIO) -lm
checklist: checklist.o 
	$(CC) $(CFLAGS) -o checklist checklist.o \
	$(FITSIO) -lm
getfitsfromlist: getfitsfromlist.o
	$(CC) $(CFLAGS) -o getfitsfromlist getfitsfromlist.o
makedirs: makedirs.o
	$(CC) $(CFLAGS) -o makedirs makedirs.o
makelist: makelist.o
	$(CC) $(CFLAGS) -o makelist makelist.o
runSWarp_remap: runSWarp_remap.o imreadsubs.o archivesubs.o
	$(CC) $(CFLAGS) -o runSWarp_remap runSWarp_remap.o \
	archivesubs.o imreadsubs.o $(NR2) $(FITSIO) -lm
runSExtractor: runSExtractor.o imreadsubs.o archivesubs.o
	$(CC) $(CFLAGS) -o runSExtractor runSExtractor.o \
	imreadsubs.o archivesubs.o $(NR2) $(FITSIO) -lm
runFitscombine: runFitscombine.o 
	$(CC) $(CFLAGS) -o runFitscombine runFitscombine.o \
	$(NR2) $(FITSIO) -lm
copy_objects: copy_objects.o db_utils.o
	$(CC) $(CFLAGS) -o copy_objects copy_objects.o db_utils.o \
	$(NR2) $(FITSIO) -lm
runCatalog_ingest: runCatalog_ingest.o archivesubs.o db_utils.o
	$(CC) $(CFLAGS) -o runCatalog_ingest runCatalog_ingest.o db_utils.o \
	archivesubs.o $(NR2) $(FITSIO) -lm
runSCAMP: runSCAMP.o imreadsubs.o archivesubs.o
	$(CC) $(CFLAGS) -o runSCAMP runSCAMP.o \
	archivesubs.o imreadsubs.o $(NR2) $(FITSIO) -lm
image_ingest: image_ingest.o imreadsubs.o archivesubsnew.o db_utils.o
	$(CC) $(CFLAGS) -o image_ingest image_ingest.o db_utils.o \
	archivesubsnew.o imreadsubs.o $(FITSIO) -lm
keyword_ingest: keyword_ingest.o imreadsubs.o archivesubsnew.o 
	$(CC) $(CFLAGS) -o keyword_ingest keyword_ingest.o \
	archivesubsnew.o imreadsubs.o $(FITSIO) -lm
usno_input: usno_input.o
	$(CC) $(CFLAGS) -o usno_input usno_input.o -lm
quick_coadd: quick_coadd.o
	$(CC) $(CFLAGS) -o quick_coadd quick_coadd.o \
	$(NR2) $(FITSIO) -lm

astrometry: astrometry.o imreadsubs.o archivesubs.o
	$(CC) $(CFLAGS) -o astrometry astrometry.o \
	archivesubs.o imreadsubs.o $(NR2) $(FITSIO) -lm
insert_head: insert_head.o imreadsubs.o archivesubs.o
	$(CC) $(CFLAGS) -o insert_head insert_head.o \
	archivesubs.o imreadsubs.o $(NR2) $(FITSIO) -lm
QA_astrometry: QA_astrometry.o
	$(CC) $(CFLAGS) -o QA_astrometry QA_astrometry.o \
	$(NR2) $(FITSIO) -lm
binary2ascii: binary2ascii.o binary2ascii.o binary2ascii.o
	$(CC) $(CFLAGS) -o binary2ascii binary2ascii.o \
	archivesubs.o imreadsubs.o $(NR2) $(FITSIO) -lm
image_chi2: image_chi2.o
	$(CC) $(CFLAGS) -o image_chi2 image_chi2.o \
	$(NR2) $(FITSIO) -lm
scampmkcat: scampmkcat.o db_utils.o
	$(CC) $(CFLAGS) -o scampmkcat scampmkcat.o db_utils.o \
	$(NR2) $(FITSIO) -lm
select_astrometric_standards: select_astrometric_standards.o db_utils.o
	$(CC) $(CFLAGS) -o select_astrometric_standards \
	select_astrometric_standards.o db_utils.o $(NR2) $(FITSIO) -lm
listconvert: listconvert.o
	$(CC) $(CFLAGS) -o listconvert listconvert.o
coadd_fluxscale: coadd_fluxscale.o db_utils.o imreadsubs.o 
	$(CC) $(CFLAGS) -o coadd_fluxscale coadd_fluxscale.o \
	db_utils.o imreadsubs.o $(NR2) $(FITSIO) -lm
coadd_chi2detection: coadd_chi2detection.o db_utils.o imreadsubs.o
	$(CC) $(CFLAGS) -o coadd_chi2detection coadd_chi2detection.o \
	imreadsubs.o db_utils.o $(NR2) $(FITSIO) -lm
coadd_catalogingest: coadd_catalogingest.o db_utils.o
	$(CC) $(CFLAGS) -o coadd_catalogingest coadd_catalogingest.o db_utils.o \
	$(NR2) $(FITSIO) -lm
coadd_fmatch: coadd_fmatch.o db_utils.o 
	$(CC) $(CFLAGS) -o coadd_fmatch coadd_fmatch.o db_utils.o \
	$(NR2) $(FITSIO) -lm
photometry_mean:  photometry_mean.o 
	$(CC) $(CFLAGS) -o photometry_mean photometry_mean.o \
	$(NR2) -lm

coadd_select: coadd_select.o db_utils.o
	$(CC) $(CFLAGS) -o coadd_select coadd_select.o db_utils.o \
	$(NR2) $(FITSIO) -lm

coadd_grabmatches: coadd_grabmatches.o db_utils.o
	$(CC) $(CFLAGS) -o coadd_grabmatches coadd_grabmatches.o db_utils.o \
	$(NR2) $(FITSIO) -lm
coadd_grabmatches_catsim1: coadd_grabmatches_catsim1.o
	$(CC) $(CFLAGS) -o coadd_grabmatches_catsim1 coadd_grabmatches_catsim1.o \
	$(NR2) $(FITSIO) -lm
grabmatches_Imsim2_truth: grabmatches_Imsim2_truth.o
	$(CC) $(CFLAGS) -o grabmatches_Imsim2_truth grabmatches_Imsim2_truth.o \
	$(NR2) $(FITSIO) -lm
grabmatches_usnob: grabmatches_usnob.o
	$(CC) $(CFLAGS) -o grabmatches_usnob grabmatches_usnob.o \
	$(NR2) $(FITSIO) -lm

get_FWHM: get_FWHM.o db_utils.o
	$(CC) $(CFLAGS) -o get_FWHM get_FWHM.o db_utils.o \
	$(NR2) $(FITSIO) -lm
get_ellipticity: get_ellipticity.o db_utils.o
	$(CC) $(CFLAGS) -o get_ellipticity get_ellipticity.o db_utils.o \
	$(NR2) $(FITSIO) -lm
get_skybrite: get_skybrite.o db_utils.o
	$(CC) $(CFLAGS) -o get_skybrite get_skybrite.o db_utils.o \
	$(NR2) $(FITSIO) -lm

skycoverage: skycoverage.o db_utils.o
	$(CC) $(CFLAGS) -o skycoverage skycoverage.o db_utils.o \
	$(NR2) $(FITSIO) -lm
check_scamp: check_scamp.o db_utils.o
	$(CC) $(CFLAGS) -o check_scamp check_scamp.o db_utils.o \
	$(NR2) $(FITSIO) -lm
check_PSM: check_PSM.o db_utils.o
	$(CC) $(CFLAGS) -o check_PSM check_PSM.o db_utils.o \
	$(NR2) $(FITSIO) -lm

grabmatches_test: grabmatches_test.o db_utils.o
	$(CC) $(CFLAGS) -o grabmatches_test grabmatches_test.o db_utils.o \
	$(NR2) $(FITSIO) -lm

coadd_grabmatches_test: coadd_grabmatches_test.o db_utils.o
	$(CC) $(CFLAGS) -o coadd_grabmatches_test coadd_grabmatches_test.o db_utils.o \
	$(NR2) $(FITSIO) -lm

check_remap: check_remap.o imreadsubs.o imageproc.h
	$(CC) $(CFLAGS) -o check_remap check_remap.o imreadsubs.o \
	$(FITSIO) -lm

########################################################################################
# Implict rules for building objects:     
########################################################################################
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
.cc.o:
	$(CPPC) $(CPPCOPTS) -c $< -o $@


########################################################################################
# Install
########################################################################################
install:
	@echo "Installing to ${BINDIR}"; \
	for i in ${EXE}; do \
	  if [ -f $$i ]; then \
	    cp $$i ${BINDIR}/$$i; \
	  fi; \
	done;

########################################################################################
# Clean-up rules.
########################################################################################
clean:
	@echo "Removing object files..."; \
	rm -f *.o; \
        echo "Removing executables..."; \
	for i in ${EXE}; do \
	  if [ -f $$i ]; then \
	    rm -f $$i; \
	  fi; \
	done;

clean_install:
	@echo "Removing installed executables..."; \
	for i in ${EXE}; do \
	  if [ -f ${BINDIR}/$$i ]; then \
	    rm -f ${BINDIR}/$$i; \
	  fi; \
	done;
