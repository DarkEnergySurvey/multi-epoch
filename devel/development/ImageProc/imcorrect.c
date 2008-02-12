
/* Carries out basic image processing of CCD data */

#include "imageproc.h"

#define BADPIX_BPM 1   	     /* set in bad pixel mask (hot/dead pixel/column) */
#define BADPIX_SATURATE 2    /* saturated pixel */
#define BADPIX_INTERP 4
#define BADPIX_THRESHOLD 0.25/* pixels less than this fraction of sky     */
			     /* are filtered -- helps remove failed reads */
#define BADPIX_MASK   8      /* too little signal- i.e. poor read */

main(argc,argv)
	int argc;
	char *argv[];
{

	int	status=0,hdunum,quiet,x,y,nfound,imtype,
		xmin,xmax,anynull,maskedpixels,interppixels,
		saturatepixels;	
	double	dbzero,dbscale,sumval;
	long	axes[2],naxes[2],pixels,npixels,bscale,bzero,bitpix,
		fpixel,ccdnum=0;
	int	i,keysexist,j,n,nkeys,scaleregionn[4]={500,1500,1500,2500},
		scalenum;
	char	comment[1000],imagename[1000],longcomment[10000],filter[100]="",
		bpmname[1000],rootname[1000],varimname[1000],
		keycard[100],keyname[10],scaleregion[100];
	/* list of keywords to be removed from the header after imcorrect */
	char	delkeys[100][10]={"CCDSUM","AMPSECA","AMPSECB","TRIMSEC",
		"BIASSECA","BIASSECB","GAINA","GAINB","RDNOISEA",
		"RDNOISEB","SATURATA","SATURATB",
		""};
	float	scale,offset,gasdev(),scale_interpolate,maxsaturate,
		overscanA=0.0,overscanB=0.0,scalefactor,mode,fwhm,
		*scalesort,skybrite,thresholdval;
	static 	desimage bias,flat,data,output,bpm,illumination,fringe,
		pupil;
	/* define and initialize flags */
	int	flag_overscan=YES,flag_bias=NO,flag_flatten=NO,flag_quiet=NO,
		flag_output=NO,flag_list=NO,flag_bpm=0,flag_variance=NO,
		imnum,imoutnum,im,hdutype,flag_interpolate_col=NO,
		flag_illumination=NO,flag_fringe=NO,
		flag_pupil=NO,flag_impupil=NO,
		flag_imoverscan,flag_imbias,flag_imflatten,
		flag_imillumination,flag_imfringe,flag_updateskybrite=NO,
		flag_updatemnseeing=NO,
		seed=-15,xlow,xhi,flag_mef=NO;
	void	rd_desimage(),headercheck(),printerror(),decodesection(),
		readimsections(),overscan(),retrievescale();
	FILE	*inp,*out,*pip;

	if (argc<3) {
	  printf("%s <image or list file> <options>\n",argv[0]);
	  printf("  Input Data\n");
	  printf("    -bpm <image>\n");
	  printf("  Image Corrections\n");
	  printf("    -nooverscan\n");
	  printf("    -bias <image>\n");
	  printf("    -pupil <image>\n");
	  printf("    -flatten <image>\n");
	  printf("    -interpolate_col <variance scaling>\n");
	  printf("    -illumination <image>\n");
	  printf("    -fringe <image>\n");
	  printf("    -scaleregion <Xmin:Xmax,Ymin,Ymax>\n");
	  printf("  Output Options\n");
	  printf("    -output <image or list file>\n");
	  printf("    -variance\n");
	  printf("    -MEF (single file output)\n");
	  printf("    -updatekeyword SKYBRITE or MNSEEING\n");
	  printf("    -quiet\n");
	  exit(0);
	}

		
	/* ********************************************** */
	/* ********* Handle Input Image/File  *********** */
	/* ********************************************** */

	/* copy input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5) ||
	  !strncmp(&(argv[1][strlen(argv[1])-8]),".fits.gz",8))  {
	  sprintf(data.name,"%s",argv[1]);
	  imnum=1;
	}
	else { /* expect file containing list of images */
	  imnum=0;flag_list=1;
	  inp=fopen(argv[1],"r");
	  if (inp==NULL) {
	    printf("  ** File %s not found\n",argv[1]);
	    exit(0);
	  }
	  while (fscanf(inp,"%s",imagename)!=EOF) {
	    imnum++;
	    if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5)
	      && strncmp(&(imagename[strlen(imagename)-8]),".fits.gz",8)) {
	      printf("  ** File must contain list of FITS or compressed FITS images **\n");
	      exit(0);
	    }
	  }
	  fclose(inp);
	  /* reopen file for processing */
	  inp=fopen(argv[1],"r");
	}



	/* ****************************************** */
	/* ********* Process Command Line *********** */
	/* ****************************************** */

	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-nooverscan")) flag_overscan=NO;
	  if (!strcmp(argv[i],"-variance")) flag_variance=YES;
	  if (!strcmp(argv[i],"-MEF")) flag_mef=YES;
          if (!strcmp(argv[i],"-scaleregion")) {
            sprintf(scaleregion,"%s",argv[i+1]);
            decodesection(scaleregion,scaleregionn,flag_quiet);
          }
	  if (!strcmp(argv[i],"-interpolate_col")) {
	    flag_interpolate_col=YES;
	    i++;
	    sscanf(argv[i],"%f",&scale_interpolate);
	    if (scale_interpolate<1.0 || scale_interpolate>10.0) {
		printf("  ** Variance scale factor for interpolated pixels must be between 1 and 10 (%f)\n",
		  scale_interpolate);
	    }
	  }
	  if (!strcmp(argv[i],"-bias")) {
	    flag_bias=YES;
	    i++;
	    if (strncmp(&(argv[i][strlen(argv[i])-5]),".fits",5) &&
	      strncmp(&(argv[i][strlen(argv[i])-8]),".fits.gz",8)) {
	      printf("  ** FITS Image must follow -bias option **\n");
	      exit(0);
	    }	    
	    else sprintf(bias.name,"%s",argv[i]);
	  }
	  if (!strcmp(argv[i],"-pupil")) {
	    flag_pupil=YES;
	    i++;
	    if (strncmp(&(argv[i][strlen(argv[i])-5]),".fits",5) &&
	      strncmp(&(argv[i][strlen(argv[i])-8]),".fits.gz",8)) {
	      printf("  ** FITS Image must follow -pupil option **\n");
	      exit(0);
	    }	    
	    else sprintf(pupil.name,"%s",argv[i]);
	  }
	  if (!strcmp(argv[i],"-bpm")) {
	    flag_bpm=YES;
	    i++;
	    if (strncmp(&(argv[i][strlen(argv[i])-5]),".fits",5) && 
	      strncmp(&(argv[i][strlen(argv[i])-8]),".fits.gz",8)) {
	      printf("  ** FITS Image must follow -bpm option **\n");
	      exit(0);
	    }	    
	    else sprintf(bpm.name,"%s",argv[i]);
	  }
	  if (!strcmp(argv[i],"-flatten")) {
	    flag_flatten=YES;
	    i++;
	    if (strncmp(&(argv[i][strlen(argv[i])-5]),".fits",5)
	      && strncmp(&(argv[i][strlen(argv[i])-8]),".fits.gz",8)) {
	      printf("  ** FITS Image must follow -flatten option **\n");
	      exit(0);
	    }
	    else sprintf(flat.name,"%s",argv[i]);
	  }
	  if (!strcmp(argv[i],"-output")) {
	    flag_output=YES;
	    i++;
	    if (!strncmp(&(argv[i][strlen(argv[i])-5]),".fits",5)
	      || !strncmp(&(argv[i][strlen(argv[i])-8]),".fits.gz",8))
	      if (!flag_list) sprintf(output.name,"!%s",argv[i]);
	      else {
		printf("  ** FITS image output name must follow -output option in single image mode **\n");
		exit(0);
	    }
	    else {
	      if (!flag_list) {
		printf("  ** ASCII file list must follow -output option in multi image mode **\n");
		exit(0);
	      }
	      imoutnum=0;
	      out=fopen(argv[i],"r");
	      if (out==NULL) {
	        printf("  ** File %s not found\n",argv[i]);
	        exit(0);
	      }
	      while (fscanf(out,"%s",imagename)!=EOF) {
		imoutnum++;
		if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5) &&
		  strncmp(&(imagename[strlen(imagename)-8]),".fits.gz",8)) {
		  printf("  ** File must contain list of FITS or compressed FITS images **\n");
		  exit(0);
		}
	      }
	      fclose(out);
	      if (imoutnum==imnum) printf("  Output list %s of same length\n",
		argv[i]);
	      else {
		printf("  ** Input and output lists must be of same length in multi image mode\n");
		exit(0);
	      }
	      /* reopen output list for processing */
	      out=fopen(argv[i],"r");
	    }
	  }
	  if (!strcmp(argv[i],"-illumination")) {
	    flag_illumination=YES;
	    i++;
	    if (strncmp(&(argv[i][strlen(argv[i])-5]),".fits",5) &&
	      strncmp(&(argv[i][strlen(argv[i])-8]),".fits.gz",8)) {
	      printf("  ** FITS Image must follow -illumination option **\n");
	      exit(0);
	    }
	    else sprintf(illumination.name,"%s",argv[i]);
	  }
	  if (!strcmp(argv[i],"-fringe")) {
	    flag_fringe=YES;
	    i++;
	    if (strncmp(&(argv[i][strlen(argv[i])-5]),".fits",5) &&
	      strncmp(&(argv[i][strlen(argv[i])-8]),".fits.gz",8)) {
	      printf("  ** FITS Image must follow -fringe option **\n");
	      exit(0);
	    }
	    else sprintf(fringe.name,"%s",argv[i]);
	  }
	  if (!strcmp(argv[i],"-updatekeyword")) {
	    i++;
	    if (!strncmp(argv[i],"SKYBRITE",8)) flag_updateskybrite=YES;
	    else if (!strncmp(argv[i],"MNSEEING",8)) flag_updatemnseeing=YES;
	    else {
	      printf("***** -updatekeyword %s not understood\n",argv[i]);
	      exit(0);
	    }
	  }
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=YES;
	}
	
	if (!flag_quiet) 
	  printf("  Input list %s contains %d FITS images\n",argv[1],imnum);


	/* set up scaling vectors */
	scalenum=(scaleregionn[1]-scaleregionn[0])*
	  (scaleregionn[3]-scaleregionn[2]);
	scalesort=(float *)calloc(scalenum,sizeof(float));



	/* ****************************************** */
	/* ********** READ CALIBRATION DATA ********* */
	/* ****************************************** */
			
	/* read bias image */
	if (flag_bias) {
	  if (!flag_quiet) printf("  Reading BIAS image %s\n",bias.name);
	  rd_desimage(&bias,READONLY,flag_quiet);
	  /* confirm that this is actual bias image */
	  /* make sure we are in the 1st extension */
	  if (fits_movabs_hdu(bias.fptr,1,&hdutype,&status)) printerror(status);
	  /* confirm that this is bias image */
	  headercheck(&bias,"NOCHECK",ccdnum,"DESZCMB");
	  if (fits_close_file(bias.fptr,&status)) printerror(status);
	}
	
	/* read flat image */	
	if (flag_flatten) {
	  if (!flag_quiet) printf("  Reading FLATFIELD image %s\n",flat.name);
	  rd_desimage(&flat,READONLY,flag_quiet);
	  /* make sure we are in the 1st extension */
	  if (fits_movabs_hdu(flat.fptr,1,&hdutype,&status)) printerror(status);
	  /* confirm that this is flat image */
	  headercheck(&flat,filter,ccdnum,"DESFCMB");
	  if (fits_close_file(flat.fptr,&status)) printerror(status);
	}
	/* read pupil image */
	if (flag_pupil) {
	  if (!flag_quiet) printf("  Reading pupil ghost image %s\n",
	    pupil.name);
	  rd_desimage(&pupil,READONLY,flag_quiet);
	  /* confirm that this is bpm image */
	  headercheck(&pupil,"NOCHECK",ccdnum,"DESPUPL");
	  if (fits_close_file(pupil.fptr,&status)) printerror(status);
	}
	/* read bad pixel mask image */
	if (flag_bpm) {
	  if (!flag_quiet) printf("  Reading BPM image %s\n",bpm.name);
	  rd_desimage(&bpm,READONLY,flag_quiet);
	  /* confirm that this is bpm image */
	  headercheck(&bpm,"NOCHECK",ccdnum,"DESBPM");
	  if (fits_close_file(bpm.fptr,&status)) printerror(status);
	}
	/* create a mock bpm regardless */
	else bpm=bias;	
		
	/* read illumination image */	
	if (flag_illumination) {
	  if (!flag_quiet) 
	    printf("  Reading ILLUMINATION CORRECTION image %s\n",
	      illumination.name);
	  rd_desimage(&illumination,READONLY,flag_quiet);
	  /* make sure we are in the 1st extension */
	  if (fits_movabs_hdu(illumination.fptr,1,&hdutype,&status)) 
	    printerror(status);
	  /* confirm that this is illumination image */
	  headercheck(&illumination,filter,ccdnum,"DESMKILL");
	  if (fits_close_file(illumination.fptr,&status)) printerror(status);
	}
		
	/* read fringe image */	
	if (flag_fringe) {
	  if (!flag_quiet) 
	    printf("  Reading FRINGE CORRECTION image %s\n",
	      fringe.name);
	  rd_desimage(&fringe,READONLY,flag_quiet);
	  /* make sure we are in the 1st extension */
	  if (fits_movabs_hdu(fringe.fptr,1,&hdutype,&status)) 
	    printerror(status);
	  /* confirm that this is fringe image */
	  headercheck(&fringe,filter,ccdnum,"DESMKFRG");
	  if (fits_close_file(fringe.fptr,&status)) printerror(status);
	}
		
		
	/********************************************/
	/****** PROCESSING SECTION BEGINS HERE ******/
	/********************************************/
	
	/* now cycle through input images to process them */
	for (im=0;im<imnum;im++) {
	  /* get next image name */
	  if (flag_list) fscanf(inp,"%s",data.name);

	  /* ************************************** */
	  /* ***** Read Input Image Section  ****** */
	  /* ************************************** */

	  if (!flag_quiet) printf("  Reading INPUT image %s\n",data.name);
	  if (flag_output)
	    rd_desimage(&data,READONLY,flag_quiet);  
	  else {
	    rd_desimage(&data,READWRITE,flag_quiet);
	    /* prepare output image */
	    output=data;	
	  }

	  /* get next output image name */
	  if (flag_list) {
	    fscanf(out,"%s",imagename);
	    sprintf(output.name,"!%s",imagename);
	  }
	  else sprintf(imagename,"%s",output.name);

	  /* prepare output image */
	  output.npixels=data.npixels;
	  output.nfound=data.nfound;
	  for (i=0;i<output.nfound;i++) output.axes[i]=data.axes[i];
	  output.bitpix=FLOAT_IMG;
	  output.saturateA=data.saturateA;output.saturateB=data.saturateB;
	  output.gainA=data.gainA;output.gainB=data.gainB;
	  output.rdnoiseA=data.rdnoiseA;output.rdnoiseB=data.rdnoiseB;
	  if (output.saturateA>output.saturateB) maxsaturate=output.saturateA;
	  else maxsaturate=output.saturateB;

		  
	  /* ************************************** */
	  /* ***** READING IMAGE SECTIONS    ****** */
	  /* ************************************** */
	  
	  /* retrieve basic image information from header */
	  if (fits_movabs_hdu(data.fptr,1,&hdutype,&status)) 
	    printerror(status);

	  /* confirm correct filter and ccdnum */
	  headercheck(&data,filter,ccdnum,"NOCHECK");


	  /* get the DATASEC information */
	  if (fits_read_key_str(data.fptr,"DATASEC",data.datasec,
	    comment,&status) ==KEY_NO_EXIST) {
	    printf("  Keyword DATASEC not defined in %s\n",
		data.name);
	    printerror(status);
          }
          if (!flag_quiet) printf("    DATASEC  = %s\n",data.datasec);
          decodesection(data.datasec,data.datasecn,flag_quiet);
  
	  

	  /* ************************************** */
	  /* ***** OVERSCAN and TRIM Section ****** */
	  /* ************************************** */

	  if (flag_overscan) {
	    if (fits_read_keyword(data.fptr,"DESOSCN",comment,comment,&status)==
	      KEY_NO_EXIST) {
	      flag_imoverscan=1;
	      status=0; /* reset flag */
	      if (!flag_quiet) printf("  OVERSCAN correcting\n");

	      /* get the required header information */
	      /* get the BIASSEC information */          
	      if (fits_read_key_str((data.fptr),"BIASSECA",(data.biasseca),
	          comment,&status)==KEY_NO_EXIST) {
	        printf("  Keyword BIASSECA not defined in %s\n",data.name);
	        printerror(status);
	      }
	      if (!flag_quiet) printf("    BIASSECA = %s\n",(data.biasseca));
	      decodesection((data.biasseca),(data.biassecan),flag_quiet);
    
	      /* get the BIASSEC information */
	      if (fits_read_key_str(data.fptr,"BIASSECB",data.biassecb,
	        comment,&status) ==KEY_NO_EXIST) {
	        printf("  Keyword BIASSECB not defined in %s\n",data.name);
	        printerror(status);
	      }
	      if (!flag_quiet) printf("    BIASSECB = %s\n",data.biassecb);
	      decodesection(data.biassecb,data.biassecbn,flag_quiet);

	     /* get the AMPSEC information */
	      if (fits_read_key_str(data.fptr,"AMPSECA",data.ampseca,
	        comment,&status)
		    ==KEY_NO_EXIST) {
	        printf("  Keyword AMPSECA not defined in %s\n",data.name);
	        printerror(status);
	      }
	      if (!flag_quiet) printf("    AMPSECA  = %s\n",data.ampseca);
	      decodesection(data.ampseca,data.ampsecan,flag_quiet);

	      /* get the AMPSEC information */
	      if (fits_read_key_str(data.fptr,"AMPSECB",data.ampsecb,
	        comment,&status) ==KEY_NO_EXIST) {
	        printf("  Keyword AMPSECB not defined in %s\n",data.name);
	        printerror(status);
	      }
	      if (!flag_quiet) printf("    AMPSECB  = %s\n",data.ampsecb);
	      decodesection(data.ampsecb,data.ampsecbn,flag_quiet);
    
	      /* get the TRIMSEC information */
	      if (fits_read_key_str(data.fptr,"TRIMSEC",data.trimsec,
	        comment,&status) ==KEY_NO_EXIST) {
	        printf("  Keyword TRIMSEC not defined in %s\n",data.name);
	        printerror(status);
	      }
	      if (!flag_quiet) printf("    TRIMSEC  = %s\n",data.trimsec);
	      decodesection(data.trimsec,data.trimsecn,flag_quiet);

	      /* carry out overscan */

	      overscan(&data,&output,flag_quiet);
	      status=0;
	      /* update dataset within the output image */
	      sprintf(output.datasec,"[%d:%d,%d:%d]",1,output.axes[0],
	 	1,output.axes[1]);
	    }
	    else {
	      if (!flag_quiet) 
	        printf("  Image %s previously OVERSCAN corrected\n",data.name);
	         flag_imoverscan=0; 
	    }
	  }
	  else {
	    /* simply copy the data into the output array  */
	    /* note that this assumes input image has already been
	    overscan/trimmed*/
	    /* need to make this robust-- check for trimming */
	    if (!flag_quiet) printf("  Copying image array");
	    output.image=(float *)calloc(output.npixels,sizeof(float));
	    for (i=0;i<output.npixels;i++) output.image[i]=data.image[i];
	    if (data.mask!=NULL) {
	      if (!flag_quiet) printf(" and mask array");
	      output.mask=(short *)calloc(output.npixels,sizeof(short));
	      for (i=0;i<output.npixels;i++) output.mask[i]=data.mask[i];
	    }
	    if (data.varim!=NULL) {
    	      /* set variance flag */	
	      flag_variance=YES;  
	      if (!flag_quiet) printf(" and variance array");
	      output.varim=(float *)calloc(output.npixels,sizeof(float));
	      for (i=0;i<output.npixels;i++) output.varim[i]=data.varim[i];
	    }
	    if (!flag_quiet) printf("\n");
	  }


	  /* prepare space for variance image and initialize */
	  if (flag_variance && data.varim==NULL) {
	    output.varim=(float *)calloc(output.npixels,sizeof(float));
	    for (i=0;i<output.npixels;i++) output.varim[i]=0.0;
	  }
	  /* prepare space for the bad pixel mask and initialize */
	  if (data.mask==NULL) {
	    /* prepare image for mask == assumes mask is not yet present*/
	    output.mask=(short *)calloc(output.npixels,sizeof(short));
	    if (flag_bpm) /* copy mask over */
	      for (i=0;i<output.npixels;i++) output.mask[i]=bpm.mask[i];
	    else  /* start with clean slate */
	      for (i=0;i<output.npixels;i++) output.mask[i]=bpm.mask[i];
	  }


	
	  /* *************************************************/
	  /* ******** Confirm Correction Image Sizes  ********/
	  /* *************************************************/	  

	  /* need to make sure bias and flat are same size as image */
	  for (i=0;i<output.nfound;i++) {
	    if (output.axes[i]!=bpm.axes[i] && flag_bpm) {
	      printf("  BPM image %s (%dX%d) different size than science image %s       (%dX%d)\n",
	        bpm.name,bpm.axes[0],bpm.axes[1],
		output.name,output.axes[0],output.axes[1]);
	      exit(0);
	    }
	    if (output.axes[i]!=bias.axes[i] && flag_bias) {
	      printf("  BIAS image %s (%dX%d) different size than science image %s       (%dX%d)\n",
	        bias.name,bias.axes[0],bias.axes[1],
		output.name,output.axes[0],output.axes[1]);
	      exit(0);
	    }
	    if (output.axes[i]!=flat.axes[i] && flag_flatten) {
	      printf("  FLAT image %s (%dX%d) different size than science image %s       (%dX%d)\n",
	        flat.name,flat.axes[0],flat.axes[1],
		output.name,output.axes[0],output.axes[1]);
	      exit(0);
	    }
	    if (output.axes[i]!=fringe.axes[i] && flag_fringe) {
	      printf("  FRINGE image %s (%dX%d) different size than science image %s       (%dX%d)\n",
	        fringe.name,fringe.axes[0],fringe.axes[1],
		output.name,output.axes[0],output.axes[1]);
	      exit(0);
	    }
	    if (output.axes[i]!=pupil.axes[i] && flag_pupil) {
	      printf("  PUPIL GHOST image %s (%dX%d) different size than science image %s       (%dX%d)\n",
	        pupil.name,pupil.axes[0],pupil.axes[1],
		output.name,output.axes[0],output.axes[1]);
	      exit(0);
	    }
	    if (output.axes[i]!=illumination.axes[i] && flag_illumination) {
	      printf("  ILLUM image %s (%dX%d) different size than science image %s       (%dX%d)\n",
	        illumination.name,illumination.axes[0],illumination.axes[1],
		output.name,output.axes[0], output.axes[1]);
	      exit(0);
	    }
	  }

	  /* ************************************************/
	  /* ******** Report on Upcoming Processing  ********/
	  /* ************************************************/	  

	  if (flag_bias) {
	    if (fits_read_keyword(data.fptr,"DESBIAS",comment,comment,&status)==
	        KEY_NO_EXIST) {
	        if (!flag_quiet) printf("  BIAS subtracting\n");
	        flag_imbias=1;
	        status=0;
	    }
	    else {
	      if (!flag_quiet) 
	        printf("  Image %s previously BIAS subtracted\n",data.name);
	      flag_imbias=0;
	    }
	  }
	  if (flag_flatten) {
	    if (fits_read_keyword(data.fptr,"DESFLAT",comment,comment,&status)==
	        KEY_NO_EXIST) {
	        flag_imflatten=1;
	        status=0;
	        if (!flag_quiet) printf("  FLATTENing\n");
	    }
	    else {
	      if (!flag_quiet) 
	        printf("  Image %s previously FLATTENed\n",data.name);
	      flag_imflatten=0;
	    }
	  }
	  if (flag_pupil) {
	    if (fits_read_keyword(data.fptr,"DESPUPC",comment,comment,&status)==
	        KEY_NO_EXIST) {
	        flag_impupil=1;
	        status=0;
	        if (!flag_quiet) printf("  Pupil Ghost Correcting\n");
	    }
	    else {
	      if (!flag_quiet) 
	        printf("  Image %s previously Pupil Ghost Corrected\n",
		  data.name);
	      flag_impupil=0;
	    }
	  }
	  
	  /* ************************************************/
	  /* ********* Mask Missing Image Sections   ********/
	  /* ************************************************/	  

          /* get scale value for image */
	  /* note that this scalefactor is pre-bias correction */
	  /* but post overscan correction */
          retrievescale(&output,scaleregionn,scalesort,flag_quiet,
           &scalefactor,&mode,&fwhm);
	  /* cycle through image masking all pixels below threshold */
	  thresholdval=scalefactor*BADPIX_THRESHOLD;
	  maskedpixels=0;
	  for (i=0;i<output.npixels;i++) {
	    if ((output.image[i]<thresholdval || output.image[i]<0.0) &&
	      !output.mask[i]) {
	      output.mask[i]+=BADPIX_MASK; /* set the masked flag */
	      maskedpixels++;
	    }
	  }
	  if (!flag_quiet) printf("    %d pixels masked with value below %.2f\n",
	    maskedpixels,thresholdval);
	  /* recalculate scale factor if many pixels masked */
	  if (maskedpixels>0.1*output.npixels) {
            retrievescale(&output,scaleregionn,scalesort,flag_quiet,
             &scalefactor,&mode,&fwhm);
	  }

	  /* ************************************************/
	  /* ******** BIAS, Pupil Ghost and FLATTEN  ********/
	  /* ************************************************/	  

	  if ((flag_flatten && flag_imflatten) || (flag_bias && flag_imbias)
	    || (flag_pupil && flag_impupil)) {
	    /* retrieve sky brightness in image */
	    scale=1.0;offset=0.0;
	    saturatepixels=0;
	    for (i=0;i<output.npixels;i++) {
	      /* grab flat field scalefactor */
	      if (flag_flatten && flag_imflatten)
	        scale=flat.image[i];
	      /* grab bias offset */
	      if (flag_bias && flag_imbias) offset=bias.image[i];
	      else offset=0;
	      /* grab pupil offset */
	      if (flag_pupil && flag_impupil) 
		offset+=pupil.image[i]*scalefactor;
	      /* do math using mask  */
	      if (!output.mask[i]) {
	        if (i%output.axes[0]<1024) { /* amplifier A */
	          if (output.image[i]>output.saturateA) {
		    output.mask[i]+=BADPIX_SATURATE;
		    saturatepixels++;
		    /* boost all saturated pixels above maxsaturate */
		    if (output.image[i]<maxsaturate) 
		      output.image[i]=scale*maxsaturate+offset;
		  }
		}		
		else {/* amplifier B */
		  if (output.image[i]>output.saturateB) {
		    output.mask[i]+=BADPIX_SATURATE;
		    saturatepixels++;
		    /* boost all saturated pixels above maxsaturate */
		    if (output.image[i]<maxsaturate) 
		       output.image[i]=scale*maxsaturate+offset;
		  }
		}
	        output.image[i]=(output.image[i]-offset)/scale;
	      }
	      /* set pixel to 0.0 if it's in a bad pixel */
	      else output.image[i]=0.0;
	  }
	  if (!flag_quiet) printf("    Masked %d saturated pixels\n",
	    saturatepixels);

	  } /* end BIAS and FLATTEN */

	  
	  /* ********************************************* */
	  /* ********** VARIANCE Image Section *********** */
	  /* ********************************************* */

	  if (flag_variance && (flag_bpm || flag_bias || flag_flatten)) {
	    if (!flag_quiet) printf("  Creating CCD statical variance image\n");
	    for (i=0;i<output.npixels;i++) 
	      if (((flag_bpm && !output.mask[i]) || !flag_bpm) && 
		bias.varim[i]>0.0 && flat.varim[i]>0.0 ) {
	        if (i%output.axes[0]<1024) { /* in AMP A section */	      
		  /* each image contributes Poisson noise, RdNoise */
		  /*  and BIAS image noise */
	          sumval=(output.image[i]/output.gainA+
		    Squ(output.rdnoiseA/output.gainA));
		}
		else { /* in AMP B */
		  /* each image contributes Poisson noise, RdNoise */
		  /*  and BIAS image noise */
	          sumval=(output.image[i]/output.gainB+
		    Squ(output.rdnoiseB/output.gainB));	
		}
		output.varim[i]=1.0/(sumval+1.0/bias.varim[i]+
		  1.0/flat.varim[i]);
	      }
	      else output.varim[i]=0.0;
	  }


	  
	  /* ********************************************************** */
	  /* ******** INTERPOLATE Image and Variance Section ********** */
	  /* ********************************************************** */


	  if (flag_interpolate_col && flag_bpm) {
	    if (!flag_quiet) printf("  Interpolating over bad pixels: ");
	    interppixels=0;
	    for (i=0;i<output.npixels;i++) {
	    	if (output.mask[i]==BADPIX_BPM) {/* this is bad pixel */
		  xlow=i-1;
		  while (output.mask[xlow]>0) xlow--;
		  xhi=i+1;
		  while (output.mask[xhi]>0) xhi++;
		  output.image[i]=0.5*(output.image[xlow]+output.image[xhi]);
		  /* could add noise to counteract averaging of 2 pixels */
		  /*if (flag_variance) output.image[i]+=gasdev(&seed)*
		    sqrt(0.25*(1.0/output.varim[xlow]+1.0/output.varim[xhi]));*/
		  output.mask[i]+=BADPIX_INTERP; /* set the interpolate flag */
	          interppixels++;
		  /* now calculate weight for this pixel */
		  /* allows for non-zero weight if interpolation */
		  /* over 2 or fewer pixels */
		  if (flag_variance && (xhi-xlow)<=3) { 
		    if (output.varim[xlow]>1.0e-10 && output.varim[xhi]>1.0e-10)
		      output.varim[i]=1.0/(0.25*(1.0/output.varim[xlow]+
		      1.0/output.image[xhi]));
		    /* else leave pixel with zero weight*/
		    /* scale weight down because this is interpolated pixel */
		    output.varim[i]/=scale_interpolate;
		  }
		}
	    }
	    if (!flag_quiet) printf("  %d pixels\n",interppixels);
	  }




	
	  /* **************************** */
	  /* **** FRINGE Correction ***** */
	  /* **************************** */

	  if (flag_fringe) {
	    flag_imfringe=YES;
	    if (!flag_quiet) {
	      printf("  Applying Fringe correction");
	      fflush(stdout);
	    }
	    /* retrieve overall scaling from image */
	    retrievescale(&output,scaleregionn,scalesort,flag_quiet,
	      &scalefactor,&mode,&fwhm);
	    if (!flag_quiet) {
	      printf("  Image Scalefactor %10.4e",scalefactor);
	      fflush(stdout);
	    }
	    for (i=0;i<output.npixels;i++) {
	      output.image[i]-=scalefactor*fringe.image[i];
	    }
	    if (!flag_quiet) {
	      printf("\n");
	      fflush(stdout);
	    }
	  }	



	
	  /* ********************************** */
	  /* **** ILLUMUNATION Correction ***** */
	  /* ********************************** */

	  if (flag_illumination) {
	    flag_imillumination=YES;
	    if (!flag_quiet) {
	      printf("  Applying Illumination correction to image");
	      fflush(stdout);
	    }
	    for (i=0;i<output.npixels;i++) 
	      output.image[i]/=illumination.image[i];
	    if (flag_variance) {
	      if (!flag_quiet) {
	        printf(" and variance image");
	        fflush(stdout);
	      }
	      for (i=0;i<output.npixels;i++) 
	        output.varim[i]/=Squ(illumination.image[i]);
	    }
	    if (!flag_quiet) {
	      printf("\n");
	      fflush(stdout);
	    }
	  }	


	  /* *************************************** */
	  /* ********* CALCULATE SKYBRITE ********** */
	  /* *************************************** */

	  if (flag_updateskybrite) {
	    /* retrieve overall scaling from image */
	    retrievescale(&output,scaleregionn,scalesort,flag_quiet,
	      &skybrite,&mode,&fwhm);
	    if (!flag_quiet) {
	      printf("  Image SKYBRITE is %10.4e\n",skybrite);
	      fflush(stdout);
	    }
	  }

/*
	  interppixels=0;
	  for (i=0;i<output.npixels;i++) if (output.mask[i]) interppixels++;
	  printf("** %d masked pixels\n",interppixels);
*/

	  /* *********************** */
	  /* **** SAVE RESULTS ***** */
	  /* *********************** */

	  if (flag_output) {
	    /* if writing individual images for each component */
	    /* then get these names now */
	    if (!flag_mef) {
	      /* strip off the ".fits" */
	      sprintf(rootname,"%s",imagename);
	      for (i=strlen(rootname);i>=0;i--) 
		if (!strncmp(rootname+i,".fits",5)) break;
	      rootname[i]=0;
	      /* create the three names */
	      sprintf(imagename,"!%s_im.fits",rootname);
	      sprintf(output.name,"%s",imagename);
	      sprintf(bpmname,"!%s_bpm.fits",rootname);
	      sprintf(varimname,"!%s_var.fits",rootname);
	    }
	    /* create the (image) file */
            if (fits_create_file(&output.fptr,output.name,&status)) 
	      printerror(status);
	  
	    /* create the image */
	    if (fits_create_img(output.fptr,FLOAT_IMG,2,output.axes,&status))
	      printerror(status);
	      
	    /* copy header */
	    /* if (fits_copy_header(data.fptr,output.fptr,&status))
	      printerror(status);*/
	    /* first read the number of keywords */
	    if (fits_get_hdrspace(data.fptr,&keysexist,&j,&status))
	      printerror(status);
	    if (!flag_quiet) printf("  Copying %d keywords into output image\n",
	      keysexist);
	    /* reset to beginning of output header space */
	    if (fits_read_record(output.fptr,0,keycard,&status))
	      	printerror(status);
	    if (fits_read_record(data.fptr,0,keycard,&status))
	      	printerror(status);
	    for (j=1;j<=keysexist;j++) {
	      if (fits_read_record(data.fptr,j,keycard,&status))
	      	printerror(status);
	      if (strncmp(keycard,"BITPIX  ",8) && 
	          strncmp(keycard,"NAXIS",5)    &&
	          strncmp(keycard,"PCOUNT  ",8) &&
	          strncmp(keycard,"EXTEND  ",8) &&
	          strncmp(keycard,"SIMPLE  ",8) &&		  		  
	          strncmp(keycard,"GCOUNT  ",8) &&
	          strncmp(keycard,"COMMENT   FITS (Flexible Image",30) &&
	          strncmp(keycard,"COMMENT   and Astrophysics', v",30) &&
	          strncmp(keycard,"EXTNAME ",8) &&
	          strncmp(keycard,"BSCALE  ",8) &&		  		  
	          strncmp(keycard,"BZERO   ",8) 
		  ) { 
	        /*if (!flag_quiet) printf("    %s\n",keycard);*/
	        /*for (n=0;n<8;n++) keyname[n]=keycard[n];
	        keyname[8]=0;*/
	        if (fits_write_record(output.fptr,keycard,&status)) 
	          printerror(status);
	      }
	    }
	      
	    
	  }
	  else {  /* writing on top of input image */

	    /* resize image */
	    if (!flag_quiet) printf("  Resizing output image %s to %d X %d\n",
	      output.name+1,output.axes[0],output.axes[1]);
	    if (fits_resize_img(output.fptr,FLOAT_IMG,2,output.axes,&status)) 
	      printerror(status);	 


	    /* change BZERO */
	    if (fits_set_bscale(output.fptr,1.0,0.0,&status)) 
	      printerror(status);
	    output.bzero=0;
	    if (fits_update_key_lng(output.fptr,"BZERO",output.bzero,comment,
	      &status)) printerror(status);
	  }


	  /* write the corrected image*/
	  if (fits_write_img(output.fptr,TFLOAT,1,output.npixels,
	    output.image,&status)) printerror(status);

	  /* free image array */ 
	  if (flag_output) {
	    free(data.image);
	    if (!flag_quiet) printf("  Deallocating data.image\n");
	  }
	  free(output.image); 
	  if (!flag_quiet) printf("  Deallocating output.image\n");


	  /* store information in the header */
	  pip=popen("date","r");
	  fgets(comment,200,pip);
	  comment[strlen(comment)-1]=0;
	  pclose(pip);
	  if (flag_overscan && flag_imoverscan) {
	    if (fits_write_key_str(output.fptr,"DESOSCN",comment,
	      "overscan corrected",&status)) printerror(status);
	    /* update the DATASEC keyword */
	    if (fits_update_key_str(output.fptr,"DATASEC",output.datasec,
	      "Data section within image",&status)) printerror(status);
	  }
	  if (flag_bias && flag_imbias)
	    if (fits_write_key_str(output.fptr,"DESBIAS",comment,
	      "bias subtracted",&status)) printerror(status);
	  if (flag_pupil && flag_impupil)
	    if (fits_write_key_str(output.fptr,"DESPUPC",comment,
	      "pupil corrected",&status)) printerror(status);
	  if (flag_flatten && flag_imflatten)
	    if (fits_write_key_str(output.fptr,"DESFLAT",comment,
	      "flat fielded",&status)) printerror(status);
	  if (flag_illumination && flag_illumination)
	    if (fits_write_key_str(output.fptr,"DESILLUM",comment,
	      "Illumination Corrected",&status)) printerror(status);
	  if (flag_fringe && flag_imfringe)
	    if (fits_write_key_str(output.fptr,"DESFRING",comment,
	      "Fringe Corrected",&status)) printerror(status);
	  if (flag_updateskybrite)
	    if (fits_update_key_flt(output.fptr,"SKYBRITE",skybrite,7,
	      "Sky Brightness (ADU)",&status)) printerror(status);

	  sprintf(longcomment,"DESDM:");
	  for (i=0;i<argc;i++) sprintf(longcomment,"%s %s",longcomment,argv[i]);
	  if (!flag_quiet) printf("\n  %s\n", longcomment);
	  if (fits_write_comment(output.fptr,longcomment,&status)) 
	    printerror(status);
	  if (fits_update_key_str(output.fptr,"DES_EXT","IMAGE",
	    "Image extension",&status)) printerror(status);
	    
	  /* remove unneeded information from the header */
	  nkeys=0;
	  while (strlen(delkeys[nkeys])) {
	    if (fits_read_keyword(output.fptr,delkeys[nkeys],
		comment,comment,&status)==KEY_NO_EXIST) status=0;
	    else {
	      if (fits_delete_key(output.fptr,delkeys[nkeys],&status)) {
	        if (!flag_quiet) 
		  printf("  ** Keyword %s not deleted from image %s\n",
		  delkeys[nkeys],output.name);	  
	        status=0;
	      }
	    }
	    nkeys++;
	  }

	  /* write new keywords */
	  if (fits_write_key(output.fptr,TFLOAT,"SATURATE",&maxsaturate,
	    "Global Saturate Value",&status)) printerror(status);
	  
	  /* update existing keywords */
	  if (fits_update_key_str(output.fptr,"OBSTYPE","reduced",
	    "Observation type",&status)) printerror(status);
	    
	  /* close and reopen if outputting individual images */
	  if (!flag_mef) {
	    /* close the image file */
            if (fits_close_file(output.fptr,&status)) printerror(status);
	    if (!flag_quiet) printf("  Opening bpm image %s\n",bpmname+1);
	    /* open the bpm file */
            if (fits_create_file(&output.fptr,bpmname,&status)) 
	      printerror(status);
	  }

	  /* now store the bad pixel mask that has been created or updated */
	  /* first create a new extension */
	  if (fits_create_img(output.fptr,USHORT_IMG,2,output.axes,&status))
	    printerror(status);
	  /* write the data */	  
	  if (fits_write_img(output.fptr,TUSHORT,1,output.npixels,output.mask,
	    &status)) printerror(status);
	  /* free image array */ 
	  if (!flag_quiet) printf("  Deallocating output.mask\n");
	  free(output.mask);   

	  if (fits_update_key_str(output.fptr,"DES_EXT","BPM",
	    "Extension type",&status)) printerror(status);
	  	    
	  /* close and reopen if outputting individual images */
	  if (!flag_mef) {
	    /* close the image file */
            if (fits_close_file(output.fptr,&status)) printerror(status);
	    if (!flag_quiet) 
	      printf("  Opening variance image %s\n",varimname+1);
	    /* open the bpm file */
            if (fits_create_file(&output.fptr,varimname,&status)) 
	      printerror(status);
	  }

	  /* now store the variance image that has been created or updated */
	  /* first create a new extension */
	  if (flag_variance) {
	    if (fits_create_img(output.fptr,FLOAT_IMG,2,output.axes,&status))
	      printerror(status);
	    /* write the data */	  
	    if (fits_write_img(output.fptr,TFLOAT,1,output.npixels,output.varim,
	      &status)) printerror(status);
	    /* free variance array */ 
	    free(output.varim);   
	    if (!flag_quiet) printf("  Deallocating output.varim\n");
	    if (fits_update_key_str(output.fptr,"DES_EXT","VARIANCE",
	      "Extension type",&status)) printerror(status);
	  }
	  	    
          /* close the corrected image */
          if (fits_close_file(output.fptr,&status)) printerror(status);
	  /* close the input image if needed */
	  if (flag_output) if (fits_close_file(data.fptr,&status)) 
	    printerror(status);
	  if (!flag_quiet) printf("  Closed %s: 2D ( %d X %d )\n",
	    &(output.name[flag_output]),output.axes[0],output.axes[1]);
	  
	  if (!flag_quiet) printf("\n\n");
	} /* end of image processing cycle with variable im */
	if (flag_list) {
	  fclose(inp);
	  printf("  Closed image list");
	  if (flag_output) {
	    printf("s\n");
	    fclose(out);
	  }
	  else printf("\n");
	}

	return (0);
}


#undef BADPIX_BPM
#undef BADPIX_THRESHOLD
#undef BADPIX_SATURATE 
#undef BADPIX_INTERP 
#undef BADPIX_MASK
