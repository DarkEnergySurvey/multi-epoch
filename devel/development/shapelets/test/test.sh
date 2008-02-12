#Run this from the /test directory so the (relative) paths are right

#maindir=~/Archive/red
#pointid=10--44-i-2
#processdate=20071017
#obsdate=20071006
#chip=47

#dir=${maindir}/DES${processdate}_des${obsdate}_${chip}/data/des${obsdate}/i/decam--${pointid}
#echo $dir

#imfile=${dir}/decam--${pointid}_${chip}_im.fits
#catfile=${dir}/decam--${pointid}_${chip}_scamp.fits

imfile=decam--10--44-i-2_47_im.fits
catfile=decam--10--44-i-2_47_scamp.fits

fconfig=../config/FindStars.conf
mconfig=../config/MeasureStars.conf

execdir=../../bin

# make sure to use old python
/usr/bin/python $execdir/measure_stars.py \
    -f $fconfig \
    -m $mconfig \
    $imfile $catfile
