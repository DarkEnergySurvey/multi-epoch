dnl @synopsis ACX_ATLAS([ATLAS_LIBDIR, ATLAS_INCDIR, ATLAS_PFLAG, [ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]]])
dnl This macro figures out if the ATLAS library and header files
dnl are installed.
dnl You may wish to use these variables in your default LIBS:
dnl
dnl        LIBS="$ATLAS_LIBS $LIBS"
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if BLAS/LAPACK
dnl is found (HAVE_ATLAS is defined first), and ACTION-IF-NOT-FOUND
dnl is a list of commands to run it if it is not found.
dnl
dnl @version $Id: acx_atlas.m4,v 1.0 2007/10/19 21:30:17 bertin Exp $
dnl @author Emmanuel Bertin <bertin@iap.fr>

AC_DEFUN([ACX_ATLAS], [
AC_REQUIRE([AC_CANONICAL_HOST])

dnl --------------------
dnl Search include files
dnl --------------------

acx_atlas_ok=no
if test x$2 = x; then
  if test x$1 = x; then
    AC_CHECK_HEADERS([cblas.h clapack.h],[acx_atlas_ok=yes])
    if test x$acx_atlas_ok = xyes; then
      AC_DEFINE(ATLAS_BLAS_H, "cblas.h", [BLAS header filename.])
      AC_DEFINE(ATLAS_LAPACK_H, "clapack.h", [CLAPACK header filename.])
    else
      AC_CHECK_HEADERS([atlas/cblas.h atlas/clapack.h],[acx_atlas_ok=yes])
      if test x$acx_atlas_ok = xyes; then
        AC_DEFINE(ATLAS_BLAS_H, "atlas/cblas.h", [BLAS header filename.])
        AC_DEFINE(ATLAS_LAPACK_H, "atlas/clapack.h", [CLAPACK header filename.])
      else
        atlas_def=/usr/local/atlas
        AC_CHECK_HEADERS(
		[$atlas_def/include/cblas.h $atlas_def/include/clapack.h],
		[acx_atlas_ok=yes])
        if test x$acx_atlas_ok = xyes; then
          AC_DEFINE_UNQUOTED(ATLAS_BLAS_H, "$atlas_def/include/cblas.h",
		[BLAS header filename.])
          AC_DEFINE_UNQUOTED(ATLAS_LAPACK_H, "$atlas_def/include/clapack.h",
		[CLAPACK header filename.])
        else
          atlas_def=/usr/atlas
          AC_CHECK_HEADERS(
		[$atlas_def/include/cblas.h $atlas_def/include/clapack.h],
		[acx_atlas_ok=yes])
          if test x$acx_atlas_ok = xyes; then
            AC_DEFINE_UNQUOTED(ATLAS_BLAS_H, "$atlas_def/include/cblas.h",
		[BLAS header filename.])
            AC_DEFINE_UNQUOTED(ATLAS_LAPACK_H, "$atlas_def/include/clapack.h",
		[CLAPACK header filename.])
          else
            ATLAS_ERROR="CBLAS/LAPack include files not found!"
          fi
        fi
      fi
    fi
  else
    AC_CHECK_HEADERS([$1/include/cblas.h $1/include/clapack.h],
		[acx_atlas_ok=yes])
    if test x$acx_atlas_ok = xyes; then
      AC_DEFINE_UNQUOTED(ATLAS_BLAS_H, "$1/include/cblas.h",
		[BLAS header filename.])
      AC_DEFINE_UNQUOTED(ATLAS_LAPACK_H, "$1/include/clapack.h",
		[CLAPACK header filename.])
    else
      AC_CHECK_HEADERS([cblas.h clapack.h],[acx_atlas_ok=yes])
      if test x$acx_atlas_ok = xyes; then
        AC_DEFINE_UNQUOTED(ATLAS_BLAS_H, "cblas.h",
		[BLAS header filename.])
        AC_DEFINE_UNQUOTED(ATLAS_LAPACK_H, "clapack.h",
		[CLAPACK header filename.])
      else
        ATLAS_ERROR="CBLAS/LAPack include files not found in $1/include!"
      fi
    fi
  fi
else
  AC_CHECK_HEADERS([$2/cblas.h $2/clapack.h], [acx_atlas_ok=yes])
  if test x$acx_atlas_ok = xyes; then
    AC_DEFINE_UNQUOTED(ATLAS_BLAS_H, "$2/cblas.h",
		[BLAS header filename.])
    AC_DEFINE_UNQUOTED(ATLAS_LAPACK_H, "$2/clapack.h",
		[CLAPACK header filename.])
  else
    ATLAS_ERROR="CBLAS/LAPack include files not found in $2!"
  fi
fi

dnl --------------------
dnl Search library files
dnl --------------------

if test x$acx_atlas_ok = xyes; then
  OLIBS="$LIBS"
  LIBS=""
  if test x$1 = x; then
    AC_CHECK_LIB(lapack, [clapack_dpotrf],, [acx_atlas_ok=no],
		[-lcblas -latlas -lm])
    AC_CHECK_LIB(cblas, cblas_dgemm,, [acx_atlas_ok=no],
		[-latlas -lm])
    if test x$acx_atlas_ok = xyes; then
      ATLAS_LIBPATH=""
    else
      atlas_def=/usr/local/atlas
      unset ac_cv_lib_lapack_clapack_dpotrf
      unset ac_cv_lib_cblas_cblas_dgemm
      acx_atlas_ok=yes
      AC_CHECK_LIB(lapack, [clapack_dpotrf],, [acx_atlas_ok=no],
		[-L$atlas_def/lib -lcblas -latlas -lm])
      AC_CHECK_LIB(cblas, cblas_dgemm,, [acx_atlas_ok=no],
		[-L$atlas_def/lib -latlas -lm])
      if test x$acx_atlas_ok = xyes; then
        ATLAS_LIBPATH="-L$atlas_def/lib"
      else
        atlas_def=/usr/atlas
        unset ac_cv_lib_lapack_clapack_dpotrf
        unset ac_cv_lib_cblas_cblas_dgemm
        acx_atlas_ok=yes
        AC_CHECK_LIB(lapack, [clapack_dpotrf],, [acx_atlas_ok=no],
		[-L$atlas_def/lib -lcblas -latlas -lm])
        AC_CHECK_LIB(cblas, cblas_dgemm,, [acx_atlas_ok=no],
		[-L$atlas_def/lib -latlas -lm])
        if test x$acx_atlas_ok = xyes; then
          ATLAS_LIBPATH="-L$atlas_def/lib"
        else
          ATLAS_ERROR="CBLAS/LAPack library files not found at usual locations!"
        fi
      fi
    fi
  else
    AC_CHECK_LIB(lapack, [clapack_dpotrf],, [acx_atlas_ok=no],
		[-L$1 -lcblas -latlas -lm])
    AC_CHECK_LIB(cblas, cblas_dgemm,, [acx_atlas_ok=no],
		[-L$1 -latlas -lm])
    if test x$acx_atlas_ok = xyes; then
      ATLAS_LIBPATH="-L$1"
    else
      unset ac_cv_lib_lapack_clapack_dpotrf
      unset ac_cv_lib_cblas_cblas_dgemm
      acx_atlas_ok=yes
      AC_CHECK_LIB(lapack, [clapack_dpotrf],, [acx_atlas_ok=no],
		[-L$1/lib -lcblas -latlas -lm])
      AC_CHECK_LIB(cblas, cblas_dgemm,, [acx_atlas_ok=no],
		[-L$1/lib -latlas -lm])
      if test x$acx_atlas_ok = xyes; then
        ATLAS_LIBPATH="-L$1/lib"
      else
        ATLAS_ERROR="CBLAS/LAPack library files not found in $1!"
      fi
    fi
  fi
  LIBS="$OLIBS"
fi

AC_SUBST(ATLAS_LIBPATH)
AC_SUBST(ATLAS_CFLAGS)

dnl -------------------------------------------------------------------------
dnl Finally, check MP version and execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND
dnl -------------------------------------------------------------------------
if test x"$acx_atlas_ok" = xyes; then
  AC_DEFINE(HAVE_ATLAS,1,
	[Define if you have the ATLAS libraries and header files.])
  if test x$3 = xyes; then
dnl Check whether the multithreaded version of ATLAS is there too:
    AC_CHECK_LIB(ptcblas, cblas_dgemm,, [acx_atlas_ok=no],
	[-L$ATLAS_LIBPATH -lcblas -latlas -lm])
    if test x$acx_atlas_ok = xyes; then
      ATLAS_LIB="$ATLAS_LIBPATH -llapack -lptcblas -lcblas -latlas"
      AC_SUBST(ATLAS_LIB)
      AC_DEFINE(HAVE_ATLAS_MP,1,
	[Define if you have the parallel ATLAS libraries.])
      $4
    else
      ATLAS_ERROR="CBLAS/LAPack was compiled without multithreading support!"
      AC_SUBST(ATLAS_ERROR)
      $5         
    fi
  else
    ATLAS_LIB="$ATLAS_LIBPATH -llapack -lcblas -latlas"
    AC_SUBST(ATLAS_LIB)
    $4
  fi
else
  AC_SUBST(ATLAS_ERROR)
  $5
fi

])dnl ACX_ATLAS
