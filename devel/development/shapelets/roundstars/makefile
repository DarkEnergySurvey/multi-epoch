CC= g++
#CFLAGS= -Wall -Werror -I../tmv0.54 -O3 
CFLAGS= -Wall -I../tmv0.54 -O3 
TMV= -L../tmv0.54/gcc-noblas -lsmalltmv -lm 
TMVLIB= ../tmv0.54/gcc-noblas/libsmalltmv.a
ODIR= odir

#FITS= -lcfitsio
#FITSI= -I/usr/local/include
FITS= -L../cfitsio -lcfitsio
FITSI= -I../cfitsio 
BINDIR= .

default: all
all : roundstars

clean:
	- rm odir/*.o
	- rm roundstars


roundstars: $(ODIR)/RoundStars.o $(ODIR)/Image.o $(ODIR)/Legendre2D.o $(ODIR)/Kernel.o $(ODIR)/FullShape.o $(ODIR)/BinomFact.o $(ODIR)/Star.o $(ODIR)/Bounds.o $(TMVLIB)
	$(CC) $(CFLAGS) $(ODIR)/RoundStars.o $(ODIR)/Image.o $(ODIR)/Legendre2D.o $(ODIR)/Kernel.o $(ODIR)/FullShape.o $(ODIR)/BinomFact.o $(ODIR)/Star.o $(ODIR)/Bounds.o $(TMV) -lm $(FITS) -o roundstars

$(ODIR)/BinomFact.o: BinomFact.cpp
	$(CC) $(CFLAGS) -c BinomFact.cpp -o $(ODIR)/BinomFact.o

$(ODIR)/Bounds.o: Bounds.cpp Bounds.h
	$(CC) $(CFLAGS) -c Bounds.cpp -o $(ODIR)/Bounds.o

$(ODIR)/RoundStars.o: RoundStars.cpp Kernel.h RSParams.h Image.h FullShape.h Star.h  Legendre2D.h Chunk.h
	$(CC) $(CFLAGS) -c RoundStars.cpp -o $(ODIR)/RoundStars.o

$(ODIR)/FullShape.o: FullShape.cpp FullShape.h Legendre2D.h Kernel.h Image.h Chunk.h Star.h RSParams.h
	$(CC) $(CFLAGS) -c FullShape.cpp -o $(ODIR)/FullShape.o

$(ODIR)/Image.o: Image.cpp Image.h Legendre2D.h Kernel.h Chunk.h
	$(CC) $(CFLAGS) -c Image.cpp  $(FITSI) -o $(ODIR)/Image.o

$(ODIR)/Kernel.o: Kernel.cpp Legendre2D.h Kernel.h Chunk.h
	$(CC) $(CFLAGS) -c Kernel.cpp -o $(ODIR)/Kernel.o

$(ODIR)/Star.o: Star.cpp FullShape.h Kernel.h Chunk.h RSParams.h
	$(CC) $(CFLAGS) -c Star.cpp -o $(ODIR)/Star.o

$(ODIR)/Legendre2D.o: Legendre2D.cpp Legendre2D.h Bounds.h Function2D.cpp Function2D.h
	$(CC) $(CFLAGS) -c Legendre2D.cpp -o $(ODIR)/Legendre2D.o

