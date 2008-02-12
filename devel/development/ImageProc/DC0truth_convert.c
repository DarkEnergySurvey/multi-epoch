/* Carries out simple overscan subtraction and trimming of DC1-like data
*/
#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{

	int	status=0;
	double	dbzero,dbscale;

	void	printerror();
	int	i;
	char	comment[FLEN_COMMENT];
	float	scale,offset;

	/* define and initialize flags */
	int	flag_quiet=0,flag_list=0,imnum=1,im;
	char	imagename[200];
	FILE	*inp,*out;
	descat	data;
	long	longi,j,k;
	long	longnull=0,*flags,*objid;
	float	*x,*y,*magauto,*magerrauto,*kronradius,*magaper5,
		*magaper10,*reddening,*fwhm,*classstar,*a,*b,*theta,
		*cxx,*cyy,*cxy,floatnull=0;
	double	*ra,*dec,doublenull=0.0;
	int	anynull;
	int	**pointer,**nullpointer;
	char	**prstring;

	if (argc<3) {
	  printf("DC0truth_convert <input catalog or file list> <output file> <options>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	
	/* copy input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5))  {
	  sprintf(data.name,"%s",argv[1]);
	  imnum=1;
	}
	else { /* expect file containing list of catalogs */
	  imnum=0;flag_list=1;
	  inp=fopen(argv[1],"r");
	  while (fscanf(inp,"%s",imagename)!=EOF) {
	    imnum++;
	    if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5)) {
	      printf("  ** File must contain list of FITS catalogs **\n");
	      exit(0);
	    }
	  }
	  fclose(inp);
	  if (!flag_quiet) printf("  Input list %s contains %d FITS catalogs\n",argv[1],imnum);
	  /* reopen file for processing */
	  inp=fopen(argv[1],"r");
	}
	
	/* process command line */
	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}
		
	if (!flag_quiet) printf("  Opening output catalog %s\n",argv[2]);
	/* open the output file */
	out=fopen(argv[2],"w");


	if (!flag_quiet) printf("\n");

	
	/* now cycle through input images to process them */
	for (im=0;im<imnum;im++) {

	  /* get next image name */
	  if (flag_list) fscanf(inp,"%s",data.name);
	
	  if (!flag_quiet) printf("  Opening %s  ",data.name);
	  /* open FITS catalog */
	  if (fits_open_file(&data.fptr,data.name,READONLY,&status))
	   printerror(status);
	 
	  if (fits_movrel_hdu(data.fptr,1,&data.hdutype,&status))
	    printerror(status);
	  if (data.hdutype!=BINARY_TBL) {
	    printf("  ** Expecting FITS Binary Table rather than %d\n",
	     data.hdutype);
	   exit(0);
	 }

	  /* get number of rows and columns */
	  if (fits_get_num_rows(data.fptr,&data.nrows,&status))
	    printerror(status);	
	  if (fits_get_num_cols(data.fptr,&data.ncols,&status))
	    printerror(status);  
	  if (!flag_quiet) printf("  %d X %d binary table\n",data.ncols,data.nrows);
	  
	  
	  if (im==0) {
	    data.repeat=(long *)calloc(data.ncols,sizeof(long));
	    data.width=(long *)calloc(data.ncols,sizeof(long));
	    data.typecode=(int *)calloc(data.ncols,sizeof(int));
	    pointer=(int *)calloc(data.ncols,sizeof(int *));
	    nullpointer=(int *)calloc(data.ncols,sizeof(int *));
	    prstring=(char **)calloc(data.ncols,sizeof(char *));
	  }
	  
	  for (i=0;i<data.ncols;i++)
	    prstring[i]=(char *)calloc(20,sizeof(char));
	  nullpointer[0]=nullpointer[19]=(long *)&longnull;
	  nullpointer[1]=nullpointer[2]=(float *)&floatnull;
	  nullpointer[3]=nullpointer[4]=(double *)&doublenull;
	  /*if (!flag_quiet) printf("  Col  #:  type   rept  wdth\n",i,data.typecode[i],
	      data.repeat[i],data.width[i]); */
	  for (i=0;i<data.ncols;i++) {
	    if (fits_get_coltype(data.fptr,i+1,&(data.typecode[i]),
	      &(data.repeat[i]),&(data.width[i]),&status)) 
	      printerror(status);
	    /*if (!flag_quiet) printf("  Col %2d: %4d  %4d  %4d\n",i+1,data.typecode[i],
	      data.repeat[i],data.width[i]); */ 
	  }

	  /* now prepare the data vectors for each column */
	  objid=(long *)calloc(data.nrows*5,sizeof(long));
	  pointer[0]=(long *)objid;
	  flags=(long *)calloc(data.nrows*5,sizeof(long));
	  pointer[19]=(long *)flags;
	  magauto=(float *)calloc(data.nrows*5,sizeof(float));
	  pointer[5]=(float *)magauto;
	  magerrauto=(float *)calloc(data.nrows*5,sizeof(float));
	  pointer[6]=(float *)magerrauto;
	  kronradius=(float *)calloc(data.nrows*5,sizeof(float));
	  pointer[7]=(float *)kronradius;
	  magaper5=(float *)calloc(data.nrows*5,sizeof(float));
	  pointer[8]=(float *)magaper5;
	  magaper10=(float *)calloc(data.nrows*5,sizeof(float));
	  pointer[9]=(float *)magaper10;
	  reddening=(float *)calloc(data.nrows*5,sizeof(float));
	  pointer[10]=(float *)reddening;
	  fwhm=(float *)calloc(data.nrows*5,sizeof(float));
	  pointer[11]=(float *)fwhm;
	  classstar=(float *)calloc(data.nrows*5,sizeof(float));
	  pointer[12]=(float *)classstar;
	  
	  x=(float *)calloc(data.nrows,sizeof(float));
	  pointer[1]=(float *)x;
	  y=(float *)calloc(data.nrows,sizeof(float));
	  pointer[2]=(float *)y;
	  a=(float *)calloc(data.nrows,sizeof(float));
	  pointer[13]=(float *)a;
	  b=(float *)calloc(data.nrows,sizeof(float));
	  pointer[14]=(float *)b;
	  theta=(float *)calloc(data.nrows,sizeof(float));
	  pointer[15]=(float *)theta;
	  cxx=(float *)calloc(data.nrows,sizeof(float));
	  pointer[16]=(float *)cxx;
	  cyy=(float *)calloc(data.nrows,sizeof(float));
	  pointer[17]=(float *)cyy;
	  cxy=(float *)calloc(data.nrows,sizeof(float));
	  pointer[18]=(float *)cxy;
	  ra=(double *)calloc(data.nrows,sizeof(double));
	  pointer[3]=(double *)ra;
	  dec=(double *)calloc(data.nrows,sizeof(double));
	  pointer[4]=(double *)dec;

	  /* this looks good to me */
	  for (i=0;i<data.ncols;i++) {
	      if (fits_read_col(data.fptr,data.typecode[i],
	        i+1,1,1,data.nrows*data.repeat[i],nullpointer[i],
		pointer[i],&anynull,&status)) printerror(status);
	      
	  }
	  /* close file... */
          if (fits_close_file(data.fptr,&status)) printerror(status);


	  for (j=0;j<data.nrows;j++) {
	    /*printf("  (%2d): ",j+1);*/
	    for (k=0;k<data.repeat[0];k++) 
	      fprintf(out," %6d,",objid[j*data.repeat[0]+k]);
	    fprintf(out,"  %7.2f, %7.2f, %9.5f, %10.5f, 2000.0,",x[j],y[j],ra[j],dec[j]);
	    /*printf("\n");*/
	    for (k=0;k<data.repeat[0];k++) fprintf(out," %7.4f, %7.4f,",
	      magauto[j*5+k],magerrauto[j*5+k]);
	    /*printf("\n");*/
	    /*for (k=0;k<data.repeat[0];k++) fprintf(out," %5.2f,",kronradius[j*5+k]);
	    /*printf("\n");*/
	    for (k=0;k<data.repeat[0];k++) fprintf(out," %7.4f,",magaper5[j*5+k]);
	    for (k=0;k<data.repeat[0];k++) fprintf(out," %7.4f,",magaper10[j*5+k]);
	    for (k=0;k<data.repeat[0];k++) fprintf(out," %7.4f,",reddening[j*5+k]);
	    /*printf("\n");*/
	    for (k=0;k<data.repeat[0];k++) fprintf(out," %6.2f,",fwhm[j*5+k]);
	    for (k=0;k<data.repeat[0];k++) fprintf(out," %4.2f,",classstar[j*5+k]);
	    /*printf("\n");*/
	    fprintf(out,"  %7.2f, %7.2f, %7.2e, %10.3e, %10.3e, %10.3e",
	      a[j],b[j],theta[j],cxx[j],cyy[j],cxy[j]);
	    for (k=0;k<data.repeat[0];k++) fprintf(out,", %3d",flags[j*5+k]);
	    fprintf(out,"\n");
	 }

	  /* free memory allocated to data arrays */	 
	  free(objid);free(flags);free(x);free(y);
	  free(magauto);free(magerrauto);free(kronradius);free(magaper5);
	  free(magaper10);free(reddening);free(fwhm);free(classstar);free(a);
	  free(b);free(theta);free(cxx);free(cyy);free(cxy);free(ra);free(dec);
	  if (!flag_quiet) printf("  Processing of file %d of %d complete\n\n",im+1,imnum);
	} /* end of image processing cycle with variable im */

	if (flag_list) fclose(inp);
	fclose(out);
	if (!flag_quiet) printf("  Closed ascii catalog file\n");
}

