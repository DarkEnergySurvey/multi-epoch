# Main Makefile for SCAMP
# Copyright (C) 2002-2007 Emmanuel Bertin.
AUTOMAKE_OPTIONS	= foreign no-dependencies
SUBDIRS			= man src
dist_pkgdata_DATA	= config/*.ahead doc xsl/scamp.xsl
EXTRA_DIST		= AUTHORS BUGS ChangeLog COPYING \
			  HISTORY INSTALL README THANKS \
			  acx_pthread.m4 acx_prog_cc_optim.m4 \
			  acx_atlas.m4 acx_cdsclient.m4 acx_fftw.m4 \
			  acx_plplot.m4 acx_urbi_resolve_dir.m4
RPM_ROOTDIR		= `rpmbuild --nobuild -E %_topdir`
RPM_SRCDIR		= $(RPM_ROOTDIR)/SOURCES
rpm:	dist
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	rpmbuild -ba --clean $(PACKAGE_NAME).spec $(PACKAGE_NAME)-mp.spec

rpm-opteron:	dist
	echo @PLPLOT_CFLAGS@
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="gcc" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -g -funroll-loops -fomit-frame-pointer -Wall -march=opteron" TPXLIBS="-lplplotd -lpthread -llapack -lcblas -latlas -lfftw3f -lm" rpmbuild -ba --target=x86_64 --clean $(PACKAGE_NAME).spec                                                            
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="gcc" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -g -pthread -funroll-loops -fomit-frame-pointer -Wall -march=opteron" TPXLIBS="-lplplotd -lpthread -llapack -lptcblas -latlas -lfftw3f -lm" rpmbuild -ba --target=x86_64 --clean $(PACKAGE_NAME)-mp.spec                                                               

rpm-athlon:	dist
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="gcc" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -g -funroll-loops -fomit-frame-pointer -Wall -m32 -march=athlon -msse -mfpmath=sse -mtune=athlon" TPXLIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lcblas -latlas -lfftw3f -lm" rpmbuild -ba --target=athlon --clean $(PACKAGE_NAME).spec
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="gcc" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -g -pthread -funroll-loops -fomit-frame-pointer -Wall -m32 -march=athlon -msse -mfpmath=sse -mtune=athlon" TPXLIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lptcblas -latlas -lfftw3f -lm" rpmbuild -ba --target=athlon --clean $(PACKAGE_NAME)-mp.spec

rpm-pentium:	dist
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="gcc" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -g -pthread -funroll-loops -fomit-frame-pointer -Wall -m32 -march=i686 -msse -mfpmath=sse -mno-3dnow" TPXLIBS="-L/usr/lib/ -lpthread -L/usr/local/lib/atlas_pentium/ -llapack -lcblas -latlas -L/usr/local/lib/ -lplplotd -lfftw3f -lm" rpmbuild -ba --target=i686 --clean $(PACKAGE_NAME).spec
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="gcc" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -g -pthread -funroll-loops -fomit-frame-pointer -Wall -m32 -march=i686 -msse -mfpmath=sse -mno-3dnow" TPXLIBS="-L/usr/lib/ -lpthread -L/usr/local/lib/atlas_pentium/ -llapack -lptcblas -latlas -L/usr/local/lib/ -lplplotd -lfftw3f -lm" rpmbuild -ba --target=i686 --clean $(PACKAGE_NAME)-mp.spec

rpm-opteron-icc:	dist
	echo @PLPLOT_CFLAGS@
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="icc" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -axWP -ip -unroll" TPXLIBS="-lplplotd -lpthread -llapack -lcblas -latlas -lfftw3f -lm" rpmbuild -ba --target=x86_64 --clean $(PACKAGE_NAME).spec                                                            
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="icc" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -axWP -ip -unroll -pthread" TPXLIBS="-lplplotd -lpthread -llapack -lptcblas -latlas -lfftw3f -lm" rpmbuild -ba --target=x86_64 --clean $(PACKAGE_NAME)-mp.spec                                                               

rpm-athlon-icc:	dist
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="icc32" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -axKWNPB -ip -unroll" TPXLIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lcblas -latlas -lfftw3f -lm" rpmbuild -ba --target=athlon --clean $(PACKAGE_NAME).spec
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="icc32" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -axKWNPB -ip -unroll -pthread" TPXLIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lptcblas -latlas -lfftw3f -lm" rpmbuild -ba --target=athlon --clean $(PACKAGE_NAME)-mp.spec

rpm-pentium-icc:	dist
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="icc32" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -axKWNPB -ip -unroll -pthread" TPXLIBS="-L/usr/lib/ -lpthread -L/usr/local/lib/atlas_pentium/ -llapack -lcblas -latlas -L/usr/local/lib/ -lplplotd -lfftw3f -lm" rpmbuild -ba --target=i686 --clean $(PACKAGE_NAME).spec
	cp -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(RPM_SRCDIR)
	TPXCC="icc32" TPXFLAGS="@PLPLOT_CFLAGS@ -O3 -axKWNPB -ip -unroll -pthread" TPXLIBS="-L/usr/lib/ -lpthread -L/usr/local/lib/atlas_pentium/ -llapack -lptcblas -latlas -L/usr/local/lib/ -lplplotd -lfftw3f -lm" rpmbuild -ba --target=i686 --clean $(PACKAGE_NAME)-mp.spec

athlon:
	$(MAKE) CFLAGS="@PLPLOT_CFLAGS@ -O3 -g -pthread -funroll-loops -fomit-frame-pointer -Wall -m32 -march=athlon -msse -mfpmath=sse -mtune=athlon" LIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lptcblas -latlas -lfftw3f -lm"

pentium:
	$(MAKE) CFLAGS="@PLPLOT_CFLAGS@ -O3 -g -pthread -funroll-loops -fomit-frame-pointer -Wall -m32 -march=i686 -msse -mfpmath=sse -mno-3dnow" LIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lptcblas -latlas -lfftw3f -lm"

icc:
	$(MAKE) CC="icc" CFLAGS="@PLPLOT_CFLAGS@ -O3 -axWP -ip -unroll -pthread"

athlon-icc:
	$(MAKE) CC="icc32" CFLAGS="@PLPLOT_CFLAGS@ -O3 -axKWNPB -ip -unroll -pthread" LIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lptcblas -latlas -lfftw3f -lm"

pentium-icc:
	$(MAKE) CC="icc32" CFLAGS="@PLPLOT_CFLAGS@ -O3 -axKWNPB -ip -unroll -pthread" LIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lptcblas -latlas -lfftw3f -lm"

debug:
	$(MAKE) CFLAGS="@PLPLOT_CFLAGS@ -O3 -funroll-loops -fomit-frame-pointer -Wall -m32 -g" LIBS="-L/usr/local/lib/ -L/usr/lib/ -lplplotd -lpthread -llapack -lcblas -latlas -lfftw3f -lm"

profile:
	$(MAKE) CFLAGS="@PLPLOT_CFLAGS@ -O3 -funroll-loops -Wall -pg"
