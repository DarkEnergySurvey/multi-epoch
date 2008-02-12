#!/bin/bash

SYS='HG'
CC=icc
CXX=icpc
DIR=`pwd`
LOG=${DIR}/support_codes_build.log

# Use this ~/.soft file and run resoft:
#  @teragrid-basic
#  @teragrid-dev
#  @globus-4.0
#  +atlas
#  +fftw-3-intel90
#  +intel-c-9.1.046-f-9.1.041-r1

export PATH=${DIR}/bin:${PATH}
export LD_LIBRARY_PATH=${DIR}/lib:${LD_LIBRARY_PATH}

if [ -f $LOG ]; then
  rm $LOG
fi 


##############################################################################
##############################################################################


echo '#######################################################################' | tee $LOG
echo "# Build of cfitsio on $SYS: STARTING..." | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
cd cfitsio
./configure CC=icc CFLAGS="-O2 --static" --prefix=${DIR} >> $LOG
make >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
  echo '#######################################################################' | tee -a $LOG
  echo "# Build of cfitsio on $SYS: FAILED!" | tee -a $LOG
  echo '#######################################################################' | tee -a $LOG
  exit 99
fi
make install >> $LOG
cd $DIR
echo '#######################################################################' | tee -a $LOG
echo "# Build of cfitsio on $SYS: COMPLETED!"  | tee -a $LOG
echo '#######################################################################' | tee -a $LOG



##############################################################################
##############################################################################

echo '#######################################################################' | tee -a $LOG
echo "# Build of Numerical Recipies on $SYS: STARTING..." | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
cd NRecipies/2ndEd_c-kr
make libs >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
  echo '#######################################################################' | tee -a $LOG
  echo "# Build of Numerical Recipies on $SYS: FAILED" | tee -a $LOG
  echo '#######################################################################' | tee -a $LOG
fi
make libs_double >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
echo '#######################################################################' | tee -a $LOG
echo "# Build of Numerical Recipies on $SYS: FAILED" | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
fi
make install >> $LOG
make install_double >> $LOG
cd $DIR


##############################################################################
##############################################################################


echo '#######################################################################' | tee -a $LOG
echo "# Build of autoconf-2.58 on $SYS: STARTING..." | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
cd autoconf-2.58
./configure --prefix=${DIR} >> $LOG
make >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
  echo '#######################################################################' | tee -a $LOG
  echo "# Build of autoconf-2.58 on $SYS: FAILED!" | tee -a $LOG
  echo '#######################################################################' | tee -a $LOG
  exit 99
fi
make install >> $LOG
cd $DIR
echo '#######################################################################' | tee -a $LOG
echo "# Build of autoconf-2.58 on $SYS: COMPLETED!" | tee -a $LOG
echo '#######################################################################' | tee -a $LOG


##############################################################################
##############################################################################


echo '#######################################################################' | tee -a $LOG
echo "# Build of PLPlot on $SYS: STARTING..." | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
cd plplot-5.8.0-RC1
./configure --prefix=${DIR} >> $LOG
gmake >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
  echo '#######################################################################' | tee -a $LOG
  echo "# Build of PLPlot on $SYS: FAILED!" | tee -a $LOG
  echo '#######################################################################' | tee -a $LOG
  exit 99
fi
gmake install >> $LOG
cd $DIR
echo '#######################################################################' | tee -a $LOG
echo "# Build of PLPlot on $SYS: COMPLETED!" | tee -a $LOG
echo '#######################################################################' | tee -a $LOG


