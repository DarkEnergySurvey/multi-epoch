/* Carries out simple overscan subtraction and trimming of DC1-like data
*/
#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{

	int	status=0,hdunum,quiet;	
	void	printerror();
	int	i,hdutype;
	char	comment[200],imagename[500],longcomment[10000];
	float	min=0.1,max=3.0,biasmax=1000.0;
	desimage flat,bias,output;
	int	flag_quiet=0,flag_bias=0,
		flag_output=1,flag_list=0,imnum,imoutnum,im,
		flag_zero=0;
	void	rd_desimage();
	FILE	*inp,*out,*pip;

	if (argc<3) {
	  printf("mkbpm <flat image> <bpm> <options>\n");
	  printf("  -flatmax <#,3.0>\n");
	  printf("  -flatmin <#,0.1>\n");
	  printf("  -bias <image>\n");
	  printf("  -biasmax <#,1000>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	
	/* copy input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5))  {
	  sprintf(flat.name,"%s",argv[1]);
	  imnum=1;
	}
	
	/* prepare output image */
	if (!strncmp(&(argv[2][strlen(argv[2])-5]),".fits",5))
	 sprintf(output.name,"!%s",argv[2]);
	else {
	  printf("  ** FITS output image required **\n");
	  exit(0);
	}

	/* process command line */
	for (i=3;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-flatmin")) sscanf(argv[i+1],"%f",&min);
	  if (!strcmp(argv[i],"-flatmax")) sscanf(argv[i+1],"%f",&max);
	  if (!strcmp(argv[i],"-biasmax")) sscanf(argv[i+1],"%f",&biasmax);
	  if (!strcmp(argv[i],"-bias")) {
	    flag_bias=1;
	    if (!strncmp(&(argv[i+1][strlen(argv[i+1])-5]),".fits",5))
	      sprintf(bias.name,"%s",argv[i+1]);
	    else {
	      printf("  ** FITS bias image requires\n");
	      exit(0);
	    }
	  }
	}
	
	/* Echo the plan */
	if (!flag_quiet) printf("  Using normalized flat to identify bad pixels (value>%.2f and value<%.2f)\n",max,min);
	if (!flag_quiet && flag_bias) printf("  Also using bias image to identify hot pixels\n");
	

	/* **************************************************** */
	/* ********** PROCESSING SECTION BEGINS HERE ********** */
	/* **************************************************** */
	
	/* read the flat image */
	rd_desimage(&flat,READONLY,flag_quiet);
	/* check flat image */
        if (fits_movabs_hdu(flat.fptr,1,&hdutype,&status)) printerror(status);
	if (fits_read_keyword(flat.fptr,"DESFCMB",comment,comment,&status)==
	  KEY_NO_EXIST) {
	  printf("  ** FLAT image %s does not have DESFCMB keyword **\n",
	    flat.name);
	  exit(0);
	}

	/* prepare output image */
	output.npixels=flat.npixels;
	output.nfound=flat.nfound;
	for (i=0;i<output.nfound;i++) output.axes[i]=flat.axes[i];
	output.bitpix=USHORT_IMG;
	output.mask=(short *)calloc(output.npixels,sizeof(short));

	if (flag_bias) {
	  /* read the bias image */
	  rd_desimage(&bias,READONLY,flag_quiet);
	  /* check flat image */
          if (fits_movabs_hdu(bias.fptr,1,&hdutype,&status)) printerror(status);
	  if (fits_read_keyword(bias.fptr,"DESZCMB",comment,comment,&status)==
	    KEY_NO_EXIST) {
	    printf("  ** BIAS image %s does not have DESZCMB keyword **\n",
	      bias.name);
	    exit(0);
	  }
	}



	/* create the BPM */
        for (i=0;i<output.npixels;i++) {
	  if (flat.image[i]>max || flat.image[i]<min) output.mask[i]=1;
	  else output.mask[i]=0;
	  /* look for hot pixels, too */
	  if (flag_bias && bias.image[i]>biasmax) output.mask[i]=1;
	}	  

	/* *********************** */	
	/* **** SAVE RESULTS ***** */
	/* *********************** */
	/* create the file */
        if (fits_create_file(&output.fptr,output.name,&status)) 
	  printerror(status);
	  
	/* create image HDU */
	if (fits_create_img(output.fptr,USHORT_IMG,2,output.axes,&status))
	  printerror(status);
	
	/* write the corrected image*/
	if (fits_write_img(output.fptr,TUSHORT,1,output.npixels,output.mask,
	  &status))  printerror(status);
	    
	/* store information in the header */
	pip=popen("date","r");
	fgets(comment,200,pip);
	comment[strlen(comment)-1]=0;
	pclose(pip);
	if (fits_write_key_str(output.fptr,"DESBPM",comment,"bad pixel map created",&status)) printerror(status);
	sprintf(longcomment,"DESDM:");
	for (i=0;i<argc;i++) sprintf(longcomment,"%s %s",longcomment,argv[i]);
	if (!flag_quiet) printf("\n  %s\n", longcomment);
	if (fits_write_comment(output.fptr,longcomment,&status)) printerror(status);
        if (fits_write_key_str(output.fptr,"DES_EXT","MASK","Extension type",&status)) 
	  printerror(status);

        /* close the corrected image */
        if (fits_close_file(output.fptr,&status)) printerror(status);
	if (!flag_quiet) printf("  Closed %s: 2D ( %d X %d )\n",
	  &(output.name[flag_output]),output.axes[0],output.axes[1]);	  
	if (!flag_quiet) printf("\n\n");
}

