/* read binary FITS scamp file and find median of fwhm */

#include "imageproc.h"

#define CLASSLIM 0.75	  /* class threshold to define star */
#define MAGERRLIMIT 0.1  /* mag error threshold for stars */


main (int argc, char *argv[])
{

	fitsfile *fptr, *bfptr;
	int	flag_quiet=0,status=0, hdunum, hdutype,fwhmcol,
		fwhmworldcol,ellipticitycol,
		flagscol, magerrautocol, classcol, anynull,ii,i,counter;
	long 	nrows,nullval_long=0,*flagsval;
	float 	*fwhmval, *fwhmshort,*magerrautoval,*classval,
		medianfwhmval=0.0, medianfwhmworldval=0.0,
		medianellipticityval=0.0,*fwhmworldval,*fwhmworldshort,
		*ellipticityval,*ellipticityshort,
		nullval_flt=0.0;
	char 	filename[500]; /* binary FITS catalog */
	char 	imagefile[500]; /* Image FITS file */
	//char 	nullstr[]="*";
	void 	shell(),printerror();

	/* see if there are the correct number of command line arguments */
	if(argc<3) {
	  printf("%s <Binary_Catalog> <Image_FITS_File>\n",argv[0]);
	  printf("  -quiet\n");
	  exit(1);
	} 
	sprintf(filename,"%s",argv[1]); /* name of Binary FITS file */
	sprintf(imagefile,"%s",argv[2]); /* name of Image FITS file */

	for (i=3;i<argc;i++)  
	  if (!strcmp("-quiet",argv[i])) flag_quiet=1;
  
	/* Open the binary FITS table for reading */

	if (fits_open_file(&fptr,filename,READONLY, &status)) 
	  printerror(status);
  
	/* move to the third HDU that contains the binary data */

	hdunum = 3;

	if (fits_movabs_hdu(fptr,hdunum,&hdutype,&status)) printerror(status);

	if (hdutype!=BINARY_TBL){
 	  printf("Error: this HDU is not a binary table\n");
          printerror(status);
	}

        /* determine column number for FWHM_IMAGE data  */
        if(fits_get_colnum(fptr,CASEINSEN,"FWHM_IMAGE",&fwhmcol,&status)) {
	  printf("Error: No FWHM_IMAGE column in binary table.\n");
          printerror(status);
	}

        /* determine column number for FWHM_WORLD data  */
/*
        if(fits_get_colnum(fptr,CASEINSEN,"FWHM_WORLD",&fwhmworldcol,&status)) {
	  printf("Error: No FWHM_WORLD column in binary table.\n");
          printerror(status);
	}
*/
        /* determine column number for ELLIPTICITY data  */
        if(fits_get_colnum(fptr,CASEINSEN,"ELLIPTICITY",&ellipticitycol,
	  &status)) {
	  printf("Error: No ELLIPTICITY column in binary table.\n");
          printerror(status);
	}

        /* determine column number for FLAGS data  */
        if(fits_get_colnum(fptr,CASEINSEN,"FLAGS",&flagscol,&status)) {
	  printf("Error: No FLAGS column in binary table.\n");
          printerror(status);
	}

        /* determine column number for MAGERR_AUTO data  */
        if(fits_get_colnum(fptr,CASEINSEN,"MAGERR_AUTO",&magerrautocol,
	   &status)) {
	   printf("Error: No MAGERR_AUTO column in binary table.\n");
           printerror(status);
	}

        /* determine column number for CLASS_STAR data  */
        if(fits_get_colnum(fptr,CASEINSEN,"CLASS_STAR",&classcol,&status)) {
	  printf("Error: No CLASS_STAR column in binary table.\n");
          printerror(status);
	}
     
        /* find the number of rows in the binary table  */
        if (fits_get_num_rows(fptr, &nrows, &status)) printerror(status);
	/* allocate memory for the data */
	fwhmval=(float *)calloc(nrows,sizeof(float));
	fwhmshort=(float *)calloc(nrows,sizeof(float));
/*
	fwhmworldval=(float *)calloc(nrows,sizeof(float));
	fwhmworldshort=(float *)calloc(nrows,sizeof(float));
*/
	ellipticityval=(float *)calloc(nrows,sizeof(float));
	ellipticityshort=(float *)calloc(nrows,sizeof(float));
	magerrautoval=(float *)calloc(nrows,sizeof(float));
	classval=(float *)calloc(nrows,sizeof(float));
	flagsval=(long *)calloc(nrows,sizeof(long));

        /* read FWHM_IMAGE, FWHM_WORLD, ELLIPTICITY, FLAGS, MAGERR_AUTO, */
	/* CLASS_STAR columns into arrays */
        if (fits_read_col(fptr,TFLOAT,fwhmcol,1,1,nrows,&nullval_flt,
	  fwhmval,&anynull,&status)) printerror(status);
/*
        if (fits_read_col(fptr,TFLOAT,fwhmworldcol,1,1,nrows,&nullval_flt,
	  fwhmworldval,&anynull,&status)) printerror(status);
*/
        if (fits_read_col(fptr,TFLOAT,ellipticitycol,1,1,nrows,&nullval_flt,
	  ellipticityval,&anynull,&status)) printerror(status);
        if (fits_read_col(fptr,TLONG,flagscol,1,1,nrows,
	  &nullval_long,flagsval,&anynull,&status)) printerror(status);
        if (fits_read_col(fptr,TFLOAT,magerrautocol,1,1,nrows,
	  &nullval_flt,magerrautoval,&anynull,&status)) printerror(status);
        if (fits_read_col(fptr,TFLOAT,classcol,1,1,nrows,
	  &nullval_flt,classval,&anynull,&status)) printerror(status);
        if (fits_close_file(fptr,&status)) printerror(status);
        

        /* make temp array for FWHM_IMAGE values for median function */
        /* Use data that have good FLAGS, CLASS_STAR, and MAG_AUTO values */  

	
        counter=0;
        for (ii=0; ii< nrows; ii++){
	   /*printf("%4d %.2f %2d %.4f = %.4f %.4f %.4f\n",ii,
	    classval[ii],flagsval[ii],magerrautoval[ii],
	    fwhmval[ii],fwhmworldval[ii],ellipticityval[ii]);*/
	  if ( flagsval[ii] < 1 && classval[ii] > CLASSLIM && 
	    magerrautoval[ii] < MAGERRLIMIT && fwhmval[ii]>0.5 
	    && ellipticityval[ii]>=0.0 ) {
	    fwhmshort[counter]=fwhmval[ii];
	    /*fwhmworldshort[counter]=fwhmworldval[ii];*/
	    ellipticityshort[counter]=ellipticityval[ii];
	    /*printf("***  %3d %.4f %.4f %.4f\n",ii,fwhmval[ii],fwhmworldval[ii],
	      ellipticityval[ii]);*/
	
	    counter=counter+1;
	  }
	}

	if (counter>1) {
          /* find median of values in the fwhmshort array */

          /* sort the array */
          shell(counter,fwhmshort-1);
          /* grab the median */
          if (counter%2) medianfwhmval=fwhmshort[counter/2];
          else medianfwhmval=0.5*(fwhmshort[counter/2]+fwhmshort[counter/2-1]);
          /* sort the array */
/*
          shell(counter,fwhmworldshort-1);
          /* grab the median */
/*
          if (counter%2) medianfwhmworldval=fwhmworldshort[counter/2];
          else medianfwhmworldval=0.5*(fwhmworldshort[counter/2]+
	    fwhmworldshort[counter/2-1]);
*/
          /* sort the array */
          shell(counter,ellipticityshort-1);
          /* grab the median */
          if (counter%2) medianellipticityval=ellipticityshort[counter/2];
          else medianellipticityval=0.5*(ellipticityshort[counter/2]+
	    ellipticityshort[counter/2-1]);

	}
	else {
	  if (counter==1) {
	    medianfwhmval=fwhmshort[counter];
/*
	    medianfwhmworldval=fwhmworldshort[counter];
*/
	    medianellipticityval=ellipticityshort[counter];
	  }
	}

	if (medianfwhmval<0.1) {
	  printf("** WARNING-- INCORRECT FWHM PARAMETER!!! **\n");
	  medianfwhmval=4.0;
	}

        /* open FITS image for adding median FWHM header keyword */
        if (fits_open_file(&bfptr,imagefile,READWRITE, &status)) 
	  printerror(status);
        if (fits_update_key(bfptr,TFLOAT,"FWHM",&medianfwhmval,
	  "Median FWHM in pixels",&status)) printerror(status);
        if (fits_update_key(bfptr,TFLOAT,"ELLIPTIC",&medianellipticityval,
	  "Median ELLIPTICITY",&status)) printerror(status);
        if (fits_close_file(bfptr,&status)) printerror(status);

	/* report results */
	if (!flag_quiet) {
	  printf("  Inserted FWHM=%.4f",medianfwhmval);
	  printf("  and ELLIPTIC=%.4f",medianellipticityval);
	  printf(" for %d stars in %s\n",counter,imagefile);
	}

}

#undef CLASSLIM
#undef MAGBRIGHT
#undef MAGFAINT
