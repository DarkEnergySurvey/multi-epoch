#CFLAGS= -O3 -I../errors -I../tmv0.54 -I../ConfigFile -Wall -Werror
CFLAGS= -O3 -I../errors -I../tmv0.54 -I../ConfigFile -Wall
CC= g++
TMV= -L../tmv0.54/gcc-noblas -L../ConfigFile -lsmalltmv -lm
TMVLIB= ../tmv0.54/gcc-noblas/libsmalltmv.a
CFLIB=../ConfigFile/ConfigFile.o
ODIR= odir

#FITS= -lcfitsio
#FITSI= -I/usr/local/include
FITS= -L../cfitsio -lcfitsio
FITSI= -I../cfitsio
BINDIR= .

default: all
#all : $(BINDIR)/findstars

all: $(ODIR)/StarFinder.o  $(ODIR)/BinomFact.o $(ODIR)/Bounds.o $(ODIR)/Legendre2D.o $(ODIR)/Histogram.o

clean:
	- rm odir/*.o

$(BINDIR)/sftest: $(ODIR)/sftest.o $(ODIR)/StarFinder.o $(ODIR)/Bounds.o $(ODIR)/BinomFact.o $(ODIR)/Legendre2D.o $(ODIR)/Histogram.o $(CFLIB) $(TMVLIB)
	$(CC) $(CFLAGS) $(ODIR)/sftest.o $(ODIR)/StarFinder.o $(ODIR)/Bounds.o $(ODIR)/BinomFact.o $(ODIR)/Legendre2D.o $(ODIR)/Histogram.o $(CFLIB) $(TMVLIB) -lm -o $(BINDIR)/sftest
 

$(ODIR)/sftest.o: sftest.cpp StarFinder.h Histogram.h Bounds.h PotentialStar.h Legendre2D.h 	
	$(CC) $(CFLAGS) -c sftest.cpp -o $(ODIR)/sftest.o



$(ODIR)/StarFinder.o: StarFinder.cpp StarFinder.h Histogram.h Bounds.h PotentialStar.h Legendre2D.h	
	$(CC) $(CFLAGS) -c StarFinder.cpp -o $(ODIR)/StarFinder.o

#$(BINDIR)/findstars: $(ODIR)/FindStars.o $(ODIR)/FindStarsLib.o $(ODIR)/Histogram.o $(ODIR)/Legendre2D.o $(ODIR)/Bounds.o $(ODIR)/BinomFact.o $(TMVLIB)
#	$(CC) $(CFLAGS) $(ODIR)/FindStars.o  $(ODIR)/FindStarsLib.o $(ODIR)/Histogram.o $(ODIR)/Legendre2D.o $(ODIR)/Bounds.o $(ODIR)/BinomFact.o $(TMV) -lm -o $(BINDIR)/findstars

$(ODIR)/BinomFact.o: BinomFact.cpp BinomFact.h 
	$(CC) $(CFLAGS) -c BinomFact.cpp -o $(ODIR)/BinomFact.o

$(ODIR)/Bounds.o: Bounds.cpp Bounds.h 
	$(CC) $(CFLAGS) -c Bounds.cpp -o $(ODIR)/Bounds.o

#$(ODIR)/FindStars.o: FindStars.cpp FindStarsLib.cpp FindStarsLib.h Histogram.h Bounds.h PotentialStar.h Legendre2D.h
#	$(CC) $(CFLAGS) -c FindStars.cpp -o $(ODIR)/FindStars.o

#$(ODIR)/FindStarsLib.o: FindStarsLib.cpp FindStarsLib.h Histogram.h Bounds.h PotentialStar.h Legendre2D.h
#	$(CC) $(CFLAGS) -c FindStarsLib.cpp -o $(ODIR)/FindStarsLib.o

$(ODIR)/Legendre2D.o: Legendre2D.cpp Legendre2D.h Bounds.h
	$(CC) $(CFLAGS) -c Legendre2D.cpp -o $(ODIR)/Legendre2D.o

$(ODIR)/Histogram.o: Histogram.cpp
	$(CC) $(CFLAGS) -c Histogram.cpp -o $(ODIR)/Histogram.o

