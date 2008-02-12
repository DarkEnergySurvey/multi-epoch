# Makefile for Recipes: BSD UNIX distribution
#

CC = $(CCKR)
CFLAGS = $(CFLAGSKR)

OBJS =	damoeba.o damotry.o dbeschb.o dbessj0.o dbessj1.o dbessj.o \
	dbessjy.o dchebev.o dfourn.o dhunt.o djacobi.o dmidinf.o dmidpnt.o \
	dnrutil.o dodeint2.o dodeint3.o dodeint.o dpolint2.o dpolint.o dqromb2.o \
	dqromb.o dqromo.o dqsimp.o drk42.o drk43.o drk4.o drkck2.o drkck3.o \
	drkck.o drkqs2.o drkqs3.o drkqs.o drlft3.o dsphbes.o dspline.o dsplint.o \
	dtrapzd2.o dtrapzd.o dgammln.o dgammp.o dgammq.o dgcf.o dgser.o

#If any of the above have problems with an optimization option or other
#compiler flags, a specific rule will be needed for them. This is very
#simple to write. For example, for svdcmp (which sometimes has problems
#with optimizers), a very plain rule is:
#svdcmp.o: svdcmp.c ; $(CC) -I../include -c $<
#Uncomment or write analogously for other recipes as needed.

all: binrecipes

binrecipes: $(CKRLIB)

$(CKRLIB): $(OBJS)
	$(AR) $(ARFLAGS) $(CKRLIB) $(OBJS)
	ranlib $(CKRLIB)

install: binrecipes
	cp $(CKRLIB) $(LIBDIR)
	ranlib $(LIBDIR)/$(CKRLIB)

# Not cp -p, ranlib -t - can't assume -p option for all systems

clean:	
	/bin/rm -f *.o *.a badluk fredex sfroid sphfpt sphoot
# DO NOT DELETE THIS LINE -- make depend depends on it.

