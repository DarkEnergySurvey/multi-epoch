#ifndef INSER_LOTS_H_
#define INSER_LOTS_H_

#include <sqlca.h>

#define MAXLISTS 500
#define MAXIMAGE 100
#define SQLLDR_LIMIT 50000
#define LEN_FIELDS 100
#define NUM_FIELDS 9
#define STRING_FIELDS (LEN_FIELDS*NUM_FIELDS)


typedef unsigned long long uint64;

int htmdepth=20;

fitsfile *fptr;      /* FITS file pointer, defined in fitsio.h */
char *val, value[1000], nullstr[]="*";
char keyword[FLEN_KEYWORD], colname[FLEN_VALUE], comment[500];
int status = 0;   /*  CFITSIO status value MUST be initialized to zero!  */
int hdunum, hdutype, ncols, ii, anynul, dispwidth[1000], parentid, softid;
int i,firstcol= 1, lastcol, linewidth, imageid,flag_ascii=0;
long jj, nrows;
float magzp,equinox=2000.0;
float *magaper,*magerrap,*magauto,*magerrauto,*threshold,*ximage;
float *yimage,*aimage,*aerrimage,*bimage,*berrimage,*thetaimage;
float *thetaerrimage,*ellipticity,*class,floatnull=0;
int *objnum,*xminimage,*yminimage,*xmaximage,*ymaximage;
short *fflags,shortnull=0;
int **pointer,**nullpointer, j,k,*typecode,intnull=0;
char **prstring;
long *repeat,*width,longnull=0;
double doublenull=0.0,*alphara,*deltadec,*alphapeak,*deltapeak,*x2world;
double *x2errworld,*y2world,*y2errworld,*xyworld,*xyerrworld,*x2image;
double *x2errimage,*y2image,*y2errimage,*xyimage,*xyerrimage;
char band[5], command[1000],line[1000];
double *cx,*cy,*cz;


void getxyz(), set_piece(), insert_objects();
uint64 cc_radec2ID();
uint64 *htmID;

/* for ascii input */
FILE  *pip,*fin;
int objnum_ascii;
double magauto_ascii, magerrauto_ascii;
double magaper1_ascii, magerrap1_ascii, magaper2_ascii, magerrap2_ascii, magaper3_ascii, magerrap3_ascii, magaper4_ascii, magerrap4_ascii, magaper5_ascii, magerrap5_ascii, magaper6_ascii, magerrap6_ascii;
double alphara_ascii, deltadec_ascii, alphapeak_ascii, deltapeak_ascii;
double x2world_ascii, x2errworld_ascii, y2world_ascii, y2errworld_ascii, xyworld_ascii;
double xyerrworld_ascii, threshold_ascii, ximage_ascii, yimage_ascii;
int xminimage_ascii, yminimage_ascii, xmaximage_ascii, ymaximage_ascii;
double x2image_ascii, x2errimage_ascii, y2image_ascii, y2errimage_ascii;
double xyimage_ascii, xyerrimage_ascii, aimage_ascii, aerrimage_ascii;
double bimage_ascii, berrimage_ascii, thetaimage_ascii, thetaerrimage_ascii;
double ellipticity_ascii, class_ascii;
int fflags_ascii;

desw_ctx ctx;
r_set rs1;

#endif /*INSER_LOTS_H_*/

