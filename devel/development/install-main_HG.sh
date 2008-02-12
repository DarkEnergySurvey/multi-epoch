#!/bin/bash

SYS='HG'
CC=icc
CXX=icpc
DIR=`pwd`
LOG=${DIR}/main_codes_build.log

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


echo '##############################################################################' | tee -a $LOG
echo "# Build of psfex on $SYS: STARTING..." | tee -a $LOG
echo '##############################################################################' | tee -a $LOG
cd terapix/psfex
./configure --prefix=`pwd`/../../ --enable-icc --with-atlas=/usr/local/ATLAS >> $LOG
make >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
  echo '##############################################################################' | tee -a $LOG
  echo "# Build of psfex on $SYS: FAILED" | tee -a $LOG
  echo '##############################################################################' | tee -a $LOG
fi
make install >> $LOG
cd $DIR
echo '##############################################################################' | tee -a $LOG
echo "# Build of psfex on $SYS: COMPLETED!" | tee -a $LOG
echo '##############################################################################' | tee -a $LOG


##############################################################################
##############################################################################


echo '#######################################################################' | tee -a $LOG
echo "# Build of scamp on $SYS: STARTING..." | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
cd terapix/scamp
./configure --prefix=$DIR --enable-icc --with-atlas=/usr/local/ATLAS --with-plplot=$DIR/lib >> $LOG
make >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
  echo '#######################################################################' | tee -a $LOG
  echo "# Build of scamp on $SYS: FAILED!" | tee -a $LOG
  echo '#######################################################################' | tee -a $LOG
  exit 99
fi
make install >> $LOG
cd $DIR
echo '#######################################################################' | tee $LOG
echo "# Build of scamp on $SYS: COMPLETED!" | tee -a $LOG
echo '#######################################################################'


##############################################################################
##############################################################################

echo '#######################################################################' | tee -a $LOG
echo "# Build of sextractor on $SYS: STARTING..." | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
cd terapix/scamp
./configure --prefix=$DIR --enable-icc --with-atlas=/usr/local/ATLAS --with-plplot=$DIR/lib >> $LOG
make >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
  echo '#######################################################################' | tee -a $LOG
  echo "# Build of scamp on $SYS: FAILED!" | tee -a $LOG
  echo '#######################################################################' | tee -a $LOG
  exit 99
fi
make install >> $LOG
cd $DIR
echo '#######################################################################' | tee -a $LOG
echo "# Build of scamp on $SYS: COMPLETED!" | tee -a $LOG
echo '#######################################################################' | tee -a $LOG


##############################################################################
##############################################################################

echo '#######################################################################' | tee -a $LOG
echo "# Build of ImageProc codes on $SYS: STARTING..." | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
cd ImageProc
make >> $LOG
STAT=$?
if [ $STAT != 0 ]; then
  echo '#######################################################################' | tee -a $LOG
  echo "# Build of ImageProc codes on $SYS: FAILED!" | tee -a $LOG
  echo '#######################################################################' | tee -a $LOG
  exit 99
fi
make install >> $LOG
echo '#######################################################################' | tee -a $LOG
echo "# Build of ImageProc codes on $SYS: COMPLETED!" | tee -a $LOG
echo '#######################################################################' | tee -a $LOG
