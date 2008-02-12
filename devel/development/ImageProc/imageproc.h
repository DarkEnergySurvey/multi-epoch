#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "fitsio.h"


#define Squ(x) ((x)*(x))
#define Cube(x) ((x)*(x)*(x))
#define Quad(x) (Squ(x)*Squ(x))
#define FIT 1
#define CONSTANT 0
#define FAST 1
#define NO 0
#define YES 1
#define SLOW 0
#define RAD2DEG 57.29578
#define MEM(st) printf("%s\n",st);system("ps aux|grep sim_ensemble");fflush(stdout);

#define GBAND 0
#define RBAND 1
#define IBAND 2
#define ZBAND 3

#define DES_IMAGE 1
#define DES_VARIANCE 2
#define DES_MASK 3
#define AVERAGE 0
#define MEDIAN 1
#define AVSIGCLIP 2
#define CLIPPEDMEDIAN 3
#define AVSIGCLIPMAXNUM 10000	/* number of sigmas to calculate for the average value */
#define VARIANCE_DELTAPIXEL 1  /* use 3X3 square centered on each pixel */
			       /* for variance */
#define VARIANCE_DIRECT 1 /* calculate variance directly */
#define VARIANCE_CCD 2	  /* assume CCD noise is dominating variance */

/* Image structure */
typedef struct {
	char	name[1000];
	char	biasseca[100],biassecb[100],
		ampseca[100], ampsecb[100],
		trimsec[100], datasec[100];
	long	axes[7],bscale,bzero,bitpix,npixels,fpixel;
	float	saturateA,saturateB,
		gainA,gainB,
		rdnoiseA,rdnoiseB;
	int	nfound,hdunum;
	int	biassecan[4],biassecbn[4],
		ampsecan[4], ampsecbn[4],
		trimsecn[4], datasecn[4];
	fitsfile *fptr;
	float	*image,*varim,nullval;
	short 	*mask,shnullval;
	} desimage;

/* FITS Catalog structure */
typedef struct {
        char    name[200];
        long    axes[7],bscale,bzero,bitpix,npixels,fpixel;
        float   *image,*varim,nullval;
        short   *mask;
        int     nfound,hdunum;
        int     biassec0n[4],biassec1n[4],
                ampsec0n[4], ampsec1n[4],
                trimsecn[4], datasecn[4];
        long    nrows,*repeat,*width;
        int     ncols,*typecode,hdutype;

        } descat;


typedef struct {
        char    nite[20],band[10],psmversion[20],date[15],time[15],pm[5];
        float   mjdlo,mjdhi;
        int     fitid,ccdid,dof,photflag;
        float   a,aerr,b,berr,k,kerr,rms,chi2,color;
        } db_psmfit;

typedef struct {
        int     imageid,ccd_number,devid,ccdid;
        char    band[10],detector[20],telescope[20],imagename[80],
                runiddesc[80],nite[80];
        float   airmass,exptime,radeceq;
        double  ra,dec;
        } db_files;

typedef struct {
        int     zp_n,imageid,fitid,sourceid;
        float   mag_zero,sigma_zp,b,berr;
        } db_zeropoint;

typedef struct {
        char    telescope[20],detector[20];
        int     chipid;
        float   raoffset,decoffset,rahwidth,dechwidth;
        } db_wcsoffset;

typedef struct {
        char    project[25],tilename[50],runiddesc[300];
        char    nite[50],band[10],imagename[1024],basedir[1000];
        int     coaddtile_id,npix_ra,npix_dec,id,ccdnum,imageid;
        double  ra,dec;
        double  ra_lo,ra_hi,dec_lo,dec_hi;
        float   equinox,pixelsize;
        } db_tiles;



