/* Simply reads USNO-B binary FITS tables from Fermilab and
   produces ascii tables for ingestion into the USNO B table
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
	double	*ra,*dec,*r2mag,doublenull=0.0;
	int	anynull;
	int	**pointer,**nullpointer;
	char	**prstring,tag[10];

	if (argc<3) {
	  printf("USNOB_convert <input catalog or file list> <output file> <options>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	
	/* copy input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-4]),".fit",4))  {
	  sprintf(data.name,"%s",argv[1]);
	  imnum=1;
	}
	else { /* expect file containing list of catalogs */
	  imnum=0;flag_list=1;
	  inp=fopen(argv[1],"r");
	  while (fscanf(inp,"%s",imagename)!=EOF) {
	    imnum++;
	    if (strncmp(&(imagename[strlen(imagename)-4]),".fit",4)) {
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
	  
	  ra=(double *)calloc(data.nrows,sizeof(double));
	  dec=(double *)calloc(data.nrows,sizeof(double));
	  r2mag=(double *)calloc(data.nrows,sizeof(double));
	  if (fits_read_col(data.fptr,TDOUBLE,2,1,1,data.nrows,
		&doublenull,ra,&anynull,&status)) printerror(status);
	  if (fits_read_col(data.fptr,TDOUBLE,3,1,1,data.nrows,
		&doublenull,dec,&anynull,&status)) printerror(status);
	  if (fits_read_col(data.fptr,TDOUBLE,33,1,1,data.nrows,
		&doublenull,r2mag,&anynull,&status)) printerror(status);
		
	  /* close file... */
          if (fits_close_file(data.fptr,&status)) printerror(status);
	  
	  for (j=0;j<5;j++) tag[j]=data.name[j];tag[5]=0;

	  for (j=0;j<data.nrows;j++) {
	    /*printf("  (%2d): ",j+1);*/
	    fprintf(out," %5s_%05d|%12.7f|%12.7f|2000.0|%7.4f",tag,j,ra[j],dec[j],100.0*r2mag[j]);
	    fprintf(out,"\n");
	 }

	  /* free memory allocated to data arrays */	 
	  free(r2mag);free(ra);free(dec);
	  if (!flag_quiet) printf("  Processing of file %d of %d complete\n\n",im+1,imnum);
	} /* end of image processing cycle with variable im */

	if (flag_list) fclose(inp);
	fclose(out);
	if (!flag_quiet) printf("  Closed ascii catalog file\n");
}

