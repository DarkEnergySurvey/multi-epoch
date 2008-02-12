
# Build directories
#BUILD_DIRS = cfitsio NRecipes/2ndEd_c-kr ImageProc

# Install directories
LIB = ./lib
BIN = ./bin
INCLUDE = ./include

VPATH=$(LIB):$(BIN):$(INCLUDE)

#.PHONY: all $(BUILD_DIRS)
#all: $(BUILD_DIRS)
all: cfitsio NRecipies ImageProc
clean: clean_cfitsio clean_NRecipies clean_ImageProc
.PHONY: all
.PHONY: clean

# Combine targets
cfitsio: libcfitsio.a fitsio.h fitsio2.h
NRecipies: libpress2ndkr.a libpress2ndkrD.a 

# Clean rules:
clean_cfitsio:
	cd cfitsio && rm -f config.log config.status
	cd cfitsio && make clean
	cd $(LIB) && rm -f libcfitsio.a
	cd $(INCLUDE) && rm -f fitsio.h fitsio2.h drvrsmem.h longnam.h
clean_NRecipies:
	cd NRecipes/2ndEd_c-kr && $(MAKE) clean
	cd $(LIB) && rm -f libpress2ndkr.a libpress2ndkrD.a
clean_ImageProc:
	cd ImageProc && $(MAKE) clean
	cd ImageProc && $(MAKE) clean_install

# Build rules
libcfitsio.a fitsio.h fitsio2.h:
	cd cfitsio && ./configure CC=icc CFLAGS="-O2 --static --prefix=`pwd`/../" --prefix=`pwd`/../
	cd cfitsio && $(MAKE)
	cd cfitsio && $(MAKE) install

libpress2ndkr.a libpress2ndkrD.a:
	cd NRecipes/2ndEd_c-kr && $(MAKE) libs
	cd NRecipes/2ndEd_c-kr && $(MAKE) install
	cd NRecipes/2ndEd_c-kr && $(MAKE) libs_double
	cd NRecipes/2ndEd_c-kr && $(MAKE) install_double
	
ImageProc: cfitsio NRecipies
	cd ImageProc && $(MAKE) all 
	cd ImageProc && $(MAKE) install
