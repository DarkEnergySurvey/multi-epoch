/* mkillumcor
/*
/* 	takes an input image (likely produced using sciencecombine)
/*	and produce a high signal to noise (i.e. smoothed) illumination
/*	correction and also, with appropriate flags, a fringe correction 
/*	image.  
/*
/*	
/*
*/



#include "imageproc.h"
#define  MAXPIXELS 500


main(argc,argv)
	int argc;
	char *argv[];
{
	char	filter[100],comment[1000],longcomment[10000];
	int	i,j,x,y,k,l,flag_fringe=NO,flag_illum=YES,flag_quiet=NO,
		minsize=4,maxsize=10,loc,dx,dy,xmin,xmax,ymin,ymax,
		flag_median=NO,flag_average=NO,status,count,
		nvec,xlen,ylen,totpix;
	long	ccdnum,ranseed=0;
	float	*vecsort,ran1(),subsample_fraction,*randnum;
	double	value;
	desimage input,illum,fringe;	
	FILE	 *pip;
	void	rd_desimage(),shell();

	if (argc<3) {
	  printf("%s <input file> <options>\n",argv[0]);
	  printf("  Smoothing Options\n");
	  printf("    -minsize <pixels>\n");
	  printf("    -maxsize <pixels>\n");
	  printf("    -median (default)\n");
	  printf("    -average\n");
	  printf("    -ranseed <#>\n");
	  printf("  Output Options\n");
	  printf("    -output_illum <file>\n");
	  printf("    -output_fringe <file>\n");
	  printf("    -quiet\n");
	  exit(0);
	}


	/* ******************************************* */
	/* ********  PROCESS COMMAND LINE ************ */
	/* ******************************************* */

	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=YES; 
	  if (!strcmp(argv[i],"-output_illum")) {
	    sprintf(illum.name,"!%s",argv[++i]);
	    flag_illum=YES;
	  }
	  if (!strcmp(argv[i],"-output_fringe")) {
	    sprintf(fringe.name,"!%s",argv[i+1]);
	    flag_fringe=YES;
	  }
	  if (!strcmp(argv[i],"-minsize")) 
	    sscanf(argv[++i],"%d",&minsize);
	  if (!strcmp(argv[i],"-maxsize")) 
	    sscanf(argv[++i],"%d",&maxsize);
	  if (!strcmp(argv[i],"-ranseed")) 
	    sscanf(argv[++i],"%ld",&ranseed);
	  if (!strcmp(argv[i],"-median")) flag_median=YES; 
	  if (!strcmp(argv[i],"-average")) flag_average=YES; 
	}	

	if (flag_median && flag_average) {
	  printf("  ** Must choose either median or combine\n");
	  exit(0); 
	}
	if (!flag_median && !flag_average) flag_median=YES;


	/* ********************************** */
	/* *******  READ INPUT IMAGE  ******* */
	/* ********************************** */

	sprintf(input.name,"%s",argv[1]);
	rd_desimage(&input,READONLY,flag_quiet);

	/* confirm that input image comes from sciencecombine */
        if (fits_read_keyword(input.fptr,"DESSCOMB",comment,
	  comment,&status)==KEY_NO_EXIST) {
	  printf("  ** Input image %s does not have DESSCOMB keyword **\n",
            input.name);
          /*exit(0);*/status=0;
        }

	/* read the FILTER information */
        if (fits_read_keyword(input.fptr,"FILTER",filter,
	  comment,&status)==KEY_NO_EXIST) {
	  printf("  ** Input image %s does not have FILTER keyword **\n",
            input.name);
          /*exit(0);*/status=0;
	}

	/* read the CCDNUM information */
        if (fits_read_key_lng(input.fptr,"CCDNUM",&ccdnum,comment,&status)
	  ==KEY_NO_EXIST) {
	  printf("  ** Input image %s does not have CCDNUM keyword **\n",
            input.name);
          /*exit(0);*/status=0;
	}

	/* **************************************************** */
	/* *******  SET UP TO CREATE ILLUMINATION IMAGE ******* */
	/* **************************************************** */

	if (!flag_quiet) printf("  Creating illumination correction");
	illum.npixels=input.npixels;
	illum.nfound=input.nfound;
	for (i=0;i<illum.nfound;i++) illum.axes[i]=input.axes[i];
	illum.bitpix=FLOAT_IMG;
	illum.image=(float *)calloc(illum.npixels,sizeof(float));

	/* set up for median sorting if needed */
	if (flag_median) {
	  count=Squ(2*maxsize);
	  vecsort=(float *)calloc(count,sizeof(float));
	  randnum=(float *)calloc(count,sizeof(float));
	  for (i=0;i<count;i++) randnum[i]=ran1(&ranseed);
	}

	/* smooth the input image to create the illumination correction */
	for (y=0;y<illum.axes[1];y++) {
	  if (y%200==1) {printf(".");fflush(stdout);}
	  dy=maxsize;
	  if (y-dy<0)  dy=y;
	  if (y+dy>=illum.axes[1])  dy=illum.axes[1]-y-1;
	  if (dy<minsize) dy=minsize;
	  ymin=y-dy;if (ymin<0) ymin=0;
	  ymax=y+dy;if (ymax>illum.axes[1]) ymax=illum.axes[1];
	  for (x=0;x<illum.axes[0];x++) {
	    dx=maxsize;
	    if (x-dx<0)  dx=x;
	    if (x+dx>=illum.axes[0])  dx=illum.axes[0]-x-1;
	    if (dx<minsize) dx=minsize;
	    xmin=x-dx;if (xmin<0) xmin=0;
	    xmax=x+dx;if (xmax>illum.axes[0]) xmax=illum.axes[0];
	    loc=x+y*illum.axes[0];
	    /* beginning of processing loop */
	    if (flag_average) {
	      count=0;value=0.0;
	      for (k=ymin;k<ymax;k++) for (l=xmin;l<xmax;l++) {
	        value+=input.image[l+k*input.axes[0]]; 
	        count++;
	      }
	      illum.image[loc]=value/(float)count;
	    }
	    else if (flag_median) {
	      count=0;
	      ylen=ymax-ymin; 
	      xlen=xmax-xmin; 
	      totpix=ylen*xlen;
	      /* use all the pixels */
	      if (!ranseed || ylen*xlen<MAXPIXELS) 
	        for (k=ymin;k<ymax;k++) for (l=xmin;l<xmax;l++) 
	          vecsort[count++]=input.image[l+k*input.axes[0]]; 
	      else /* randomly choose the pixels */
	        while (count<MAXPIXELS) {
		  k=ymin+(int)(totpix*randnum[count])/xlen;
		  if (k>=ymax) k=ymax-1;
		  l=xmin+(int)(totpix*randnum[count])%xlen;
		  if (l>=xmax) l=xmax-1;
		  vecsort[count++]=input.image[l+k*input.axes[0]]; 
	  	}
	      /* sort */
	      shell(count,vecsort-1);
	      /* odd or even number of pixels */
	      if (count%2) illum.image[loc]=vecsort[count/2];
	      else illum.image[loc]=0.5*(vecsort[count/2]+vecsort[count/2-1]);
	    }
	    /* end of processing loop */
	  }
	}
	printf("\n");

	/* **************************************************** */
	/* *********  SET UP TO CREATE  FRINGE IMAGE ********** */
	/* **************************************************** */

	if (flag_fringe) {
			
	  if (!flag_quiet) printf("  Creating fringe correction");
	  fringe.npixels=input.npixels;
	  fringe.nfound=input.nfound;
	  for (i=0;i<fringe.nfound;i++) fringe.axes[i]=input.axes[i];
	  fringe.bitpix=FLOAT_IMG;
	  fringe.image=(float *)calloc(fringe.npixels,sizeof(float));

	  for (i=0;i<fringe.npixels;i++) fringe.image[i]=input.image[i]-
	    illum.image[i];

	  printf("\n");
	}



	/* ******************************************** */
	/* ********** WRITE OUTPUT IMAGE(S) *********** */
	/* ******************************************** */

	if (flag_illum) {
	  printf("  Writing results to %s",illum.name+1);

	  /* create the file */
          if (fits_create_file(&illum.fptr,illum.name,&status)) 
	    printerror(status);

	  /* create image extension */
	  if (fits_create_img(illum.fptr,FLOAT_IMG,2,illum.axes,&status)) 
	    printerror(status);
  
	  /* write the corrected image*/
	  if (fits_write_img(illum.fptr,TFLOAT,1,illum.npixels,illum.image,
	    &status)) printerror(status);
	  
	  /* write basic information into the header */
	  if (fits_update_key_str(illum.fptr,"OBSTYPE","illumcor",
	    "Observation type",&status)) printerror(status);  
	  if (fits_update_key_str(illum.fptr,"FILTER",filter,
	    "Filter name(s)",&status)) printerror(status);
	  if (fits_update_key_lng(illum.fptr,"CCDNUM",ccdnum,
	    "CCD number",&status)) printerror(status);
	  /* Write information into the header describing the processing */
	  pip=popen("date","r");
	  fgets(comment,200,pip);
	  comment[strlen(comment)-1]=0;
	  pclose(pip);
	  if (fits_write_key_str(illum.fptr,"DESMKILL",comment,
	    "Created illumination correction",&status)) printerror(status);
	  sprintf(longcomment,"DESDM:");
	  for (i=0;i<argc;i++) sprintf(longcomment,"%s %s",longcomment,argv[i]);
	   if (!flag_quiet) printf("\n  %s\n", longcomment);
	   if (fits_write_comment(illum.fptr,longcomment,&status)) 
	     printerror(status);
	   if (fits_write_key_str(illum.fptr,"DES_EXT","IMAGE",
	    "Image extension",
	    &status)) printerror(status);

          /* close the corrected image */
          if (fits_close_file(illum.fptr,&status)) printerror(status);
	}


	if (flag_fringe) {
	  printf("  Writing results to %s",fringe.name+1);
	  /* create the file */
          if (fits_create_file(&fringe.fptr,fringe.name,&status)) 
	    printerror(status);

	  /* create image extension */
	  if (fits_create_img(fringe.fptr,FLOAT_IMG,2,fringe.axes,&status)) 
	    printerror(status);
  
	  /* write the corrected image*/
	  if (fits_write_img(fringe.fptr,TFLOAT,1,fringe.npixels,fringe.image,
	    &status)) printerror(status);
	  
	  /* write basic information into the header */
	  if (fits_write_key_str(fringe.fptr,"OBSTYPE","fringecor",
	    "Observation type",&status)) printerror(status);  
	  if (fits_update_key_str(illum.fptr,"FILTER",filter,
	    "Filter name(s)",&status)) printerror(status);
	  if (fits_update_key_lng(illum.fptr,"CCDNUM",ccdnum,
	    "CCD number",&status)) printerror(status);

	  /* Write information into the header describing the processing */
	  pip=popen("date","r");
	  fgets(comment,200,pip);
	  comment[strlen(comment)-1]=0;
	  pclose(pip);
	  if (fits_write_key_str(fringe.fptr,"DESMKFRG",comment,
	    "Created fringe correction",&status)) printerror(status);
	  sprintf(longcomment,"DESDM:");
	  for (i=0;i<argc;i++) sprintf(longcomment,"%s %s",longcomment,argv[i]);
	   if (!flag_quiet) printf("\n  %s\n", longcomment);
	   if (fits_write_comment(fringe.fptr,longcomment,&status)) 
	    printerror(status);
	   if (fits_write_key_str(fringe.fptr,"DES_EXT","IMAGE",
	    "Image extension",
	    &status)) printerror(status);

          /* close the fringe image */
          if (fits_close_file(fringe.fptr,&status)) printerror(status);
	}

	free(input.image);
	free(illum.image);
	if (flag_fringe) free(fringe.mask);

	printf("\n");

	return(0);
}

#undef  MAXPIXELS 
