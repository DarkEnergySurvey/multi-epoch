/* CAUTION: This is the traditional K&R C (only) version of the Numerical
   Recipes utility file complex.h.  Do not confuse this file with the
   same-named file complex.h that is supplied in the 'misc' subdirectory.
   *That* file is the one from the book, and contains both ANSI and
   traditional K&R versions, along with #ifdef macros to select the
   correct version.  *This* file contains only traditional K&R.           */

#ifndef _NR_COMPLEX_H_
#define _NR_COMPLEX_H_

#ifndef _FCOMPLEX_DECLARE_T_
typedef struct FCOMPLEX {float r,i;} fcomplex;
#define _FCOMPLEX_DECLARE_T_
#endif /* _FCOMPLEX_DECLARE_T_ */

fcomplex Cadd();
fcomplex Csub();
fcomplex Cmul();
fcomplex Complex();
fcomplex Conjg();
fcomplex Cdiv();
float Cabs();
fcomplex Csqrt();
fcomplex RCmul();

#endif /* _NR_COMPLEX_H_ */
