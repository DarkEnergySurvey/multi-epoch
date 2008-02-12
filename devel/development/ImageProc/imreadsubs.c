/* image reading and writing subroutines */
#define VERBOSE 0

/* currently assumes image data in first extension and doesn't bother */
/* to read other extensions */

#include "imageproc.h"


void rd_desimage(image,mode,flag_quiet)
	desimage *image;
	int mode,flag_quiet;
{
        int     status=0,anynull,hdu,flag_type=0,hdutype,flag_nonmef=NO,
		imcount,hdunum;
	float	saturate;
	char	comment[1000],type[100],compressname[500],name_im[500],
		rootname[500],compressname_im[500],currentimage[500],
		obstype[50];
        void    printerror();

	/* is image a FITS file */
	sprintf(currentimage,"%s",image->name);
        if (strncmp(".fits",&(currentimage[strlen(currentimage)-5]),5) && 
	  strncmp("fit",&(currentimage[strlen(currentimage)-4]),4) && 
	  strncmp(".fits.gz",&(currentimage[strlen(currentimage)-8]),8)) {
	  printf("  ** fits or fits.gz image name required:  %s **\n",
	    currentimage);
	  exit(0);
	}
	
        /* open the file */
        if (fits_open_file(&image->fptr,currentimage,mode,&status)) {
	  /* check if .gz version is available */
	  sprintf(currentimage,"%s.gz",image->name);
	  status=0;
          if (fits_open_file(&image->fptr,currentimage,mode,&status)) {
	    status=0;
	    sprintf(rootname,"%s",currentimage);
	    rootname[strlen(rootname)-8]=0;
	    sprintf(currentimage,"%s_im.fits",rootname);
            if (fits_open_file(&image->fptr,currentimage,mode,&status)) {
	      status=0;
	      sprintf(currentimage,"%s_im.fits.gz",rootname);
              if (fits_open_file(&image->fptr,currentimage,mode,&status)) {
	        printf("  ** Images %s(.gz) and %s(.gz) not found\n",
	  	  image->name,currentimage);
	        printerror(status);
	      }
	    }
	    flag_nonmef=YES;
	  }
	}
	
	/* determine how many extensions */
	if (fits_get_num_hdus(image->fptr,&(image->hdunum),&status)) 
	  printerror(status);
	if (!flag_quiet) {
	  printf("    %s has %d HDU",currentimage,image->hdunum);
	  if (image->hdunum>1) printf("s\n");
	  else printf("\n");
	}
	
	/* read the NAXIS1 and NAXIS2 keyword to get image size */
	/* assumes all extensions have images of the same size */
        if (fits_read_keys_lng(image->fptr,"NAXIS",1,2,image->axes,
	  &image->nfound,&status)) printerror(status);
	if (image->nfound>2) {
	  printf("  ** Image dimension too large (%s,%d)\n",
	    currentimage,image->nfound);
	  exit(0);
	}
	if (!flag_quiet)
	  printf("    Opened %s:(%dX%d)\n",
	    currentimage,image->axes[0],image->axes[1]);
        image->npixels  = image->axes[0]*image->axes[1];
	
	/* current hardwiring these FITS parameters */
        image->fpixel=1;image->nullval=0.0;image->shnullval=0;

	if (flag_nonmef)  /* must cycle to look for im, var and bpm images */
	  imcount=0;
	else imcount=2;

	while (imcount<3) {
	  hdunum=image->hdunum;

	  if (imcount==1) { /* look for the bpm image */
	    sprintf(name_im,"%s_var.fits",rootname);
            /* open the file */
            if (fits_open_file(&image->fptr,name_im,mode,&status)) {
	      /* check if .gz version is available */
	      sprintf(compressname,"%s.gz",name_im);
	      status=0;
              if (fits_open_file(&image->fptr,compressname,mode,&status)) {
	        printf("  ** Neither %s nor %s found\n",name_im,compressname);
	        status=0;
		hdunum=0;
	      }
	    }
	  }

	  if (imcount==2 && flag_nonmef) { /* look for the var image */
	    sprintf(name_im,"%s_bpm.fits",rootname);
            /* open the file */
            if (fits_open_file(&image->fptr,name_im,mode,&status)) {
	      /* check if .gz version is available */
	      sprintf(compressname,"%s.gz",name_im);
	      status=0;
              if (fits_open_file(&image->fptr,compressname,mode,&status)) {
	        printf("  ** Neither %s nor %s found\n",name_im,compressname);
	        status=0;
		hdunum=0;
	      }
	    }
	  }

	  /* cycle through the HDU's loading as appropriate */
	  for (hdu=0;hdu<image->hdunum;hdu++) {

	    if (hdu>0) if (fits_movrel_hdu(image->fptr,1,&hdutype,&status)) 
	      printerror(status);
	
	    if (fits_read_key_str(image->fptr,"DES_EXT",type,comment,&status)==
	      KEY_NO_EXIST) {
	      status=0; /* reset status flag */
	      if (!flag_quiet) printf("  **Warning:  No DES_EXT keyword found in %s\n",
	        image->name);
	      sprintf(type,"IMAGE"); 
	    }
	    if (!strcmp(type,"IMAGE")) flag_type=DES_IMAGE;
	    else if (!strcmp(type,"MASK") || !strcmp(type,"BPM")) flag_type=DES_MASK;
	    else if (!strcmp(type,"VARIANCE")) flag_type=DES_VARIANCE;
	    else {
	      printf("  Undefined image extension type %s for FITS file %s\n",type,
	      image->name);
	    }	 
	    if (flag_type==DES_IMAGE) {  /* standard image in this extension */
	      
	      /* read the data type, BSCALE and BZERO */
	      if (fits_read_key_lng(image->fptr,"BITPIX",&(image->bitpix),comment,&status))
	        printerror(status);	
	      if (!flag_quiet)  printf("        BITPIX   = %d",image->bitpix);
  
	      if (fits_read_key_lng(image->fptr,"BSCALE",&(image->bscale),comment,
	        &status)==KEY_NO_EXIST) status=0;
	      else if (!flag_quiet) printf("      BSCALE   = %d",image->bscale);
	  
	      if (fits_read_key_lng(image->fptr,"BZERO",&(image->bzero),comment,
	        &status)) status=0;
	      else if (!flag_quiet) printf("      BZERO    = %d",image->bzero);
	      
	      /* grab OBSTYPE if it is present */
	      if (fits_read_key_str(image->fptr,"OBSTYPE",obstype,
	        comment,&status)==KEY_NO_EXIST) {
	        status=0;
	      }

	      /* convert to lower case */
	      if (!strcmp(obstype,"OBJECT")) sprintf(obstype,"object");
	      if (!strcmp(obstype,"DOME FLAT")) sprintf(obstype,"dome flat");
	      if (!strcmp(obstype,"FLAT")) sprintf(obstype,"flat");
	      if (!strcmp(obstype,"ZERO")) sprintf(obstype,"zero");
	      if (!strcmp(obstype,"BIAS")) sprintf(obstype,"bias");
	
	      /* Read other noise parameters if this is raw image */
	      if (!strcmp(obstype,"object") || !strcmp(obstype,"dome flat")
		|| !strcmp(obstype,"flat") || !strcmp(obstype,"zero")
		|| !strcmp(obstype,"bias")) {
	        /* grab SATURATA and SATURATB if they are present */
	        if (fits_read_key_flt(image->fptr,"SATURATA",&(image->saturateA),
	          comment,&status)==KEY_NO_EXIST) {
	          status=0;
	          if (fits_read_key_flt(image->fptr,"SATURAT0",&(image->saturateA),
	            comment,&status)==KEY_NO_EXIST) {
	            status=0;
	            image->saturateA=0.0;
	          }
	        }
	        if (fits_read_key_flt(image->fptr,"SATURATB",&(image->saturateB),
	          comment,&status)==KEY_NO_EXIST) {
	          status=0;
	          if (fits_read_key_flt(image->fptr,"SATURAT1",&(image->saturateB),
	            comment,&status)==KEY_NO_EXIST) {
	            status=0;
	            image->saturateB=0.0;
	          }
	        }
	        
	        /* grab GAINA and GAINB if they are present */
	        if (fits_read_key_flt(image->fptr,"GAINA",&(image->gainA),
	          comment,&status)==KEY_NO_EXIST) {
	          status=0;
	          image->gainA=0.0;
	        }
	        if (fits_read_key_flt(image->fptr,"GAINB",&(image->gainB),
	          comment,&status)==KEY_NO_EXIST) {
	          status=0;
	          image->gainB=0.0;
	        }
	        
	        /* grab RDNOISEA and RDNOISEB if they are present */
	        if (fits_read_key_flt(image->fptr,"RDNOISEA",&(image->rdnoiseA),
	          comment,&status)==KEY_NO_EXIST) {
	          status=0;
	          image->rdnoiseA=0.0;
	        }
	        if (fits_read_key_flt(image->fptr,"RDNOISEB",&(image->rdnoiseB),
	          comment,&status)==KEY_NO_EXIST) {
	          status=0;
	          image->rdnoiseB=0.0;
	        }
	        if (!flag_quiet) {
	          printf("\n      OBSTYPE: %s\n",obstype);
	          printf("      AMP A: Saturate= %.1f Rdnoise=%.2f Gain=%.3f\n",
		    image->saturateA,image->rdnoiseA,image->gainA);
	          printf("      AMP B: Saturate= %.1f Rdnoise=%.2f Gain=%.3f\n",
		    image->saturateB,image->rdnoiseB,image->gainB);
	        }
    
	      }
	      else { /* not raw data */
	        if (!flag_quiet) printf("\n    OBSTYPE: %s\n",obstype);
	        /* see if there is a SATURATE keyword */
	        if (fits_read_key_flt(image->fptr,"SATURATE",&saturate,
	          comment,&status)==KEY_NO_EXIST) {
	          status=0;
	        }
		else if (!flag_quiet) 
	          printf("      CCD: Saturate= %.1f\n",saturate);
	      }
	      image->image=(float *)calloc(image->npixels,sizeof(float));
	      status=0;
              if (fits_read_img(image->fptr,TFLOAT,image->fpixel,image->npixels,
	        &image->nullval,image->image,&anynull,&status)) 
		printerror(status);
	      
	      if (!flag_quiet) printf("    Read IMAGE extension\n");
	    }
	    if (flag_type==DES_MASK) {  /* image mask in this extension */
	      image->mask=(short *)calloc(image->npixels,sizeof(short));
              if (fits_read_img(image->fptr,TUSHORT,image->fpixel,
		image->npixels,
	        &image->shnullval,image->mask,&anynull,&status)) 
	  	printerror(status);  
	      if (!flag_quiet) printf("    Read MASK extension\n");
	    }
	    if (flag_type==DES_VARIANCE) {  /* image mask in this extension */
	      image->varim=(float *)calloc(image->npixels,sizeof(float));
	      status=0;
              if (fits_read_img(image->fptr,TFLOAT,image->fpixel,image->npixels,
	        &image->nullval,image->varim,&anynull,&status)) 
		printerror(status);
	      if (!flag_quiet) printf("    Read VARIANCE extension\n");
	    }
	  } /* end of HDU loop */
	  if (flag_nonmef) 
	    if (fits_close_file(image->fptr,&status)) {
	      printf("   ** Current image not closed\n");
	      exit(0);
	    }
	  /* move on to the next image (if there is one ) */
	  imcount++;
	} /* end of imcount loop */
        if (flag_nonmef) if (fits_open_file(&image->fptr,currentimage,
	  mode,&status)) {
	    printf("  ** Could not reopen %s\n",currentimage);
	    exit(0);
	  }
}

void printerror(status)
        int status;
{
	printf("  **** FITS Error ****\n");
        fits_report_error(stderr,status);
        exit(0);
}


/* retrieve basic image information from header */
void readimsections(data,flag_quiet)
	desimage *data;
	int flag_quiet;
{
	int	status,hdu;
	char	comment[200];
	void	decodesection();
	
	
	  /* get the BIASSEC information */
	  if (fits_read_key_str((data->fptr),"BIASSECA",(data->biasseca),
	    comment,&status)==KEY_NO_EXIST) {
	    printf("  Keyword BIASSECA not defined in %s\n",data->name);
	    printerror(status);
	  }
	  if (!flag_quiet) printf("    BIASSECA = %s\n",(data->biasseca));	  
	  decodesection((data->biasseca),(data->biassecan),flag_quiet);
	
	  /* get the AMPSEC information */
	  if (fits_read_key_str(data->fptr,"AMPSECA",data->ampseca,comment,
	    &status) ==KEY_NO_EXIST) {
	    printf("  Keyword AMPSECA not defined in %s\n",data->name);
	    printerror(status);
	  }
	  if (!flag_quiet) printf("    AMPSECA  = %s\n",data->ampseca);	  
	  decodesection(data->ampseca,data->ampsecan,flag_quiet);
	
	  /* get the BIASSEC information */
	  if (fits_read_key_str(data->fptr,"BIASSECB",data->biassecb,comment,
	    &status)==KEY_NO_EXIST) {
	    printf("  Keyword BIASSECB not defined in %s\n",data->name);
	    printerror(status);
	  }
	  if (!flag_quiet) printf("    BIASSECB = %s\n",data->biassecb);	  
	  decodesection(data->biassecb,data->biassecbn,flag_quiet);
	
	  /* get the AMPSEC information */
	  if (fits_read_key_str(data->fptr,"AMPSECB",data->ampsecb,comment,
	    &status)==KEY_NO_EXIST) {
	    printf("  Keyword AMPSECB not defined in %s\n",data->name);
	    printerror(status);
	  }
	  if (!flag_quiet) printf("    AMPSECB  = %s\n",data->ampsecb);	  
	  decodesection(data->ampsecb,data->ampsecbn,flag_quiet);

	  /* get the TRIMSEC information */
	  if (fits_read_key_str(data->fptr,"TRIMSEC",data->trimsec,comment,
	    &status) ==KEY_NO_EXIST) {
	    printf("  Keyword TRIMSEC not defined in %s\n",data->name);
	    printerror(status);
	  }
	  if (!flag_quiet) printf("    TRIMSEC  = %s\n",data->trimsec);	  
	  decodesection(data->trimsec,data->trimsecn,flag_quiet);

	  /* get the DATASEC information */
	  if (fits_read_key_str(data->fptr,"DATASEC",data->datasec,comment,
	    &status) ==KEY_NO_EXIST) {
	      printf("  Keyword DATASEC not defined in %s\n",
		data->name);
	      printerror(status);
	  }
	  if (!flag_quiet) printf("    DATASEC  = %s\n",data->datasec);	  
	  decodesection(data->datasec,data->datasecn,flag_quiet);
}



/* decode section string */
void decodesection(name,numbers,flag_quiet)
	char name[];
	int numbers[],flag_quiet;
{
	int i,len;
	
	len=strlen(name);
	for (i=0;i<len;i++) {
	  if (strncmp(&name[i],"0",1)<0 || strncmp(&name[i],"9",1)>0) name[i]=32; 
	} 
	sscanf(name,"%d %d %d %d",numbers,numbers+1,numbers+2,numbers+3);
}


void mkdecstring(dec,answer,decd,decm,decs)
  float dec,*decs;
  int *decd,*decm;
  char answer[];
{
	if (dec>90.0 || dec<-90.0) {
	  printf("** Declination out of bounds:  %8.4f\n",dec);
	  exit(0);
	}
	*decd=(int)fabs(dec);
	*decm=(int)((fabs(dec)-(*decd))*60.0);
	*decs=((fabs(dec)-(*decd))*3600.0-(*decm)*60.0);
	if (dec<0.0) (*decd)*=-1;
	sprintf(answer,"%03d:%02d:%04.1f",*decd,*decm,*decs);
}

void mkrastring(ra,answer,rah,ram,ras)
  float ra,*ras;
  int *rah,*ram;
  char answer[];
{
	if (ra>=360.0) ra-=360.0;
	if (ra<0.0) ra+=360.0;
	ra/=15.0;  /* convert from degrees to hours */
	*rah=(int)ra;
	*ram=(int)((ra-(*rah))*60.0);
	*ras=((ra-(*rah))*3600.0-(*ram)*60.0);
	sprintf(answer,"%02d:%02d:%04.1f",*rah,*ram,*ras);

}

double raconvert(rastring,rah,ram,ras)
	char rastring[];
	int *rah,*ram;
	float *ras;
{
	int	i,len;
	double	val;
	char	tmp[200];
	
	sprintf(tmp,"%s",rastring);
	len=strlen(rastring);
	for (i=0;i<len;i++) 
	  if (!strncmp(&(tmp[i]),":",1) || !strncmp(&(tmp[i]),"'",1)) tmp[i]=32;
	sscanf(tmp,"%d %d %f",rah,ram,ras);
	val=(*rah)+(*ram)/60.0+(*ras)/3600.0;
	val*=15.0;
	if (VERBOSE) printf("%s %f\n",tmp,val);
	return(val);
}

double decconvert(decstring,decd,decm,decs)
	char decstring[];
	int *decd,*decm;
	float *decs;
{
	int	i,len;
	double	val;
	char	tmp[200];
	
	sprintf(tmp,"%s",decstring);
	len=strlen(decstring);
	for (i=0;i<len;i++) 
	  if (!strncmp(&(tmp[i]),":",1) || !strncmp(&(tmp[i]),"'",1)) tmp[i]=32;
	sscanf(tmp,"%d %d %f",decd,decm,decs);
	if ((*decd)>=0.0) val=(*decd)+(*decm)/60.0+(*decs)/3600.0;
	else val=(*decd)-(*decm)/60.0-(*decs)/3600.0;
	if (VERBOSE) printf("%s  %f\n",tmp,val);
	return(val);
}

void headercheck(image,filter,ccdnum,keyword)
	desimage *image;
	char filter[],keyword[];
	long ccdnum;
{

	char	comment[1000],value[1000],newfilter[1000]="";
	long	newccdnum;
	int	status;
	void	printerror();

	/* check test keyword to confirm imagetype  */
	if (strcmp(keyword,"NOCHECK")) {
	  if (fits_read_keyword(image->fptr,keyword,value,comment,&status)==
	    KEY_NO_EXIST) {
	    printf("  ** Image %s does not have %s keyword **\n",
	      image->name,keyword);
	    exit(0);
	  }
	}

	/* check the FILTER keyword value */
	if (strcmp(filter,"NOCHECK")) {
	  if (fits_read_key_str(image->fptr,"FILTER",newfilter,comment,
	    &status)==KEY_NO_EXIST) {
	    printf("  ** Image %s does not have FILTER keyword **\n",
	      image->name);
	    /*exit(0);*/
	    status=0;
	  }

	  /* clean up filter */
	  /* simply transfer filter if it hasn't already been read */
	  if (!strlen(filter)) sprintf(filter,"%s",newfilter);
	  else {
	    if (strncmp(filter,newfilter,strlen(filter))) {
	      printf("  ** Image %s FILTER %s != current filter %s\n",
	        image->name,newfilter,filter);
	      /*exit(0);*/
	    status=0;
	    }
	  }
	}


	/* check the CCDNUM keyword value */
	if (fits_read_key_lng(image->fptr,"CCDNUM",&newccdnum,comment,&status)==
	  KEY_NO_EXIST) {
	  printf("  ** Image %s does not have CCDNUM keyword **\n",
	    image->name);
	  /*exit(0);*/
	  status=0;
	}
	/* simply transfer filter if it hasn't already been read */
	if (!ccdnum) ccdnum=newccdnum;
	else {
	  if (ccdnum!=newccdnum) {
	    printf("  ** Image %s CCDNUM %d does not match current ccdnum %d\n",
	      image->name,newccdnum,ccdnum);
	    /*exit(0);*/
	  }
	}
}


