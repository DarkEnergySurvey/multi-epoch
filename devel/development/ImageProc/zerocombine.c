/* Combine bias frames into a composite */
#include "imageproc.h"

#define SIGMACUT 4.0

main(argc,argv)
	int argc;
	char *argv[];
{

	int	status=0,i,im,num,x,y,loca,locb,delta,
		len,xmin,xmax,ymin,ymax,loc,hdutype;	
	long	ccdnum=0;
	void	printerror(),readimsections(),overscan();
	char	comment[1000],imagename[1000],command[1000],
		longcomment[10000],obstype[200],filter[100];
	unsigned long imnum;
	float	avsigclip_sigma,*vecsort,val,sigmalima,sigmalimb;
	double	sum2a,sum2b,sum1a,sum1b;
	/* define and initialize flags */
	int	flag_quiet=0,flag_combine=0,flag_variance=VARIANCE_DIRECT;
	void	rd_desimage(),shell(),decodesection(),headercheck();
	FILE	*inp=NULL,*pip=NULL;
	desimage *data=NULL,datain,output;
	fitsfile *fptr=NULL;

	if (argc<3) {
	  printf("zerocombine <input list> <output image> <options>\n");
	  printf("  -average\n");
	  printf("  -avsigclip <Sigma>\n");
	  printf("  -median (default)\n");
	  printf("  -novariance\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	
	/* copy input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5)  
	 || !strncmp(&(argv[1][strlen(argv[1])-8]),".fits.gz",8))  {
	  printf("  zerocombine requires an input image list\n");
	  exit(0);
	}
	else { /* expect file containing list of zero images */
	  imnum=0;
	  inp=fopen(argv[1],"r");
	  if (inp==NULL) {printf("  File %s not found\n",argv[1]);exit(0);}
	  while (fscanf(inp,"%s",imagename)!=EOF) {
	    imnum++;
	    if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5)
	     || !strncmp(&(imagename[strlen(imagename)-8]),".fits.gz",8)) {
	      printf("  ** File must contain list of FITS or compressed FITS images **\n");
	      exit(0);
	    }
	    else { /* open file and check header */
	      if (fits_open_file(&fptr,imagename,READONLY,&status)) 
		printerror(status);
	      /* confirm OBSTYPE */
	      if (fits_read_key_str(fptr,"OBSTYPE",obstype,comment,&status)==
	      KEY_NO_EXIST) {
	        printf("  OBSTYPE keyword not found in %s \n",imagename);
		exit(0);
	      }
	      if (strncmp(obstype,"zero",5) && strncmp(obstype,"bias",5) &&
		strncmp(obstype,"BIAS",5) && strncmp(obstype,"ZERO",5)) {
	        printf("  Expecting OBSTYPE 'zero' or 'bias' in %s\n",
		  imagename);
		exit(0);
	      }
	      if (fits_close_file(fptr,&status)) printerror(status);
	    }
	  }
	  fclose(inp);
	  /* reopen file for processing */
	  inp=fopen(argv[1],"r");
	}
	
	/* prepare output file */
	sprintf(output.name,"!%s",argv[2]);
	len=strlen(output.name)-1;
	for (i=len;i>=0;i--) {
	  if (!strncmp(&(output.name[i]),".fit",4)) break;
	}
	if (i<0) {
	  printf("  ** Output file must be FITS file\n");
	  exit(0);
	}

	/* set defaults */
	flag_combine=MEDIAN;
	/* process command line options */
	for (i=3;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-average")) flag_combine=AVERAGE;
	  if (!strcmp(argv[i],"-median"))  flag_combine=MEDIAN;
	  if (!strcmp(argv[i],"-avsigclip")) {
	    flag_combine=AVSIGCLIP;
	    sscanf(argv[i+1],"%f",&avsigclip_sigma);
	  }
	  if (!strcmp(argv[i],"-novariance"))  flag_variance=0;
	}
	
	if (!flag_quiet) printf("  Input list %s contains %d FITS images\n",argv[1],imnum);
	if (!flag_quiet) {
	  if (flag_combine==AVERAGE) printf("  Averaging images\n");
	  else if (flag_combine==MEDIAN) {
	    printf("  Median combining images\n");
	  }
	  else if (flag_combine==AVSIGCLIP) {
	    printf("  Average sigma clipping with +/-%.1f sigma\n",avsigclip_sigma);
	    printf("  Not yet implemented\n");exit(0);
	  }
	}

	/* create an array of image structures to hold the input images */
	data=(desimage *)calloc(imnum,sizeof(desimage));
	if (data==NULL) {
	  printf("  ** Calloc failed!\n");
	  exit(0);
	}
	if (flag_combine==MEDIAN) {
	  vecsort=(float *)calloc(imnum,sizeof(float));
	  if (vecsort==NULL) {
	    printf("  ** Calloc failed for vecsort\n");
	    exit(0);
	  }
	}
	
	/* now cycle through input images to read and prepare them */
	for (im=0;im<imnum;im++) {
	  /* get next image name */
	  fscanf(inp,"%s",datain.name);
	  if (!flag_quiet) printf("  Reading image %s\n",datain.name);
	  
	  /* read input image */
	  rd_desimage(&(datain),READONLY,flag_quiet);
	  /* copy the name into the current image */
	  sprintf(data[im].name,"%s",datain.name);
	  /* check image for ccdnumber */
	  headercheck(&datain,"NOCHECK",ccdnum,"NOCHECK");
	
	  /* retrieve basic image information from header */
	  /* first make sure we are at the first extension */
	  if (fits_movabs_hdu(datain.fptr,1,&hdutype,&status)) printerror(status);


          /* get the BIASSEC information */
          if (fits_read_key_str((datain.fptr),"BIASSECA",(datain.biasseca),comment,&status)
              ==KEY_NO_EXIST) {
            printf("  Keyword BIASSECA not defined in %s\n",datain.name);
            printerror(status);
          }
          if (!flag_quiet) printf("    BIASSECA = %s\n",(datain.biasseca));        
          decodesection((datain.biasseca),(datain.biassecan),flag_quiet);

         /* get the AMPSEC information */
          if (fits_read_key_str(datain.fptr,"AMPSECA",datain.ampseca,comment,&status)
                ==KEY_NO_EXIST) {
            printf("  Keyword AMPSECA not defined in %s\n",datain.name);
            printerror(status);
          }
          if (!flag_quiet) printf("    AMPSECA  = %s\n",datain.ampseca);
          decodesection(datain.ampseca,datain.ampsecan,flag_quiet);

          /* get the BIASSEC information */
          if (fits_read_key_str(datain.fptr,"BIASSECB",datain.biassecb,comment,&status)
                ==KEY_NO_EXIST) {
            printf("  Keyword BIASSECB not defined in %s\n",datain.name);
            printerror(status);
          }
          if (!flag_quiet) printf("    BIASSECB = %s\n",datain.biassecb);          
          decodesection(datain.biassecb,datain.biassecbn,flag_quiet);
          /* get the AMPSEC information */
          if (fits_read_key_str(datain.fptr,"AMPSECB",datain.ampsecb,comment,&status)
                ==KEY_NO_EXIST) {
            printf("  Keyword AMPSECB not defined in %s\n",datain.name);
            printerror(status);
          }
          if (!flag_quiet) printf("    AMPSECB  = %s\n",datain.ampsecb);
          decodesection(datain.ampsecb,datain.ampsecbn,flag_quiet);

          /* get the TRIMSEC information */
          if (fits_read_key_str(datain.fptr,"TRIMSEC",datain.trimsec,comment,&status)
                ==KEY_NO_EXIST) {
            printf("  Keyword TRIMSEC not defined in %s\n",datain.name);
            printerror(status);
          }
          if (!flag_quiet) printf("    TRIMSEC  = %s\n",datain.trimsec);
          decodesection(datain.trimsec,datain.trimsecn,flag_quiet);

          /* get the DATASEC information */
          if (fits_read_key_str(datain.fptr,"DATASEC",datain.datasec,comment,&status)
                ==KEY_NO_EXIST) {
            status=0;
              printf("  Keyword DATASEC not defined in %s\n",
                datain.name);
              printerror(status);
          }
          if (!flag_quiet) printf("    DATASEC  = %s\n",datain.datasec);
          decodesection(datain.datasec,datain.datasecn,flag_quiet);

	  /*readimsections(&(datain),flag_quiet);*/
	  
	  /* ***** OVERSCAN Section ****** */
	  /* check to see if DESOSCAN keyword is set */
	  if (fits_read_keyword(datain.fptr,"DESOSCN",comment,
	    comment,&status)==KEY_NO_EXIST) {
	    status=0;
	    if (!flag_quiet) printf("  Doing overscan correction");
	    overscan(&(datain),data+im,flag_quiet);
	    if (!flag_quiet) printf(".\n");
	  }
	  else {/* image has already been OVERSCAN corrected and trimmed */
	    /* simply copy the data into the output array  */
	    data[im].bitpix=datain.bitpix;
	    data[im].npixels=datain.npixels;
	    data[im].nfound=datain.nfound;
	    for (i=0;i<datain.nfound;i++) data[im].axes[i]=datain.axes[i];
	    data[im].image=(float *)calloc(data[im].npixels,sizeof(float));
	    if (data[im].image==NULL) {
	      printf("  ** Calloc failed for data[im].image\n");
	      exit(0);
	    }
	    for (i=0;i<data[im].npixels;i++) data[im].image[i]=datain.image[i];
	  }
		  
	  if (im==0) { /* prepare output image first time through*/
	    output.npixels=data[im].npixels;
	    output.nfound=data[im].nfound;
	    for (i=0;i<output.nfound;i++) output.axes[i]=data[im].axes[i];
	    output.bitpix=FLOAT_IMG;
	    output.image=(float *)calloc(output.npixels,sizeof(float));
	    if (output.image==NULL) {
	      printf("  ** Calloc failed for output.image\n");
	      exit(0);
	    }
	    if (flag_variance) {
	      output.varim=(float *)calloc(output.npixels,sizeof(float));
	      if (output.varim==NULL) {
	        printf("  ** Calloc failed for output.varim\n");
	        exit(0);
	      }
	    }
	  }
	  /* close input FITS file before moving on */
	  if (fits_close_file(datain.fptr,&status)) printerror(status);
	  free(datain.image);
	}  /* all the input images have been read and overscan subtracted */

	/* close input file list */	
	fclose(inp);
	if (!flag_quiet) printf("  Closed image list %s\n",argv[1]);

	
	if (!flag_quiet) printf("  Combining images \n");
	/* Combine the input images to create composite */
	if (flag_combine==AVERAGE) {
  	  for (i=0;i<output.npixels;i++) {
	    output.image[i]=0;
	    for (im=0;im<imnum;im++) output.image[i]+=data[im].image[i];
	    output.image[i]/=(float)imnum;
	  }
	}
	
	if (flag_combine==MEDIAN) {
  	  for (i=0;i<output.npixels;i++) { /* for each pixel */
	    /* copy values into sorting vector */
	    for (im=0;im<imnum;im++) vecsort[im]=data[im].image[i];
	    shell(imnum,vecsort-1);
	    /* odd number of images */
	    if (imnum%2) output.image[i]=vecsort[imnum/2]; /* record the median value */  
	    else output.image[i]=0.5*(vecsort[imnum/2]+vecsort[imnum/2-1]);
	  }
	}

	if (flag_combine==AVSIGCLIP) {
	  printf("  Not yet implemented\n"); exit(0);
	}
	
	
	
	
	/* ******************************************* */
	/* *********  VARIANCE SECTION *************** */
	/* ******************************************* */
	
	if (flag_variance) {
	  /* first estimate two variances to use for sigma clipping */
	  /* use hardwired central region of the chip */
	  sum2a=sum2b=0.0;num=0;
	  for (y=1500;y<2500;y++) for (x=400;x<600;x++) {
	    loca=y*output.axes[0]+x;
	    locb=y*output.axes[0]+x+1024;
	    for (im=0;im<imnum;im++) {
	      val=data[im].image[loca]-output.image[loca]; 
	      sum2a+=Squ(val);
	      val=data[im].image[locb]-output.image[locb]; 
	      sum2b+=Squ(val);
	      num++;
	    }
	  }
	  sigmalima=SIGMACUT*sqrt(sum2a/(float)num);
	  /* introduce lower limit for sigmalima */
	  if (sigmalima<1.0e-2) sigmalima=1.0e-2;
	  sigmalimb=SIGMACUT*sqrt(sum2b/(float)num);
	  /* introduce lower limit for sigmalimb */
	  if (sigmalimb<1.0e-2) sigmalimb=1.0e-2;
	  if (!flag_quiet) 
	    printf("  Estimated variance for Amp A/B is %.2f/%.2f\n",
	      sigmalima/SIGMACUT,sigmalimb/SIGMACUT);
	  delta=VARIANCE_DELTAPIXEL;
	  
	  
	  if (!flag_quiet) printf("  Now calculating pixel by pixel variance using %dX%d square centered on each pixel\n",
	    2*delta+1,2*delta+1);
	  for (i=0;i<output.npixels;i++) {
	    x=i%output.axes[0];y=i/output.axes[0];
	    /* define a small square centered on this pixel */
	    
	    xmin=x-delta;xmax=x+delta;
	    if (x<1024) { /* in AMP A */
	      if (xmin<0) {xmax+=abs(xmin);xmin=0;}
	      if (xmax>=1024) {xmin-=(xmax-1023);xmax=1023;}
	    }
	    else { /* in Amp B */	      
	      if (xmin<1024) {xmax+=(1024-xmin);xmin=1024;}
	      if (xmax>=output.axes[0]) {xmin-=(xmax-output.axes[0]+1);xmax=output.axes[0]-1;}    
	    }
	    
	    ymin=y-delta;ymax=y+delta;
	    /* hanging off bottom edge? */
	    if (ymin<0) {ymax+=abs(ymin);ymin=0;}
	    /* hanging off top edge? */
	    if (ymax>=output.axes[1])
	      {ymin-=(ymax-output.axes[1]+1);ymax=output.axes[1]-1;}

	    sum2a=0.0;num=0;	    
	    for (x=xmin;x<=xmax;x++) for (y=ymin;y<=ymax;y++) {
	    loc=x+y*output.axes[0];  
	      for (im=0;im<imnum;im++) {
	        val=fabs(data[im].image[loc]-output.image[loc]); 
	        if ((x<1024 && val<sigmalima) || (x>=1024 && val<sigmalimb)) {
	          sum2a+=Squ(val);
		  num++;
	        }
	      }
	    }
	    if (num>1) val=sum2a/(float)num/(float)imnum;
	    else {
	      printf("  *** Warning:  No pixels meet criterion for inclusion (%d,%d)\n",i%output.axes[0],i/output.axes[0]);
	      if (i%output.axes[0]<1024) val=Squ(sigmalima/SIGMACUT)/(float)imnum;
	      else val=Squ(sigmalimb/SIGMACUT)/(float)imnum; 
	    }
	    /* IMPOSE VARIANCE LOWER LIMIT */
	    if (val>1.0e-6) output.varim[i]=1.0/val;
	    else output.varim[i]=1.0e+6;
	  }
	}  /* end of flag_variance section */  
		
	/* **** SAVE RESULTS ***** */
	printf("  Writing results to %s\n",&(output.name[1]));fflush(stdout);
	/* create the file */
        if (fits_create_file(&output.fptr,output.name,&status)) 
	  printerror(status);

	/* create image extension */
	if (fits_create_img(output.fptr,FLOAT_IMG,2,output.axes,&status)) printerror(status);
	
	/* write the corrected image*/
	if (fits_write_img(output.fptr,TFLOAT,1,output.npixels,output.image,&status))
	  printerror(status);
	  
	/* write basic information into the header */
	if (fits_write_key_str(output.fptr,"OBSTYPE","zero",
	  "Observation type",&status)) printerror(status);  
	if (fits_write_key_lng(output.fptr,"CCDNUM",ccdnum,
	  "CCD Number",&status)) printerror(status);  

	/* Write information into the header describing the processing */
	pip=popen("date","r");
	fgets(comment,200,pip);
	comment[strlen(comment)-1]=0;
	pclose(pip);
	if (fits_write_key_str(output.fptr,"DESZCMB",comment,"zerocombine output",&status))
	  printerror(status);
	sprintf(longcomment,"DESDM:");
	for (i=0;i<argc;i++) sprintf(longcomment,"%s %s",longcomment,argv[i]);
	if (!flag_quiet) printf("\n  %s\n", longcomment);
	if (fits_write_comment(output.fptr,longcomment,&status)) printerror(status);
	sprintf(longcomment,"DESDM: (%d) ",imnum);
	for (im=0;im<imnum;im++) sprintf(longcomment,"%s %s ",longcomment,data[im].name);
	if (!flag_quiet) printf("  %s\n", longcomment);
	if (fits_write_comment(output.fptr,longcomment,&status)) printerror(status);
	if (fits_write_key_str(output.fptr,"DES_EXT","IMAGE","Image extension",
	  &status)) printerror(status);

	if (flag_variance) {
	  /* now store the variance image that has been created or updated */
	  /* first create a new extension */
	  if (fits_create_img(output.fptr,FLOAT_IMG,2,output.axes,&status))
	    printerror(status);
	  /* write the data */	  
	  if (fits_write_img(output.fptr,TFLOAT,1,output.npixels,output.varim,
	    &status)) printerror(status);
	  if (fits_write_key_str(output.fptr,"DES_EXT","VARIANCE","Extension type",&status)) printerror(status);
	}
			
        /* close the corrected image */
        if (fits_close_file(output.fptr,&status)) printerror(status);

	printf("\n");
	
	free(output.image);
	for (im=0;im<imnum;im++) free(data[im].image);

	return(0);
}
