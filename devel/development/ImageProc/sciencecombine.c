/* Combine reduced science frames into a composite image */
/* for use in the illumination and fringe corrections */
/*
/*
/*  Expects input list of images-- but expects them to be single HDU images 
/*  in standard form output by imcorrect:  *_im.fits *_bpm.fits
/*
/*
*/

#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
	int	status=0,i,j,im,imnum,x,y,loc,xmin,xmax,ymin,ymax,num,delta,
		xdim,ydim,npixels;
	long	ccdnum;
	char	comment[1000],imagename[1000],command[1000],
		longcomment[1000],scaleregion[100],filter[200],
		obstype[200],maskname[1000],temp[1000];
	float	avsigclip_sigma,srcgrowrad=0.0,avsig,sigma,
		*vecsort,*scalefactor,*scalesort,maxval,*meanimage,
		val,sigmalim,mean,mode,fwhm,threshold;
	static desimage *data,output;
	int	flag_quiet=0,flag_combine=0,flag_bias=0,flag_filter=0,
		flag_variance=0,flag_bpm=1,hdutype,flag_srcgrowrad=0,
		flag_rejectobjects=1,flag_outputmasks=0,
		flag_scale=1,scaleregionn[4]={500,1500,1500,2500},scalenum,
		avsignum,srcgrowrad_int,valnum,dy,dx,newx,newy;
	void	rd_desimage(),shell(),decodesection(),retrievescale(),
		headercheck(),printerror();
	double	avsigsum,val1sum,val2sum,weightsum;
	short	interppix=4,maskpix=8,growpix=16,rejectpix=32;
	fitsfile *fptr;
	FILE	*inp,*pip;

	if (argc<3) {
	  printf("sciencecombine <input list> <output image> <options>\n");
	  printf("  Preprocessing Options\n");
	  printf("    -noscale\n");
	  printf("    -scaleregion <Xmin:Xmax,Ymin,Ymax>\n");
	  printf("  Filter Options\n");
	  printf("    -norejectobjects\n");
	  printf("    -avsigclip <Sigma,3>\n");
	  printf("    -srcgrowrad <radius,0>\n");
	  printf("  Combine Options\n");
	  printf("    -average\n");
	  printf("    -median (default)\n");
	  printf("  Output Options\n");
	  printf("    -outputmasks\n");
	  printf("    -quiet\n");
	  exit(0);
	}


	/* ******************************************* */
	/* ********  PROCESS COMMAND LINE ************ */
	/* ******************************************* */

	/* first cycle through looking for quiet flag */
	for (i=3;i<argc;i++) 
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1; 

	/* open list of input images */
	imnum=0;
	inp=fopen(argv[1],"r");
	if (inp==NULL) {
	  printf("  ** File %s not found!\n",argv[1]);
	  exit(0);
	}
	while(fscanf(inp,"%s",imagename)!=EOF) imnum++;
	fclose(inp);
	if (!flag_quiet) printf("  Input list contains %d images\n",imnum);

	/* prepare output file */
	sprintf(output.name,"!%s",argv[2]);
	if (strncmp(&(output.name[strlen(output.name)-5]),".fits",5)
	 && !strncmp(&(output.name[strlen(output.name)-8]),".fits.gz",8)) {
	   printf("  ** Output file must be FITS or compressed FITS image\n");
	   exit(0);
	}

	/* set defaults */
	flag_combine=MEDIAN;
	flag_scale=YES;
	flag_filter=NO;
	/* process command line options */
	for (i=3;i<argc;i++) {
	  if (!strcmp(argv[i],"-noscale")) flag_scale=NO;
	  if (!strcmp(argv[i],"-norejectobjects")) flag_rejectobjects=NO;
	  if (!strcmp(argv[i],"-scaleregion")) {
	    sprintf(scaleregion,"%s",argv[i+1]);
	    decodesection(scaleregion,scaleregionn,flag_quiet);
	  }
	  if (!strcmp(argv[i],"-average")) flag_combine=AVERAGE;
	  if (!strcmp(argv[i],"-median"))  flag_combine=MEDIAN;
	  if (!strcmp(argv[i],"-avsigclip")) {
	    flag_filter=AVSIGCLIP;
	    sscanf(argv[i+1],"%f",&avsigclip_sigma);
	  }
	  if (!strcmp(argv[i],"-srcgrowrad")) {
	    flag_srcgrowrad=YES;
	    sscanf(argv[i+1],"%f",&srcgrowrad);
	  }
	  if (!strcmp(argv[i],"-outputmasks")) flag_outputmasks=YES;
	}	

	
	/* ***************************************************************** */
	/* ********  READ and determine SCALEFACTORS for Input Images ****** */
	/* ********           also, Reject Objects                    ****** */
	/* ***************************************************************** */

	/* create an array of image structures to hold the input images */
	data=(desimage *)calloc(imnum+1,sizeof(desimage));
	/* vector for median combining the images */
	if (flag_combine==MEDIAN) vecsort=(float *)calloc(imnum,sizeof(float));
	/* vector for determining the scale of an image */
	if (flag_scale) { /* prepare for medianing the scale */
	  scalenum=(scaleregionn[1]-scaleregionn[0])*(scaleregionn[3]-
	    scaleregionn[2]);
	  scalesort=(float *)calloc(scalenum,sizeof(float));
	}
	/* vector for holding scale factor */
	scalefactor=(float *)calloc(imnum,sizeof(float));	
	
	/* now cycle through input images to read and calculate scale factors */
	inp=fopen(argv[1],"r");
	for (im=0;im<imnum;im++) {
	  fscanf(inp,"%s",imagename);
	  if (!strncmp(&(imagename[strlen(imagename)-5]),".fits",5))
	    imagename[strlen(imagename)-5]=0;
	  else if (strncmp(&(imagename[strlen(imagename)-8]),".fits.gz",8)) 
	    imagename[strlen(imagename)-8]=0;
	  else {
	    printf("  ** Image lists must contain FITS or compressed FITS files\n");
	    exit(0);
	  }
	  
	  sprintf(data[im].name,"%s_im.fits",imagename);
	  if (!flag_quiet) printf("  Reading image %s\n",data[im].name);

	  /* read input image */
	  rd_desimage(data+im,READONLY,flag_quiet);

	  /* check image */
	  headercheck(data+im,filter,ccdnum,"NOCHECK");

	  /* first time through pull out image dimensions */
	  if (!im) {
	    xdim=data[im].axes[0];
	    ydim=data[im].axes[1];
	    npixels=data[im].npixels;
	  }
	  else 
	    if (data[im].axes[0]!=xdim || data[im].axes[1]!=ydim 
	      || data[im].npixels!=npixels) 
	      printf("  ** Input image %s has different dimensions than previous images\n",
	        data[im].name);

	  /* confirm OBSTYPE */
	  if (fits_read_key_str(data[im].fptr,"OBSTYPE",obstype,comment,
	    &status)==KEY_NO_EXIST) {
	    printf("  ** OBSTYPE keyword not found in %s \n",data[im].name);
	    exit(0);
	  }
	  if (strncmp(obstype,"reduced",7) && strncmp(obstype,"REDUCED",7)) {
	    printf("  ** Expecting OBSTYPE 'reduced' in %s\n",data[im].name);
	    exit(0);
	  }

	  /* close image now */
	  if (fits_close_file(data[im].fptr,&status)) printerror(status);

	  /* also read the mask image */
	  sprintf(data[im].name,"%s_bpm.fits",imagename);
	  if (!flag_quiet) printf("  Reading associated mask image %s\n",
	    data[im].name);

	  /* read input image */
	  rd_desimage(&(data[im]),READONLY,flag_quiet);

	  /* close mask image now */
	  if (fits_close_file(data[im].fptr,&status)) printerror(status);

	  /* check on image sizes */
	  if (data[im].axes[0]!=xdim || data[im].axes[1]!=ydim || 
	    data[im].npixels!=npixels) {
	      printf("  ** Input image %s has different dimensions than previous images\n",
	        data[im].name);
	  }

	  /* calculate the scalefactor, mode and fwhm in the scaleregion */
	  /* also read the mask image */
	  sprintf(data[im].name,"%s_im.fits",imagename);
	  if (flag_scale || flag_rejectobjects) 
	    retrievescale(data+im,scaleregionn,scalesort,flag_quiet,
	      scalefactor+im,&mode,&fwhm);

	  /* check scalefactor */
	  if (flag_scale) {
	    /* mask entire image if scalefactor unknown */
	    if (fabs(scalefactor[im])<1.0e-4) {
	      for (i=0;i<npixels;i++) data[im].mask[i]|=maskpix;
	      scalefactor[im]=1.0;
	    }
	  }
	  else scalefactor[im]=1.0;

	  /* cycle through masking objects */
	  if (flag_rejectobjects) {
	    /* set threshold at ~12 sigma above the mode in the sky */
	    threshold=mode+5.0*fwhm;
	    for (i=0;i<npixels;i++) 
	      if (data[im].image[i]>threshold) data[im].mask[i]|=rejectpix;
	  }

	} 

	/* close input file list */	
	fclose(inp);
	if (!flag_quiet) printf("  Closed image list %s\n",argv[1]);

	

	/* *********************************************** */
	/* *********  PREPARE OUTPUT IMAGE *************** */
	/* *********************************************** */

	im=0;
	output=data[im];
	sprintf(output.name,"!%s",argv[2]);
	for (i=0;i<output.nfound;i++) output.axes[i]=data[im].axes[i];
	output.bitpix=FLOAT_IMG;
	output.npixels=data[im].npixels;
	output.image=(float *)calloc(output.npixels,sizeof(float));
	if (flag_variance) {
	  /* prepares variance image */
	  output.varim=(float *)calloc(output.npixels,sizeof(float));
	}




	/* ********************************************** */
	/* *********  GROW RADIUS SECTION *************** */
	/* ********************************************** */

	if (flag_srcgrowrad==YES) {
	  if (!flag_quiet) printf("  Beginning source grow radius filtering\n"); 
	  srcgrowrad_int=srcgrowrad;
	  srcgrowrad=Squ(srcgrowrad);
	  for (im=0;im<imnum;im++) {
	    for (y=0;y<ydim;y++) 
	      for (x=0;x<xdim;x++) {
	      loc=x+y*xdim;
	      /* if bad pixel but not interpolated */
	      if (data[im].mask[loc] && !(data[im].mask[loc]&interppix) && 
		!(data[im].mask[loc]&growpix)) 
		for (dy=-srcgrowrad_int;dy<=srcgrowrad_int;dy++) {
		  newy=y+dy;
		  if (newy>=0 && newy<ydim) {
		    xmax=(int)sqrt(srcgrowrad-Squ(dy));
		    for (dx=-xmax;dx<=xmax;dx++) {
		      newx=x+dx;
		      if (newx>=0 && newx<xdim) 
		        data[im].mask[newx+newy*xdim]|=growpix;
	  	    } /* cycle through x */
		  } /* y within range? */
		} /* cycle through y */
	    } /* cycle through pixels of image */
	  } /* cycle through list of images */
	}
	

	/* ***************************************************************** */
	/* *********  FILTER IMAGE using AVSIGCLIP if NEEDED *************** */
	/* ***************************************************************** */

	if (flag_filter==AVSIGCLIP) {
	  if (!flag_quiet) printf("  Beginning AVSIGCLIP filtering\n"); 
	  /* first determine the average sigma */
	  for (y=scaleregionn[2]-1;y<scaleregionn[3]-1;y++) 
	    for (x=scaleregionn[0]-1;x<scaleregionn[1]-1;x++) {
	    loc=x+y*xdim;
	    num=0;val1sum=val2sum=0.0;
	    for (im=0;im<imnum;im++)
	      if (!data[im].mask[loc]) {
	      	val=(data[im].image[loc]/scalefactor[im]);
	      	val1sum+=val;
		val2sum+=Squ(val);
	        num++;
	      }
	    if (num>3) {
	      mean=val1sum/(float)num;
	      sigma=sqrt(val2sum/(float)num-Squ(mean));
	      avsigsum+=sigma;
	      avsignum++;
	      if (avsignum>AVSIGCLIPMAXNUM) break;
	    }
	  }
	  avsig=avsigsum/(float)avsignum;
	  if (!flag_quiet) printf("    Average sigma is %12.3e\n",avsig); 

	  /* now go through clipping with avsig */
	  /* scale avsig by the number of sigma requested */
	  avsig*=avsigclip_sigma;
	  for (y=0;y<ydim;y++) 
	    for (x=0;x<xdim;x++) {
	    loc=x+y*xdim;
	    /* first determine mean value for this pixel */
	    val1sum=0.0;valnum=0;
	    for (im=0;im<imnum;im++) 
	      /* if pixel not flagged or pixel has been interpolated */
	      if (!data[im].mask[loc] || data[im].mask[loc]&interppix) {
	        val1sum+=(data[im].image[loc]/scalefactor[im]);
	        valnum++;
	    }
	    mean=val1sum/(float)valnum;
	    /* second, flag deviant pixels */
	    for (im=0;im<imnum;im++) 
	      if (!data[im].mask[loc] || data[im].mask[loc]&interppix) 
	        if (fabs((data[im].image[loc]/scalefactor[im])-mean)>avsig) 
		  data[im].mask[loc]|=maskpix;
	  }
	}
	

	/* ******************************************************** */
	/* *********  RECALCULATE the SCALEFACTORs  *************** */
	/* ******************************************************** */

	if (flag_scale) {
	  for (im=0;im<imnum;im++) {
	    retrievescale(data+im,scaleregionn,scalesort,flag_quiet,
	      scalefactor+im,&mode,&fwhm);
	    /* mask entire image of scale factor cannot be determined */
	    if (fabs(scalefactor[im])<1.0e-4) {
	      for (i=0;i<npixels;i++) data[im].mask[i]|=maskpix;
	      scalefactor[im]=1.0;
	    }
	  }
	}

	if (flag_outputmasks) for (im=0;im<imnum;im++) {
	  /* set all masked pixels to 1.0 */
	  for (i=0;i<npixels;i++) 
	    if (data[im].mask[i] && !(data[im].mask[i]&interppix))
	      data[im].image[i]=scalefactor[im];
	  /* now write the image */
	  sprintf(temp,"%s",data[im].name);
	  for (i=strlen(temp);i>=0;i--)
	    if (!strncmp(temp+i,"_im.fits",8)) {
	      temp[i]=0;
	      break;
	    }
	  sprintf(maskname,"!%s_mask.fits",temp);
	      
	  if (!flag_quiet) printf("    Writing mask file %s\n",maskname+1);
          if (fits_create_file(&fptr,maskname,&status))              
		printerror(status);
          if (fits_create_img(fptr,FLOAT_IMG,2,data[im].axes,&status))            
		printerror(status);
          if (fits_write_img(fptr,TFLOAT,1,npixels,data[im].image,
                &status)) printerror(status);
          if (fits_close_file(fptr,&status)) printerror(status);
	}

	
	/* ******************************************* */
	/* *********  COMBINE SECTION *************** */
	/* ******************************************* */
	
	/* Combine the input images to create composite */
	if (flag_combine==AVERAGE) {
	  if (!flag_quiet) printf("  Combining images using AVERAGE\n");
  	  for (i=0;i<output.npixels;i++) {
	    val1sum=weightsum=0.0;
	    for (im=0;im<imnum;im++) 
	      if (!data[im].mask[loc] || data[im].mask[loc]&interppix) {
		/* carry out a variance weighted sum */
	  	val1sum+=(data[im].image[i]);
	  	weightsum+=scalefactor[im];
		valnum++;
	      }
	    if (valnum) output.image[i]=val1sum/weightsum;
	    else output.image[i]=1.0;
	  }
	}
	
	if (flag_combine==MEDIAN) {
	  if (!flag_quiet) printf("  Combining images using MEDIAN\n");
  	  for (i=0;i<npixels;i++) { /* for each pixel */
	    output.image[i]=1.0;
	    /* copy values into sorting vector */
	    j=0;
	    for (im=0;im<imnum;im++) {
	      if (!data[im].mask[i] || (data[im].mask[i]&interppix)) {
	        vecsort[j]=data[im].image[i]/scalefactor[im];
	        j++;
	      }
	    }
	    if (j>0) {
	      shell(j,vecsort-1);
	      /* odd number of images */
	      if (j%2) output.image[i]=vecsort[j/2]; /* record median value */  
	      else output.image[i]=0.5*(vecsort[j/2]+vecsort[j/2-1]);
	    }
	    else output.image[i]=1.0;
	  }
	}



	/* *********************************** */
	/* **** RENORMALIZE IMAGE TO 1.0 ***** */
	/* *********************************** */


			
	/* ***************************** */
	/* **** WRITE OUTPUT IMAGE ***** */
	/* ***************************** */

	printf("  Writing results to %s",output.name+1);
	/* create the file */
        if (fits_create_file(&output.fptr,output.name,&status)) 
	  printerror(status);

	/* create image extension */
	if (fits_create_img(output.fptr,FLOAT_IMG,2,output.axes,&status)) printerror(status);
	/* write the corrected image*/
	if (fits_write_img(output.fptr,TFLOAT,1,output.npixels,output.image,&status))
	  printerror(status);
	  
	/* write basic information into the header */
	if (fits_update_key_str(output.fptr,"OBSTYPE","sccombine",
	  "Observation type",&status)) printerror(status);  
	if (fits_update_key_str(output.fptr,"FILTER",filter,
	  "Filter name(s)",&status)) printerror(status);
	if (fits_update_key_lng(output.fptr,"CCDNUM",ccdnum,
	  "CCD number",&status)) printerror(status);
	/* Write information into the header describing the processing */
	pip=popen("date","r");
	fgets(comment,200,pip);
	comment[strlen(comment)-1]=0;
	pclose(pip);
	if (fits_write_key_str(output.fptr,"DESSCOMB",comment,
	  "sciencecombine output",&status))
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


        /* close the corrected image */
        if (fits_close_file(output.fptr,&status)) printerror(status);

	free(output.image);
	for (im=0;im<imnum;im++) {
	  free(data[im].image);
	  free(data[im].mask);
	}

	printf("\n");

	return(0);
}
