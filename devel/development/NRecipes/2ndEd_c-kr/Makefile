# Master makefile for Numerical Recipes

#CCKR = cc
#CCANSI = gcc

# Intel Compilers
CCKR = icc -O3 -static -no-prec-div
CCANSI = icc -O3 -ansi -static -no-prec-div

#gcc has been found to be the best bet

# Important system parameters
#FFLAGS =
#CFLAGSANSI = -O -I../include
#CFLAGSKR = -O -I../include

# Intel change.
CFLAGSANSI = -I../include
CFLAGSKR = -I../include


# Use -O if you trust your optimizer, and
# -g to make a debugging version of this library
# See the document COMPILING and your system documentation for information
# on values to use here.

# Library names . . .
CKRLIB = libpress2ndkr.a
CKRLIB_DOUBLE = libpress2ndkrD.a
CANSILIB = librecipes_c.a
FLIB = librecipes_f.a
# and destination.
LIBDIR = ../../../lib

TEST = /usr/bin/test

AR = /usr/bin/ar
ARFLAGS = crv

MAKE = /usr/bin/make

CBIN = ../bin
FBIN = ../bin
# This is internal

SHELL = /bin/sh
# Just to make sure

COMMON_ENV = "LIBDIR = $(LIBDIR)" \
	"FC = $(FC)" "CCKR = $(CCKR)" "CCANSI = $(CCANSI)" \
	"FFLAGS = $(FFLAGS)" \
	"CFLAGSANSI = $(CFLAGSANSI)" "CFLAGSKR = $(CFLAGSKR)" \
	"FBIN = $(FBIN)" "CBIN = $(CBIN)" "FLIB = $(FLIB)" \
	"CANSILIB = $(CANSILIB)" "CKRLIB = $(CKRLIB)" \
	"AR = $(AR)" "ARFLAGS = $(ARFLAGS)" "SHELL = $(SHELL)"

COMMON_ENV_DOUBLE = "LIBDIR = $(LIBDIR)" \
	"FC = $(FC)" "CCKR = $(CCKR)" "CCANSI = $(CCANSI)" \
	"FFLAGS = $(FFLAGS)" \
	"CFLAGSANSI = $(CFLAGSANSI)" "CFLAGSKR = $(CFLAGSKR)" \
	"FBIN = $(FBIN)" "CBIN = $(CBIN)" "FLIB = $(FLIB)" \
	"CANSILIB = $(CANSILIB)" "CKRLIB = $(CKRLIB_DOUBLE)" \
	"AR = $(AR)" "ARFLAGS = $(ARFLAGS)" "SHELL = $(SHELL)"

MAKE_LIST = make_list

all: distributed libs install libs_double install_double

clear_dist:
	/bin/rm -f $(MAKE_LIST)
	cp /dev/null $(MAKE_LIST)

fortran_if_have:
	-if $(TEST) -d ../2ndEd_f; then \
	echo '../2ndEd_f' >> $(MAKE_LIST); fi

c_ansi_if_have:
	-if $(TEST) -d ../2ndEd_c-ansi; then \
	echo '../2ndEd_c-ansi' >> $(MAKE_LIST); fi

c_kr_if_have:
	-if $(TEST) -d ../2ndEd_c-kr; then \
	echo '../2ndEd_c-kr' >> $(MAKE_LIST); fi

distributed: clear_dist fortran_if_have c_ansi_if_have c_kr_if_have

libs:
	echo Making Numerical Recipes
	for distrib in `cat $(MAKE_LIST)`; do \
	for library in $$distrib/recipes; do \
	(cd $$library && $(MAKE) all $(COMMON_ENV)); done; done

libs_double:
	echo Making Double Precision Numerical Recipes
	for distrib in `cat $(MAKE_LIST)`; do \
	for library in $$distrib/recipes_double; do \
	(cd $$library && $(MAKE) all $(COMMON_ENV_DOUBLE)); done; done

install: 
	echo Making Install
	for distrib in `cat $(MAKE_LIST)`; do \
	for library in $$distrib/recipes; do \
	(cd $$library && $(MAKE) install $(COMMON_ENV)); done; done

install_double: 
	echo Making Double Precision Install
	for distrib in `cat $(MAKE_LIST)`; do \
	for library in $$distrib/recipes_double; do \
	(cd $$library && $(MAKE) install $(COMMON_ENV_DOUBLE)); done; done

#demobins: 
#	echo Making Numerical Recipes Demonstration output
#	for distrib in `cat $(MAKE_LIST)`; do \
#	for bindir in $$distrib/demo/bin; do \
#	(cd $$bindir && $(MAKE) all $(COMMON_ENV)); done; done

#results:
#	echo Making Numerical Recipes Demonstration output
#	for distrib in `cat $(MAKE_LIST)`; do \
#	for resdir in $$distrib/demo/results; do \
#	(cd $$resdir && $(MAKE) all $(COMMON_ENV)); done; done

clean: 
	echo Making Clean
	for distrib in `cat $(MAKE_LIST)`; do \
	for library in $$distrib/recipes; do \
	(cd $$library && $(MAKE) clean $(COMMON_ENV)); done; done
	echo Making Clean Double Precision
	for distrib in `cat $(MAKE_LIST)`; do \
	for library in $$distrib/recipes_double; do \
	(cd $$library && $(MAKE) clean $(COMMON_ENV_DOUBLE)); done; done
