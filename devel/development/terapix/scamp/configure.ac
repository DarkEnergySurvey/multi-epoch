# configure.in for SCAMP
# (C) E.Bertin 2002-2007
# Process this file with autoconf to produce a configure script.
# First, disable the annoying config.cache
define([AC_CACHE_LOAD],)
define([AC_CACHE_SAVE],)

# This is your standard Bertin source code...
AC_INIT(scamp, 1.4.0, [bertin@iap.fr])
AC_CONFIG_SRCDIR(src/makeit.c)
AC_CONFIG_AUX_DIR(autoconf)
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE
date=`date +%Y-%m-%d`
date2=`date +"%a %b %d %Y"`
date3=`date +"%B %Y"`
AC_DEFINE_UNQUOTED(DATE, "$date", [Archive creation date])
AC_SUBST(PACKAGER, "Emmanuel Bertin")
AC_SUBST(DATE2, "$date2")
AC_SUBST(DATE3, "$date3")

# Include macros
sinclude(acx_atlas.m4)
sinclude(acx_cdsclient.m4)
sinclude(acx_fftw.m4)
sinclude(acx_plplot.m4)
sinclude(acx_prog_cc_optim.m4)
sinclude(acx_pthread.m4)
sinclude(acx_urbi_resolve_dir.m4)

# Display pakage and version number
AC_MSG_RESULT([******** Configuring:  $PACKAGE_NAME $VERSION ($date) ********])

# Initialize the list of compilers to consider
cclist="cc gcc"
# Backup and reset the input CFLAGS
mycflags="$CFLAGS"
CFLAGS=""

# Provide special option for the Linux Intel C compiler
AC_MSG_CHECKING([for Linux Intel C compiler mode])
AC_ARG_ENABLE(icc,
	[AC_HELP_STRING([--enable-icc],
	[Enable special mode for compilation with the Intel compiler \
(off by default)])],
        use_icc="yes"
        cclist="icc $cclist"
	AC_MSG_RESULT([yes]),
        use_icc="no"
	AC_MSG_RESULT([no]))

# Checks for programs.
# GCC is chosen last because it is likely to yield less optimized code
AC_LANG(C)
AC_PROG_CC([$cclist])
# C Compiler: Check that it is ANSI C
AM_PROG_CC_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# C Compiler: Check that it is POSIX-compliant
AC_ISC_POSIX
ACX_PROG_CC_OPTIM
#AC_DISABLE_SHARED
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB(m, sin)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h sys/mman.h])
echo 
# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MMAP
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([atexit getenv memcpy memmove memset mkdir munmap strstr \
sysconf setlinebuf])

datadir2=$(URBI_RESOLVE_DIR([$datadir]))

# Provide a special option for PlPLOT
AC_ARG_WITH(plplot,
	[AC_HELP_STRING([--with-plplot@<:@=<PLPlot dir>@:>@],
	[Provide an alternative path to the PLPlot library])],
    use_plplot="yes"
    if test "$withval" != "yes"; then
      plplot_dir=$withval
    fi,
    use_plplot="yes"
    )
AC_ARG_WITH(plplot,
	[AC_HELP_STRING([--without-plplot],
	[Link without the PLPlot library (with is the default)])],
    use_plplot="no",
    use_plplot="yes"
    [ACX_PTHREAD]
    )
# Provide special option for debugging
AC_MSG_CHECKING([for debugger mode])
AC_ARG_ENABLE(debug,
	[AC_HELP_STRING([--enable-debug],
	[Enable special mode for debugging (off by default)])],
        use_debug="yes"
	AC_MSG_RESULT([yes]),
        use_debug="no"
	AC_MSG_RESULT([no]))

# Provide a special option for ATLAS
AC_ARG_WITH(atlas,
	[AC_HELP_STRING([--with-atlas=<ATLAS library path>],
	[Provide an alternative path to the ATLAS library])],
    atlas_libdir=$withval,
    atlas_libdir=""
    )

# Provide a special option for CDSClient
AC_ARG_WITH(cdsclient_exec,
	[AC_HELP_STRING([--with-cdsclient_exec=<aclient executable>],
	[Provide an alternative default path to the aclient executable])],
    cdsclient_exec="$withval",
    cdsclient_exec="aclient"
    )

# Provide a special option for the default XSLT URL
AC_ARG_WITH(xsl_url,
	[AC_HELP_STRING([--with-xsl_url=<default URL for XSLT filter>],
	[Provide an alternative default URL of the XSLT filter])],
    [xsl_url=$withval],
    [xsl_url="file://$datadir2/$PACKAGE_NAME/$PACKAGE_NAME.xsl"]
    )

AC_DEFINE_UNQUOTED([XSL_URL], "$xsl_url",[Default URL of the XSLT filter])

# Set flags for multithreading
AC_ARG_ENABLE(threads,
	[AC_HELP_STRING([--enable-threads@<:@=<max_number_of_threads>@:>@],
		[Enable multhreading (off by default)])],
    if test "$enableval" = "yes"; then
      n_pthreads=16
    else
      n_pthreads=$enableval
    fi
    use_pthreads=yes,
    n_pthreads=1
    use_pthreads=no
    )

# Provide special option for profiling
AC_MSG_CHECKING([for profile mode])
AC_ARG_ENABLE(prof,
	[AC_HELP_STRING([--enable-prof],
	[Enable special mode for profiling (off by default)])],
        use_prof="yes"
	AC_MSG_RESULT([yes]),
        use_prof="no"
	AC_MSG_RESULT([no]))

# Provide a special option for static linking
AC_MSG_CHECKING([for static linking])
AC_ARG_ENABLE(static,
	[AC_HELP_STRING([--enable-static],
	[Choose linking with static libraries (off by default)])],
        use_static=$enableval
	AC_MSG_RESULT([yes]),
        use_static="no"
	AC_MSG_RESULT([no]))

################# Actions to complete in case of multhreading ################
AC_DEFINE_UNQUOTED(THREADS_NMAX, $n_pthreads,[Maximum number of POSIX threads])
if test "$use_pthreads" = "yes"; then
  AC_MSG_CHECKING([for multithreading])
  AC_MSG_RESULT([maximum of $n_pthreads thread(s)])
  AC_DEFINE(USE_THREADS, 1, [Triggers multhreading])
# CC, CFLAGS and LIBS are system and compiler-dependent
  ACX_PTHREAD
  CC="$PTHREAD_CC"
  [CFLAGS="$CFLAGS $PTHREAD_CFLAGS -D_REENTRANT"]
  LIBS="$PTHREAD_LIBS $LIBS"
fi
AM_CONDITIONAL(USE_THREADS, test $use_pthreads = "yes")

ACX_FFTW(,[use_fftw=yes],[use_fftw=no])
ACX_ATLAS($atlas_libdir,[use_atlas=yes],[use_atlas=no])
ACX_CDSCLIENT($cdsclient_exec,[use_cdsclient=yes],[use_cdsclient=no])
AC_DEFINE_UNQUOTED(CDSCLIENT, "$cdsclient_exec", [CDSclient executable])
if test "$use_plplot" = "yes"; then
  ACX_PLPLOT(,,[use_plplot=no])
  if test "$use_plplot" = "no"; then
    AC_MSG_WARN(PLPlot library not found! Plotting deactivated.)
  fi
fi

################ handle the FFTW library (Fourier transforms) ################
if test "$use_fftw" = "yes"; then
  if test "$use_pthreads" = "yes"; then
    if  test "$acx_fftwft_ok" = "no"; then
      AC_MSG_ERROR([Threaded version of the FFTW library not found! Exiting.])
    fi
    LIBS="$FFTWFT_LIBS $LIBS"
  else
    LIBS="$FFTWF_LIBS $LIBS"
  fi
else
  AC_MSG_ERROR([FFTW library not found! Exiting.])
fi

################## handle the ATLAS library(linear algebra) ##################
if test "$use_atlas" = "yes"; then
  if test "$use_pthreads" = "yes"; then
    LIBS="$ATLAS_LIBPATH -llapack -lptcblas -lcblas -latlas $LIBS"
  else
    LIBS="$ATLAS_LIBPATH -llapack -lcblas -latlas $LIBS"
  fi
else
  AC_MSG_ERROR([CBLAS/LAPack library not found! Exiting.])
fi

################# handle the CDSclient package (CDS queries) #################
if test "$use_cdsclient" = "no"; then
  AC_MSG_ERROR(["$cdsclient_exec" not found. exiting.]
["aclient" from the CDSclient package is required to use SCAMP:]
[1. Install the CDSclient package: http://cdsweb.u-strasbg.fr/doc/cdsclient.html]
[2. Use the --with-cdsclient_exec=<aclient_exec> configure option if aclient is
   not in your PATH.])
fi

################# handle the PLPlot library (graphic plots) ##################
if test "$use_plplot" = "yes"; then
  AC_SUBST(PLPLOT_CFLAGS)
  [CFLAGS="$CFLAGS $PLPLOT_CFLAGS"]
  [LIBS="$PLPLOT_LIBS $PTHREAD_LIBS $LIBS"]
  fi
AM_CONDITIONAL(USE_PLPLOT, test $use_plplot = "yes")

# Special optimization options for the INTEL C compiler
if test $use_icc = "yes" && test $CC = "icc"; then
  CFLAGS="$CFLAGS -O3 -axPW -ip -unroll"
fi

# Debugging
if test "$use_debug" = "yes"; then
  CFLAGS="-g $CFLAGS"
fi

# Profiling
if test "$use_prof" = "yes"; then
  CFLAGS="-pg -O3 -Wall $CFLAGS"
fi

# Static linking option
if test "$use_static" = "yes"; then
  LDFLAGS="-all-static $LDFLAGS"
fi

# Link with e-fence option
if test "$use_efence" = "yes"; then
  LIBS="-lefence $LIBS"
fi

# Override automatic CFLAGS and LDFLAGS with those of user
if test -n "$mycflags"; then
CFLAGS="$mycflags"
fi
# Display compiler and linker flags
AC_MSG_RESULT([***************************************************************])
AC_MSG_RESULT([Compile cmdline:  $CC $CFLAGS])
AC_MSG_RESULT([Link    cmdline:  $CC $LDFLAGS $LIBS])
AC_MSG_RESULT([Default XSLT URL: $xsl_url])
AC_MSG_RESULT([***************************************************************])
AC_CONFIG_FILES([
Makefile
man/Makefile
src/Makefile
src/fits/Makefile
src/wcs/Makefile
scamp.spec
scamp-mp.spec
man/scamp.1])
AC_OUTPUT
