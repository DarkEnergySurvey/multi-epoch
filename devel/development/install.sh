#!/bin/bash

DIR=`pwd`
#######################################################################
# cfitsio
#
#
#######################################################################
cd cfitsio
./configure CC=icc CFLAGS="-O2 --static -no-prec-div" --prefix=`pwd`/../
make
make install
cd $DIR

#######################################################################
# Numerical Recipies
#######################################################################
cd NRecipes/2ndEd_c-kr
make libs
make install
make libs_double
make install_double

cd $DIR

cd ImageProc
make
make install
