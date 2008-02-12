CC= g++
CFLAGS= -I../errors -I../tmv0.54 -I../tmv0.54 -I../ConfigFile -I../roundstars -I../ConfigFile -I../findstars -O3 
TMV= -L../tmv0.54/gcc-noblas -L../ConfigFile -lsmalltmv -lm 
TMVLIB= ../tmv0.54/gcc-noblas/libsmalltmv.a

SFDIR=../findstars/odir
EDIR=../errors
SFLIBS=$(SFDIR)/StarFinder.o \
	   $(SFDIR)/BinomFact.o \
	   $(SFDIR)/Bounds.o \
	   $(SFDIR)/Histogram.o \
	   $(SFDIR)/Legendre2D.o \
	   $(EDIR)/Errors.o

CFLIB=../ConfigFile/ConfigFile.o

ODIR= ../roundstars/odir

FITS= -L../cfitsio -lcfitsio
FITSI= -I../cfitsio 
#FITS= -lcfitsio
#FITSI= -I/usr/local/include
BINDIR= ~mjarvis/bin

OLIST = measure_stars.o \
	MeasureStars.o \
	$(ODIR)/Image.o \
	$(ODIR)/Kernel.o \
	$(ODIR)/FullShape.o \
	$(ODIR)/Star.o \
	$(CFLIB) \
	$(SFLIBS) 

all : measure_stars 

install:
	- cp measure_stars ../../bin

measure_stars: $(OLIST) $(TMVLIB)
	$(CC) $(CFLAGS) $(OLIST) $(TMV) -lm $(FITS) -o measure_stars


clean:
	- rm *.o measure_stars
	- rm ../../bin/measure_stars


measure_stars.o: measure_stars.cpp measure_stars.hpp types.h
	$(CC) $(CFLAGS)	-c measure_stars.cpp $(FITSI) -o measure_stars.o
MeasureStars.o: MeasureStars.cpp MeasureStars.hpp types.h
	$(CC) $(CFLAGS)	-c MeasureStars.cpp $(FITSI) -o MeasureStars.o

