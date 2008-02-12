/* Carries out simple overscan subtraction and trimming of DC1-like data
*/
#include "localdefs.h"

#define VERBOSE 0

main(argc,argv)
	int argc;
	char *argv[];
{
	char	filename[200],
		comment[FLEN_COMMENT],
		trimsec[FLEN_VALUE],datasec[FLEN_VALUE],
		biassec0[FLEN_VALUE],ampsec0[FLEN_VALUE],
		biassec1[FLEN_VALUE],ampsec1[FLEN_VALUE];
	int	status=0,hdunum,quiet,x,y,nfound,imtype,
		length,xmin,xmax,anynull,
		biassec0n[4],biassec1n[4],
		ampsec0n[4],ampsec1n[4],
		trimsecn[4],datasecn[4];
	float	*vecsort0,*vecsort1;
	double	dbzero,dbscale;
	long	axes[2],naxes[2],pixels,npixels,bscale,bzero,bitpix,
		fpixel;
	unsigned long width;
	short	*data,*data1,*overscan0,*overscan1,nullval,value;
	void	printerror(),decodesection(),shell();
	fitsfile *fptr,*nfptr;

	if (argc<2) {printf("overscan <infile.fits> <quiet=0/1>\n");exit(0);}
	sprintf(filename,"%s",argv[1]);
	if (argc<3) quiet=0;
	else sscanf(argv[2],"%d",&quiet);


        /* open the file */
        if (fits_open_file(&fptr,filename,READWRITE,&status)) printerror(status);
	if (VERBOSE) printf("  Opened %s\n",filename);
	
        /* read the NAXIS1 and NAXIS2 keyword to get image size */
        if (fits_read_keys_lng(fptr,"NAXIS",1,2,axes,&nfound,&status))
          printerror(status);
	if (VERBOSE) printf("  %d X %d image\n",axes[0],axes[1]);

	/* read the data type, BSCALE and BZERO */
	if (fits_read_key_lng(fptr,"BITPIX",&bitpix,comment,&status))
	  printerror(status);
	if (VERBOSE) printf("  BITPIX   = %d\n",bitpix);

	if (fits_read_key_lng(fptr,"BSCALE",&bscale,comment,&status))
	  printerror(status);
	if (VERBOSE) printf("  BSCALE   = %d\n",bscale);
	
	if (fits_read_key_lng(fptr,"BZERO",&bzero,comment,&status))
	  printerror(status);
	if (VERBOSE) printf("  BZERO    = %d\n",bzero);
	
	if (bitpix==SHORT_IMG && bscale==1 && bzero==32768) {
	  imtype=USHORT_IMG;
	  if (!quiet) printf("  %s is a %d X %d image of type Unsigned Short\n",filename,
	    axes[0],axes[1]);
	}
	else {printf("Only set up to read unsigned short images\n");exit(0);}	  
	
	if (VERBOSE) printf(" Image %s opened successfully\n");
	
	/* get the BIASSEC information */
	if (fits_read_key_str(fptr,"BIASSEC0",biassec0,comment,&status))
	  printerror(status);
	if (!quiet) {
	  if (status==VALUE_UNDEFINED) printf("  Keyword BIASSEC0 not defined\n");
	  else printf("  BIASSEC0 for this exposure is %s\n",biassec0);	  
	}
	decodesection(biassec0,biassec0n);
	
	/* get the AMPSEC information */
	if (fits_read_key_str(fptr,"AMPSEC0",ampsec0,comment,&status))
	  printerror(status);
	if (!quiet) {
	  if (status==VALUE_UNDEFINED) printf("  Keyword AMPSEC0 not defined\n");
	  else printf("  AMPSEC0 for this exposure is %s\n",ampsec0);	  
	}
	decodesection(ampsec0,ampsec0n);
	
	/* get the BIASSEC information */
	if (fits_read_key_str(fptr,"BIASSEC1",biassec1,comment,&status))
	  printerror(status);
	if (!quiet) {
	  if (status==VALUE_UNDEFINED) printf("  Keyword BIASSEC1 not defined\n");
	  else printf("  BIASSEC1 for this exposure is %s\n",biassec1);	  
	}
	decodesection(biassec1,biassec1n);
	
	/* get the AMPSEC information */
	if (fits_read_key_str(fptr,"AMPSEC1",ampsec1,comment,&status))
	  printerror(status);
	if (!quiet) {
	  if (status==VALUE_UNDEFINED) printf("  Keyword AMPSEC1 not defined\n");
	  else printf("  AMPSEC1 for this exposure is %s\n",ampsec1);	  
	}
	decodesection(ampsec1,ampsec1n);

	/* get the TRIMSEC information */
	if (fits_read_key_str(fptr,"TRIMSEC",trimsec,comment,&status))
	  printerror(status);
	if (!quiet) {
	  if (status==VALUE_UNDEFINED) printf("  Keyword TRIMSEC not defined\n");
	  else printf("  TRIMSEC for this exposure is %s\n",trimsec);	  
	}
	decodesection(trimsec,trimsecn);

	/* get the DATASEC information */
	if (fits_read_key_str(fptr,"DATASEC",datasec,comment,&status))
	  printerror(status);
	if (!quiet) {
	  if (status==VALUE_UNDEFINED) printf("  Keyword DATASEC not defined\n");
	  else printf("  DATASEC for this exposure is %s\n",datasec);	  
	}
	decodesection(datasec,datasecn);
	/* get the number of HDUs in the image */
	if (fits_get_num_hdus(fptr,&hdunum,&status)) printerror(status);
	if (!quiet) printf("  %s has %d HDUs\n",filename,hdunum);
	if (hdunum>1) {printf("Currently not configured to work on MEFs\n");exit(0);}
		
        /* prepare to read the image */
        pixels  = axes[0]*axes[1];
        fpixel   = 1; nullval  = 0;
	/* prepare data vector */
        data=(short *)calloc(pixels,sizeof(short));
	/* read the CHDU image  */
        if (fits_read_img(fptr,USHORT_IMG,fpixel,pixels,&nullval,
          data,&anynull,&status)) printerror(status);
	  
	/* create overscan vectors for each amp */
	/* assume that the overscan is a set of columns of equal size on both sides of image */
	length=biassec0n[3]-biassec0n[2]+1;
	overscan0=(short *)calloc(length,sizeof(short));
	overscan1=(short *)calloc(length,sizeof(short));
	vecsort0=(float *)calloc(length,sizeof(float));
	vecsort1=(float *)calloc(length,sizeof(float));
	width=biassec0n[1]-biassec0n[0]+1;
	for (y=0;y<length;y++) {
	  /* copy overscan for current line into sorting vector */
	  for (x=0;x<width;x++) {
	    vecsort0[x]=data[y*axes[0]+x+biassec0n[0]-1];
	    vecsort1[x]=data[y*axes[0]+x+biassec1n[0]-1];
	  }
	  /* sort the vectors */
	  shell(width,vecsort0-1);
	  shell(width,vecsort1-1);
	  /* copy median into overscan vectors */
	  overscan0[y]=vecsort0[width/2];
	  overscan1[y]=vecsort1[width/2];
	  if (!quiet) printf("  %3d %4d %4d\n",y+biassec0n[2],overscan0[y],overscan1[y]);
	}

	/* create a single image array that fits the trimmed data */
	naxes[0]=1+trimsecn[1]-trimsecn[0];naxes[1]=1+trimsecn[3]-trimsecn[2];
	npixels=naxes[0]*naxes[1];
	data1=(short *)calloc(npixels,sizeof(short));
	
	/* copy the image into the new data array with overscan correction */
	xmin=ampsec0n[0]+datasecn[0]-1;
	xmax=ampsec0n[1]+datasecn[0]-1;
	for (y=trimsecn[2];y<=trimsecn[3];y++) for (x=trimsecn[0];x<=trimsecn[1];x++) {
	  if (x>=xmin && x<=xmax) /* then we are in AMPSEC0 */   
	    data1[(y-trimsecn[2])*naxes[0]+(x-trimsecn[0])]=
	      data[(y-1)*axes[0]+x-1]-overscan0[y-trimsecn[2]];
	  else /* assume we are in AMPSEC1 */ 
	    data1[(y-trimsecn[2])*naxes[0]+(x-trimsecn[0])]=
	      data[(y-1)*axes[0]+x-1]-overscan1[y-trimsecn[2]];
	  if (VERBOSE) {
	   value=data1[(y-trimsecn[2])*naxes[0]+(x-trimsecn[0])];
	   if (value<=SHRT_MIN || value>=SHRT_MAX) printf("%4d %4d %d\n", y,x,value);
	  }
	}
	  
	/* simply resize the image to the new, trimmed size*/
	if (fits_resize_img(fptr,SHORT_IMG,2,naxes,&status)) printerror(status);

	dbzero=0.0;dbscale=1.0;
	if (fits_set_bscale(fptr,dbscale,dbzero,&status)) printerror(status);
	bzero=0;
	if (fits_write_key_lng(fptr,"BZERO",bzero,comment,&status))
	  printerror(status);
		
	/* write the corrected image*/
	if (fits_write_img(fptr,TSHORT,1,npixels,data1,&status))
	  printerror(status);

        /* close the corrected image */
        if (fits_close_file(fptr,&status)) printerror(status);
	if (!quiet) printf("  Closed image %s\n",filename);
}

/* report Cfitsio errors */
void printerror(status)
        int status;
{
        fits_report_error(stderr,status);
        exit(0);
}

/* decode section string */
void decodesection(name,numbers)
	char name[];
	int numbers[];
{
	int i,len;
	
	len=strlen(name);
	for (i=0;i<len;i++) {
	  if (strncmp(&name[i],"0",1)<0 || strncmp(&name[i],"9",1)>0) name[i]=32; 
	} 
	sscanf(name,"%d %d %d %d",numbers,numbers+1,numbers+2,numbers+3);
	if (VERBOSE) {
	  printf("  Cleaned string is %s\n",name);
	  printf("    corresponding to %d %d %d %d\n",numbers[0],numbers[1],numbers[2],numbers[3]);
	}
}
