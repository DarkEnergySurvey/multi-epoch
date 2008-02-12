/* This program will transform Mosaic2 images from the Blanco 4m into 
/* 8 FITS files, each
/* containing the imaging data from a single CCD. Initial WCS 
/* parameters are simply
/* copied directly from the original image header Mosaic2 images
*/
#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
	char	filename[500],nfilename[500],record[120],
		comment[1000],**hdumapping,rdfilename[200],
		longstring[2000],tr[1000],strrightascension[20],
	  	strdeclination[20],command[200],longcomment[200],
	        astfilename[200],nmstring[100],imagename[200],
		nimagename[200],
		filter[1000],obstype[1000],biassec[200],
	        date[100],
		ampname_a[1000],ampname_b[1000],
		detsec[8][30],ccdnumkeyword[100],framelong[200],frameid[200],
		ctype1[1000],ctype2[1000],trash[200],
		tag1[50],tag2[50];
	char	*zeroheader,**excl,exclist[10][10]={"SIMPLE",
		"BITPIX","NAXIS","EXTEND","NEXTEND","CHECKSUM","DATASUM",
		"CHECKVER"};
	int	nkeys,nexc=8;
	char	zerokeyval[100][1000],zerocomment[100][1000],
		zerokeyword[100][10]={"OBSERVAT","TELESCOP","TELRADEC",
		"TELRA","TELDEC","TELEQUIN","HA","ZD","AIRMASS",
		"TELFOCUS","DETECTOR","DETSIZE",
		"WEATDATE","WINDSPD","WINDDIR","AMBTEMP",
		"HUMIDITY","PRESSURE","OBSERVER","PROPOSER","PROPID",
		"OBJECT","FILTER",""};
	char	delkeys[100][10]={"RA","DEC","RADECEQ","CCDNAME",
		"CONTROLR","CONHWV","ARCONWD","ARCONGI","ARCONG",
		"BPM","CCDSIZE","CCDSEC","ATM1_1","ATM2_2","ATV1",
		"ATV2","LTM1_1","LTM2_2","LTV1","LTV2","DTM1_1",
		"DTM2_2","DTV1","DTV2","WCSASTRM","WAT0_001",
		"WAT1_001","WAT1_002","WAT1_003","WAT1_004",
		"WAT1_005","WAT2_001","WAT2_002","WAT2_003",
		"WAT2_004","WAT2_005","CHECKSUM","DATASUM","CHECKVER",
		"EXTVER","EXTNAME","PREFLASH","INHERIT","IMAGEID",
		"OBJECT",""};
	int	status=0,anynull,nfound,keysexist,morekeys,i,rah,ram,decd,decm,
		imtype,hdunum,x,y,hdutype,chdu,j,k,len,crpix=1,ltv,keynum,
		*hdultv,flag,wcsdim=2,mef_flag,chip_i,count,frameidlen,
		ccdnum,locin,locout,locin2;
	int	N_OBSERVAT=0,N_TELESCOP=1,N_TELRADEC=2,N_TELRA=3,N_TELDEC=4,
		N_TELEQUIN=5,N_HA=6,N_ZD=7,N_AIRMASS=8,N_TELFOCUS=9,
		N_DETECTOR=10,N_DETSIZE=11,N_WEATDATE=12,
		N_WINDSPD=13,
		N_WINDDIR=14,N_AMBTEMP=15,N_HUMIDITY=16,N_PRESSURE=17,
		N_OBSERVER=18,N_PROPOSER=19,N_PROPID=20,N_OBJECT=21;
	int	flag_crosstalk=0,flag_quiet=0,ext1,ext2,flag_fixim=0,
		flag_gzip=0;
	long	flag_phot=0;
	float	exptime,airmass=1.0,darktime,equinox=2000.0,
		ltv1,radecequinox=2000.0,*hdurdnoise,*hdugain,
	        ras,decs,rightascension,declination,ltm1=1.0,
	        equinoxkeyword,gain,rdnoise,
		gain_a,gain_b,rdnoise_a,rdnoise_b,saturate_a,saturate_b,
		significance,uncertainty,value;
	long	axes[2],naxes[2],taxes[2],pixels,npixels,fpixel,oldpixel,bscale,
		bzero,bitpix;
	float	nullval,*outdata,**indata,min,max;
	char	rakeyword[40],deckeyword[40];
	char	nite[200],runid[100],band[100],tilename[100],imagetype[100],
		imageclass[100];
	double	*hducrpix1,*hducrpix2,*hducrdpix1_1,*hducrdpix1_2,
		mjdobs=53392.8868,radouble,decdouble,
		*hducrdpix2_1,*hducrdpix2_2,
		cd1_1,cd1_2,cd2_1,cd2_2,crval1,crval2,crpix1,crpix2;
	double	raconvert(),decconvert(),ra,dec;
	float   xoff[63], yoff[63];
	char 	temp[200],airmasskeyword[200];
	
	void	printerror(),filename_resolve();
	fitsfile *fptr,*nfptr;
	FILE	*inp,*pip, *foffset;
	/* array for storing crosstalk coefficients */
	float	xtalk[16][16],fix_gain[16],fix_rdnoise[16],fix_saturate[16]; 

	if (argc<3) {
	  printf("%s <infile.fits> <root/outfile.fits> <options>\n",argv[0]);
          printf("  -crosstalk <crosstalk matrix- file>\n");
          printf("  -fixim <gain/rdnoise/saturate file>\n");
	  printf("  -photflag <0 or 1>\n");
	  printf("  -gzip\n");
          printf("  -quiet\n");
          exit(0);
	}
	sprintf(filename,"%s",argv[1]);
	sprintf(nfilename,"%s",argv[2]);

	mef_flag=0; /* write out individual files */

        /* process command line */
	/* first search through for quiet flag */
        for (i=3;i<argc;i++) {
          if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}
	/* now go through again more carefully */
        for (i=3;i<argc;i++) {
          if (!strcmp(argv[i],"-gzip")) flag_gzip=1;
	  if (!strcmp(argv[i],"-crosstalk")) {
	    flag_crosstalk=1;
	    for (j=0;j<16;j++) for (k=0;k<16;k++) xtalk[j][k]=0.0;
	    i++;
	    /* read through crosstalk matrix file */
	    if (!flag_quiet) printf("  Reading file %s\n",argv[i]);
	    inp=fopen(argv[i],"r");
	    while (fgets(trash,200,inp)!=NULL) {
	      if (!strncmp(trash,"im",2)) {
	        /* first replace parentheses with spaces */
		for (j=0;j<strlen(trash);j++) 
		  if (!strncmp(&(trash[j]),"(",1) || 
		  !strncmp(&(trash[j]),")",1)) trash[j]=32; 
	        sscanf(trash,"%s %s %f %f %f",tag1,tag2,
		  &value,&uncertainty,&significance);
	        sscanf(&(tag1[2]),"%d",&ext1);
	        sscanf(&(tag2[2]),"%d",&ext2);
	        xtalk[ext1-1][ext2-1]=value;
	      }
	    }
	    fclose(inp);
	    if (!flag_quiet) 
	      for (j=0;j<16;j++) {
	        printf("  j=%2d  ",j);
	        for (k=0;k<16;k++) {
		  printf("  %7.5f",xtalk[j][k]);
		}
		printf("\n");
	      }
	  }
	  if (!strcmp(argv[i],"-fixim")) {
	    flag_fixim=1;
	    i++;j=0;
	    inp=fopen(argv[i],"r");
	    while (fgets(trash,200,inp)!=NULL) {
	      if (strncmp(trash,"#",1)) {
	        sscanf(trash,"%s %f %f %f",tr,fix_gain+j,
		  fix_rdnoise+j,fix_saturate+j);
		j++;
	      }
	    }
	    fclose(inp);
	  }
	  if (!strcmp(argv[i],"-photflag")) 
	    sscanf(argv[++i],"%ld",&flag_phot);
	}

	/* write out the image name */
	if (!flag_quiet) printf("Mosaic2_convert:  converting %s\n",filename);

	/* extract nite and imagetype from the name */
	filename_resolve(filename,imageclass,runid,nite,tilename,
	  imagetype,imagename,band,&ccdnum);
	
        if (!flag_quiet)
          printf("%s\n imageclass=%s runid=%s nite=%s band=%s tilename=%s imagetype=%s ccdnum=%d imagename=%s\n", 
	  filename,imageclass,runid,nite,band,tilename,imagetype,ccdnum,
	    imagename);

        /* open the file */
        if (fits_open_file(&fptr,filename,READONLY,&status)) printerror(status);
	if (!flag_quiet) printf("  Opened %s\n",filename);	
		
	/* get the number of HDUs in the image */
	if (fits_get_num_hdus(fptr,&hdunum,&status)) printerror(status);
	if (!flag_quiet) printf("  %s has %d HDUs\n",filename,hdunum);
	if (hdunum!=17) {
	  printf(" *** Not standard Mosaic2 format ***\n");
	  exit(0);
	}
	
	/* allocate vector to hold 16 images */
	indata=(float **)calloc(hdunum-1,sizeof(float *));
	  
	/* now read all image data into memory */
	/* this enables crosstalk correction if requested */
	for (i=0;i<hdunum-1;i++) {

	  /* jump forward one HDU because the 0th extension contains */
	  /* only header info */
	  if (fits_movrel_hdu(fptr,1,&hdutype,&status)) printerror(status);
	  fits_get_hdu_num(fptr,&chdu);	

          /* read the NAXIS1 and NAXIS2 keyword to get image size */
          if (fits_read_keys_lng(fptr,"NAXIS",1,2,axes,&nfound,&status))
            printerror(status);
	  if (!flag_quiet) printf("  %d X %d image\n",axes[0],axes[1]);

	  /* read the data type, BSCALE and BZERO */
	  if (fits_read_key_lng(fptr,"BITPIX",&bitpix,comment,&status))
	    printerror(status);
	  if (!flag_quiet) printf("  BITPIX   = %d\n",bitpix);

	  if (fits_read_key_lng(fptr,"BSCALE",&bscale,comment,&status))
	    printerror(status);
	  if (!flag_quiet) printf("  BSCALE   = %d\n",bscale);
	
	  if (fits_read_key_lng(fptr,"BZERO",&bzero,comment,&status))
	    printerror(status);
	  if (!flag_quiet) printf("  BZERO    = %d\n",bzero);
	
	  if (bitpix==SHORT_IMG && bscale==1 && bzero==32768) {
	    imtype=USHORT_IMG;
	    if (!flag_quiet) 
	      printf("  %s[%d] contains image (%dX%d) of type Unsigned Short\n",
	      filename,chdu,axes[0],axes[1]);
	  }
	  else {printf("Only set up to read unsigned short images\n");exit(0);}

          /* prepare to read the image */
          pixels  = axes[0]*axes[1];
          fpixel   = 1; nullval  = 0;
	  if (i) {
	    if (pixels!=oldpixel) {
	      printf("  Image extensions have different sizes:  %d  vs %d\n",
	        pixels,oldpixel);
	      exit(0);
	    }
	  }
	  else oldpixel=pixels;

	  indata[i]=(float *)calloc(pixels,sizeof(float));
	  
          /* read the CHDU image  */
          if (fits_read_img(fptr,TFLOAT,fpixel,pixels,&nullval,
            indata[i],&anynull,&status)) printerror(status);

	  if (!flag_quiet) printf("  Read image data from CHDU=%d of %s\n",
	    chdu,filename);
  
	}  
	
	/* now move back to the 1st header data unit */
	/* jump back to 1st HDU because the 1st extension contains only */
	/* header info */
	if (fits_movabs_hdu(fptr,1,&hdutype,&status)) printerror(status);
	fits_get_hdu_num(fptr,&chdu);	

        /* read 0th header into a string */
        excl=(char **)calloc(nexc,sizeof(char *));
        for (j=0;j<nexc;j++) {
          excl[j]=(char *)calloc(10,sizeof(char));
          sprintf(excl[j],"%s",exclist[j]);
        }
        if (fits_hdr2str(fptr,1,excl,nexc,&zeroheader,&nkeys,&status))
          printerror(status);
        if (!flag_quiet) printf("  Read %d header keywords from 0th header\n",nkeys);

	
	ra=dec=radecequinox=0.0; /* set up defaults */
	/* get parameters we need for the FILES database table. */
       	if (fits_read_key_str(fptr,"DATE-OBS",date,comment,&status)
	  ==KEY_NO_EXIST) {
	  printf("\n  **Warning:  DATE-OBS not found in %s\n",filename);
	  status=0;
	}
	else if (!flag_quiet) printf("  DATE-OBS:  %s",date);

	if (fits_read_key_str(fptr,"OBSTYPE",imagetype,comment,&status)
	  ==KEY_NO_EXIST) {
	  printf("\n  **Warning:  OBSTYPE not found in %s\n",filename);
	  status=0;
	}
	else if (!flag_quiet) printf("  OBSTYPE=%s",imagetype);

	if (!strncmp(imagetype,"zero",4)) {  
	/* don't bother with FILTER, AIRMASS or Pointing for bias frames */
	  sprintf(filter,"none");
	  airmass=1.0;
	}
	else if (!strncmp(imagetype,"dome flat",9)) {  
	/* don't bother with AIRMASS or Pointing for dome flats */
	  airmass=1.0;
	  if (fits_read_key_str(fptr,"FILTER",filter,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  \n  **Warning:  FILTER not found %s\n",filter);
	    sprintf(filter,"unknown");
	    status=0;
	  }
	  /* just grab the first component of the filter name */
	  sscanf(filter,"%s",temp);
	  sprintf(filter,"%s",temp);
	  if (!flag_quiet) printf("  Filter=%s\n",filter);
	}
	else { /* get FILTER, AIRMASS and Pointing */
	  if (fits_read_key_flt(fptr,"AIRMASS",&airmass,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("\n  **Warning:  AIRMASS not found %.4f\n",airmass);
	    airmass=10.0;
	    status=0;
	  }
	  if (!flag_quiet) printf("  Z=%0.3f",airmass);
	  if (fits_read_key_str(fptr,"FILTER",filter,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  \n  **Warning:  FILTER not found %s\n",filter);
	    sprintf(filter,"unknown");
	    status=0;
	  }
	  /* just grab the first component of the filter name */
	  sscanf(filter,"%s",temp);
	  sprintf(filter,"%s",temp);
	  if (!flag_quiet) printf("  Filter=%s\n",filter);

	  if (fits_read_key_str(fptr,"TELRA",rakeyword,comment,&status)
	    ==KEY_NO_EXIST){
	    printf("  **Warning:  TELRA not found  %s\n",filename);
	    ra=0.0;
	    status=0;
          }
	  else ra=raconvert(rakeyword,&rah,&ram,&ras);

	  if (fits_read_key_str(fptr,"TELDEC",deckeyword,comment,&status)
	    ==KEY_NO_EXIST){
	     printf("  **Warning:  TELDEC not found  %s\n",filename);
	     dec=0.0;
	    status=0;
          }
	  else dec=decconvert(deckeyword,&decd,&decm,&decs);

	  if (fits_read_key_flt(fptr,"TELEQUIN",&radecequinox,comment,
	     &status)==KEY_NO_EXIST){
	     printf("  **Warning:  TELEQUIN not found  %s\n",filename);
	     equinox=0.0;
	    status=0;
          }
	  if (!flag_quiet) printf("  Pointing:  %0.7f  %0.7f %.0f\n",ra,dec,
	    equinox);
	}
	
	/* get exposure and darktime */
	if (fits_read_key_flt(fptr,"EXPTIME",&exptime,comment,&status)
	  ==KEY_NO_EXIST){
	  printf("  **Warning:  EXPTIME not found  %s\n",filename);
	  exptime=0.0;
	  status=0;
	}
	if (fits_read_key_flt(fptr,"DARKTIME",&darktime,comment,&status)
	  ==KEY_NO_EXIST){
	  printf("  **Warning:  DARKTIME not found  %s\n",filename);
	  darktime=0.0;
	  status=0;
	}
	
	/* read additional 1st HDU header information that will be inserted */
	/* in all headers */
	for (keynum=0;keynum<100;keynum++) 
	  if (!strlen(zerokeyword[keynum])) break;
	if (!flag_quiet) printf("  Reading %d keywords from 1st HDU\n",keynum);
	for (i=0;i<keynum;i++) {
	  if (fits_read_key_str(fptr,zerokeyword[i],zerokeyval[i],
	    zerocomment[i],&status)==KEY_NO_EXIST) {
	    printf("  **Warning:  %s not found\n",zerokeyword[i]);
	    sprintf(zerokeyval[i],"");
	    status=0;
	  }
	}
			
	/* create a single image array that fits a full CCD- */
	/* contains two AMP sections */
	/* this will contain 2048 imaging region plus two 50 pixel */
	/* overscan sections */
	naxes[0]=2148;naxes[1]=4096;
	npixels=naxes[0]*naxes[1];
	outdata=(float *)calloc(npixels,sizeof(float));

	/*strip .fits and any prefix off the input filename*/
	for (i=0;i<strlen(filename);i++) 
	  if (!strncmp(&filename[i],".fit",4)) filename[i]=0;
	for (i=strlen(filename)-1;i>=0;i--) 
	  if (!strncmp(&filename[i],"/",1)) {
	  i++;
	  for (j=i;j<=strlen(filename);j++) filename[j-i]=filename[j];
	  break;
	}
	if (!flag_quiet) printf("  Input filename root is %s\n",filename);
	
	/* set up DETSEC keyword values -- used by mscdisplay */
	sprintf(detsec[0],"[1:2048,1:4096]");
	sprintf(detsec[1],"[2049:4096,1:4096]");
	sprintf(detsec[2],"[4097:6144,1:4096]");
	sprintf(detsec[3],"[6145:8192,1:4096]");
	sprintf(detsec[4],"[1:2048,4097:8192]");
	sprintf(detsec[5],"[2049:4096,4097:8192]");
	sprintf(detsec[6],"[4097:6144,4097:8192]");
	sprintf(detsec[7],"[6145:8192,4097:8192]");
	
	/* jump to 2nd HDU because the 1st extension contains */
	/* only header info */
	if (fits_movabs_hdu(fptr,2,&hdutype,&status)) printerror(status);	

	/* now cycle through the HDU's parsing properly and */
	/* writing useful FITS file */
	for (i=2;i<hdunum;i+=2) {
	  
	  fits_get_hdu_num(fptr,&chdu);
	  if (!flag_quiet) printf("  Currently located at HDU %d of %d\n",
	    chdu,hdunum);
	  if (chdu!=i) {
	    printf("*****Not located at correct HDU (%d instead of %d)\n",
	    chdu,i);
	    exit(0);
	  }

	  /* open new image */
	  if (flag_gzip) sprintf(nfilename,"!%s_%02d.fits.gz",argv[2],i/2);
	  else sprintf(nfilename,"!%s_%02d.fits",argv[2],i/2);
	  if (fits_create_file(&nfptr,nfilename,&status)) printerror(status);
	  if (!flag_quiet) printf("  Opened image %s for CCD %d\n",
	    nfilename+1,i/2);
	  
	  /* copy the header from the CHDU in the input image */
	  if (fits_copy_header(fptr,nfptr,&status)) printerror (status);
	  if (!flag_quiet) 
	    printf("  Copied header from %s[%i] into new image\n",filename,i);
	  /* resize the image */
	  if (fits_resize_img(nfptr,FLOAT_IMG,2,naxes,&status)) 
	    printerror(status);
	  
	  /* change BZERO */
	  if (fits_set_bscale(nfptr,1.0,0.0,&status)) printerror(status);
	  bzero=0;
	  if (fits_update_key_lng(nfptr,"BZERO",bzero,comment,
	    &status)) printerror(status);
	  
	  /* copy zero header information into the new header */
	  /* note that last keyword is null keyword and truncates the header! */
	  /* only copy the headers that live within zerokeyword */
	  for (j=0;j<nkeys-1;j++) {
	    keynum=0;
	    while (strlen(zerokeyword[keynum])) {
	      if (!strncmp(zerokeyword[keynum],zeroheader+j*80,
		strlen(zerokeyword[keynum]))) {
	        if (fits_write_record(nfptr,zeroheader+j*80,&status))
	          printerror(status);
		break;
	      }
	      keynum++;
	    }
	  }
	    
	    
	  /* read the WCS information */
	  if (strcmp(imagetype,"zero") && strcmp(imagetype,"dome flat")) {
	    if (fits_read_key_str(fptr,"CTYPE1",ctype1,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      ctype1[0]=0;
	    }
	    sprintf(ctype1,"RA---TAN");
	    if (fits_read_key_str(fptr,"CTYPE2",ctype2,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      ctype2[0]=0;
	    }
	    sprintf(ctype2,"DEC--TAN");
	    if (fits_read_key_flt(fptr,"EQUINOX",&equinox,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      equinox=0.0;
	    }	
	    if (fits_read_key_dbl(fptr,"CRVAL1",&crval1,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      crval1=0.0;
	    }	        
	    if (fits_read_key_dbl(fptr,"CRVAL2",&crval2,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      crval2=0.0;
	    }
	    if (fits_read_key_dbl(fptr,"CRPIX1",&crpix1,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      crpix1=0.0;
	    }
	    if (fits_read_key_dbl(fptr,"CRPIX2",&crpix2,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      crpix2=0.0;
	    }
	    if (fits_read_key_dbl(fptr,"CD1_1",&cd1_1,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      cd1_1=0.0;
	    }
	    if (fits_read_key_dbl(fptr,"CD1_2",&cd1_2,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      cd1_2=0.0;
	    }
	    if (fits_read_key_dbl(fptr,"CD2_1",&cd2_1,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      cd2_1=0.0;
	    }
	    if (fits_read_key_dbl(fptr,"CD2_2",&cd2_2,comment,&status)
	      ==KEY_NO_EXIST) {
	      status=0;
	      cd2_2=0.0;
	    }
	  }	  

	  /* read the AMP-specific information- GAIN, RDNOISE, SATURATE */
	  if (fits_read_key_flt(fptr,"GAIN",&gain_a,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  **Warning:  GAIN_A not found\n");
	    gain_a=0.0;
	    status=0;
	  }
	  if (fits_read_key_flt(fptr,"RDNOISE",&rdnoise_a,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  **Warning:  RDNOISE_A not found\n");
	    rdnoise_a=0.0;
	    status=0;
	  }
	  if (fits_read_key_flt(fptr,"SATURATE",&saturate_a,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  **Warning:  SATURATE_A not found\n");
	    saturate_a=65535.0;
	    status=0;
	  }
	  if (fits_read_key_str(fptr,"AMPNAME",ampname_a,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  **Warning:  AMPNAME_A not found\n");
	    sprintf(ampname_a,"");
	    status=0;
	  }
	  if (!flag_quiet) 
	    printf("  Amp %s: Gain (%.1f) RdNoise (%.1f) Saturation (%.0f)\n",
	    ampname_a,gain_a,rdnoise_a,saturate_a);
		  	  
	  /* move forward one HDU */
	  if (fits_movrel_hdu(fptr,1,&hdutype,&status)) printerror(status);
	  fits_get_hdu_num(fptr,&chdu);    

	  /* read the AMP-specific information- GAIN, RDNOISE, SATURATE */
	  if (fits_read_key_flt(fptr,"GAIN",&gain_b,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  **Warning:  GAIN_B not found\n");
	    gain_b=0.0;
	    status=0;
	  }
	  if (fits_read_key_flt(fptr,"RDNOISE",&rdnoise_b,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  **Warning:  RDNOISE_B not found\n");
	    rdnoise_b=0.0;
	    status=0;
	  }
	  if (fits_read_key_flt(fptr,"SATURATE",&saturate_b,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  **Warning:  SATURATE_B not found\n");
	    saturate_b=65535.0;
	    status=0;
	  }
	  if (fits_read_key_str(fptr,"AMPNAME",ampname_b,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("  **Warning:  AMPNAME_B not found\n");
	    sprintf(ampname_b,"");
	    status=0;
	  }
	  if (!flag_quiet) 
	    printf("  Amp %s: Gain (%.1f) RdNoise (%.1f) Saturation (%.0f)\n",
	    ampname_b,gain_b,rdnoise_b,saturate_b);
	    
	  /* replace the gain/rdnoise/saturate params if required */
	  if (flag_fixim) {
	    gain_a=fix_gain[i-2];
	    rdnoise_a=fix_rdnoise[i-2];
	    saturate_a=fix_saturate[i-2];
	    gain_b=fix_gain[i-1];
	    rdnoise_b=fix_rdnoise[i-1];
	    saturate_b=fix_saturate[i-1];	  
	  }
	    	    	  
	  /* copy the images into the new image array*/
	  /* remember that for i=2 the image number is 0 */
	  k=i-2;
	  for (y=0;y<naxes[1];y++) for (x=0;x<naxes[0];x++) {
	    locout=y*naxes[0]+x;
	     /* copy overscan region for first amplifier */
	    if (x<50) {
	      locin=y*axes[0]+x+1062;
	      locin2=y*axes[0]+(axes[0]-(x+1062)-1);
	      outdata[locout]=indata[k][locin];
	      if (flag_crosstalk)   {/* apply crosstalk correction */
	        for (j=0;j<16;j+=2) outdata[locout]-=
		  (xtalk[k][j]*indata[j][locin]+xtalk[k][j+1]*
		  indata[j+1][locin2]);
	      }
	    }
	    /* copy data from first amplifier */
	    else if (x<1074) {
	      locin=y*axes[0]+(x-50)+24;
	      locin2=y*axes[0]+(axes[0]-((x-50)+24)-1);
	      outdata[locout]=indata[k][locin];
	      if (flag_crosstalk)   /* apply crosstalk correction */
	        for (j=0;j<16;j+=2) outdata[locout]-=
		  (xtalk[k][j]*indata[j][locin]+xtalk[k][j+1]*
		  indata[j+1][locin2]);
	    }
	    /* copy data from second amplifier */
	    else if (x<2098) {
	      locin=y*axes[0]+(x-1074)+64;
	      locin2=y*axes[0]+(axes[0]-((x-1074)+64)-1);
	      outdata[locout]=indata[k+1][locin];
	      if (flag_crosstalk)   /* apply crosstalk correction */
	        for (j=0;j<16;j+=2) outdata[locout]-=
		  (xtalk[k+1][j]*indata[j][locin2]+xtalk[k+1][j+1]*
		  indata[j+1][locin]);
	    }
	    /* copy overscan region for second amplifier */
	    else {
	      locin=y*axes[0]+(x-2098);
	      locin2=y*axes[0]+(axes[0]-(x-2098)-1);
	      outdata[locout]=indata[k+1][locin];
	      if (flag_crosstalk)   /* apply crosstalk correction */
	        for (j=0;j<16;j+=2) outdata[locout]-=
		  (xtalk[k+1][j]*indata[j][locin2]+xtalk[k+1][j+1]*
		  indata[j+1][locin]);
	    }
	  }
	  	  
	  /* write the new image and copy the header */
	  if (fits_write_img(nfptr,TFLOAT,1,npixels,outdata,&status))
	    printerror(status);
	  if (!flag_quiet) printf("  Wrote image data to %s\n",nfilename+1);
	  
	  /* check for AMPNAME header keyword; delete and add */
	  /* AMPNAMEA and AMPNAMEB */
	  if (fits_read_keyword(nfptr,"AMPNAME",comment,comment,&status)
	    ==KEY_NO_EXIST) status=0;
	    /* add CCDNUM keyword */
	    ccdnum=i/2;
	    if (fits_insert_key_lng(nfptr,"CCDNUM",ccdnum,"CCD number (1-8)",
	      &status)) printerror(status);
	    /* now do AMPNAME */
	    if (fits_insert_key_str(nfptr,"AMPNAMEA",ampname_a,
	      "Amplifier name",&status)) printerror(status);
	    if (fits_insert_key_str(nfptr,"AMPNAMEB",ampname_b,
	      "Amplifier name",&status)) printerror(status);
	    if (fits_delete_key(nfptr,"AMPNAME",&status)) {
	      printf("  **Warning:  AMPNAME not deleted in %s\n",filename);
	      status=0;
	    }
	  if (!flag_quiet) printf("  Writing:  AMPNAMEA/B");
	  

	  /* check for AMPSEC header keyword; delete and */
	  /* add AMPSECA and AMPSECB */
	  if (fits_read_keyword(nfptr,"AMPSEC",biassec,comment,&status)
	    ==KEY_NO_EXIST) status=0; 
	    if (fits_insert_key_str(nfptr,"AMPSECA","[1:1024,1:4096]",
	      "Section AMP A",&status)) printerror(status);
	    if (fits_insert_key_str(nfptr,"AMPSECB","[1025:2048,1:4096]",
	      "Section AMP B",&status)) printerror(status);
	    if (fits_delete_key(nfptr,"AMPSEC",&status)) {
	      printf("  **Warning:  AMPSEC not deleted in %s\n",filename);
	      status=0;
	    }
	  if (!flag_quiet) printf(", AMPSECA/B");

	  /* check for the BIASSEC keyword; delete and */
	  /* add  BIASSECA and BIASSECB*/
	  if (fits_read_keyword(nfptr,"BIASSEC",comment,comment,&count)
	    ==KEY_NO_EXIST) status=0;
	    if (fits_insert_key_str(nfptr,"BIASSECA","[1:50,1:4096]",
	      "OVERSCAN for AMP A",&status)) printerror(status);
	    if (fits_insert_key_str(nfptr,"BIASSECB","[2098:2148,1:4096]",
	      "OVERSCAN for AMP B",&status)) printerror(status);
	    if (fits_delete_key(nfptr,"BIASSEC",&status)) {
	      printf("  **Warning:  BIASSEC not deleted in %s\n",filename);
	      status=0;
	    }
	  if (!flag_quiet) printf(", BIASSECA/B");

	  /* check for the RDNOISE keyword; delete and */
	  /* add  RDNOISEA and RDNOISEB*/
	  if (fits_read_keyword(nfptr,"RDNOISE",comment,comment,&count)
	    ==KEY_NO_EXIST) status=0;
	    if (fits_insert_key_flt(nfptr,"RDNOISEA",rdnoise_a,1,
	      "Read Noise for AMP A",&status)) printerror(status);
	    if (fits_insert_key_flt(nfptr,"RDNOISEB",rdnoise_b,1,
	      "Read Noise for AMP B",&status)) printerror(status);
	    /* delete RDNOISE if it is present */
	    if (fits_delete_key(nfptr,"RDNOISE",&status)) {
	      printf("  **Warning:  RDNOISE not deleted in %s\n",filename);
	      status=0;
	    }
	  if (!flag_quiet) printf(", RDNOISEA/B");
	  
	  /* check for the GAIN keyword; delete and add  GAINA and GAINB*/
	  if (fits_read_keyword(nfptr,"GAIN",comment,comment,&count)
	    ==KEY_NO_EXIST) status=0;
	    if (fits_insert_key_flt(nfptr,"GAINA",gain_a,1,"Gain for AMP A",
	      &status)) printerror(status);
	    if (fits_insert_key_flt(nfptr,"GAINB",gain_b,1,"Gain for AMP B",
	      &status)) printerror(status);
	    /* delete GAIN if present */
	    if (fits_delete_key(nfptr,"GAIN",&status)) {
	      printf("  **Warning:  GAIN not deleted in %s\n",filename);
	      status=0;
	    }
	  if (!flag_quiet) printf(", GAINA/B");
	  	  
	  /* check for the SATURATE keyword; delete and */
	  /* add  SATURATEA and SATURATEB*/
	  if (fits_read_keyword(nfptr,"SATURATE",comment,comment,
	    &count)==KEY_NO_EXIST) status=0;
	    if (fits_insert_key_flt(nfptr,"SATURATA",saturate_a,0,
	      "Saturation for AMP A",&status)) printerror(status);
	    if (fits_insert_key_flt(nfptr,"SATURATB",saturate_b,0,
	      "Saturation for AMP B",&status)) printerror(status);
	    /* delete SATURATE if it's present */
	    if (fits_delete_key(nfptr,"SATURATE",&status)) {
	      printf("  **Warning:  SATURATE not deleted in %s\n",filename);
	      status=0;
	    }
	  if (!flag_quiet) printf(", SATURATEA/B");
	  	  
	  /* change DATASEC keyword value */
	  if (fits_update_key_str(nfptr,"DATASEC","[51:2098,1:4096]","",
	    &status)) printerror(status);  
	  if (!flag_quiet) printf(", DATASEC");
	  
	  /* change CTYPE1 keyword value */
	  if (fits_update_key_str(nfptr,"CTYPE1",ctype1,"Coordinate type",
	    &status)) printerror(status);  
	  if (!flag_quiet) printf(", CTYPE1");
	  
	  /* change CTYPE2 keyword value */
	  if (fits_update_key_str(nfptr,"CTYPE2",ctype2,"Coordinate type",
	    &status)) printerror(status);  
	  if (!flag_quiet) printf(", CTYPE2");
	  
	  /* change TRIMSEC keyword value */
	  if (fits_update_key_str(nfptr,"TRIMSEC","[51:2098,1:4096]","",
	    &status)) printerror(status);  
	  if (!flag_quiet) printf(", TRIMSEC");
	  	  
	  /* change DETSEC keyword value-- depends on extension */
	  if (fits_update_key_str(nfptr,"DETSEC",detsec[i/2-1],"",&status))
	    printerror(status);
	  if (!flag_quiet) printf(", DETSEC");
	  	    
	  /* read in LTV1 and use it to correct CRPIX1 */
	  if (fits_read_key_flt(nfptr,"LTV1",&ltv1,comment,&status)
	    ==KEY_NO_EXIST) {
	    printf("\n  **Warning:  LTV1 not found\n");
	    ltv1=0.0;
	    status=0;
	  }
	  /* now update CRPIX1 with correct value */
	  crpix1-=ltv1;
	  if (fits_update_key_dbl(nfptr,"CRPIX1",crpix1,10,comment,&status))
	    printerror(status);  
	  if (!flag_quiet) printf(", CRPIX1\n");
	  
	  /* Delete some unneeded keywords */
	  nkeys=0;
	  while(strlen(delkeys[nkeys])) {
	    if (fits_delete_key(nfptr,delkeys[nkeys],&status)) status=0;
	    nkeys++;
	  }
	  
	  /* Delete FILENAME keyword and rename to include image extension */
	  if (fits_read_keyword(nfptr,"OBSID",comment,comment,&count)
	    ==KEY_NO_EXIST) status=0;
	  /* want only the image name-- search for first occurence of "/" */
          for (j=strlen(nfilename);j>=0;j--) 
	     if (!strncmp(&(nfilename[j]),"/",1)) break;
	  sprintf(imagename,"%s",(nfilename+j+1));
	  if (fits_insert_key_str(nfptr,"FILENAME",imagename,"",&status)) 
	    printerror(status);
	  if (!flag_quiet) printf("  Wrote keyword FILENAME = %s\n",imagename);
	  
	  /* add in Photometricity Flag */
	  if (fits_update_key_lng(nfptr,"PHOTFLAG",flag_phot,
	    "Night Photometric (1) or not (0)",
	    &status)) printerror(status);
	    
	  if (flag_gzip) imagename[strlen(imagename)-11]=0;	  
	  else imagename[strlen(imagename)-8]=0;	  
	  if (!strncmp(imagename,"!",1)) sprintf(nimagename,"%s",imagename+1);
	  else sprintf(nimagename,"%s",imagename);
	  /* output database ingest line */
	  if (!strcmp(zerokeyval[N_TELESCOP],"CTIO 4.0 meter telescope")) 
	    sprintf(zerokeyval[N_TELESCOP],"Blanco 4m");
	  if (1) {
	    printf("%.6f|%.6f|%.2f|%s|%.1f|%.1f|%.1f|%.1f|%0.3f|%s|%s|%s|",
	      ra,dec,radecequinox,date,gain_a,rdnoise_a,gain_b,rdnoise_b,
	      airmass,filter,imageclass,imagetype);
	    printf("%s|%s|%d|%.3f|%.3f|",
	      nimagename,nite,ccdnum,exptime,darktime);
	    printf("%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|",
	      zerokeyval[N_OBJECT],
	      zerokeyval[N_OBSERVAT],zerokeyval[N_TELESCOP],zerokeyval[N_HA],
	      zerokeyval[N_ZD],zerokeyval[N_DETECTOR],zerokeyval[N_OBSERVER],
	      zerokeyval[N_PROPID],zerokeyval[N_WEATDATE],zerokeyval[N_WINDSPD],
	      zerokeyval[N_WINDDIR],zerokeyval[N_AMBTEMP],
	      zerokeyval[N_HUMIDITY],
	      zerokeyval[N_PRESSURE],"");
	    printf("%7.2f|%d|%s|%s|%16.12f|%16.12f|%10.4f|%10.4f",
	      equinox,wcsdim,ctype1,ctype2,crval1,crval2,crpix1,crpix2);
	    printf("|%19.13e|%19.13e|%19.13e|%19.13e|%d|%d|%d|%d\n",
	      cd1_1,cd2_1,cd1_2,cd2_2,axes[0],axes[1],1,flag_phot);
	  }
	  
	  /* leave history in the header */
	  pip=popen("date","r");
	  fgets(command,200,pip);
	  pclose(pip);
	  /* must remove the newline */
	  command[strlen(command)-1]=0;
	  if (fits_write_key_str(nfptr,"DESMScvt",command,
	    "Mosaic2 image conversion",&status)) printerror(status);
	  sprintf(longcomment,"DESDM:");
	  for (j=0;j<argc;j++) sprintf(longcomment,"%s %s",longcomment,argv[j]);
	  if (fits_write_comment(nfptr,longcomment,&status)) printerror(status);
	  if (!flag_quiet) printf("  %s\n",longcomment);
	  if (!flag_quiet) 
	    printf("  **Wrote image data and Header Keywords to CHDU of %s\n",
	    nfilename+1);
	  /* mark this as an image extension */
	  if (fits_write_key_str(nfptr,"DES_EXT","IMAGE","Image extension",
	    &status)) printerror(status);

          /* close the new image */
          if (fits_close_file(nfptr,&status)) printerror(status);
	  if (!flag_quiet) printf("  Closed image %s\n",nfilename+1);

	  /* Move to the next HDU */
	  if (i<hdunum-1) if (fits_movrel_hdu(fptr,1,&hdutype,&status)) 
	    printerror(status);

	}
	  
        /* close the image */
        if (fits_close_file(fptr,&status)) printerror(status);
	if (!flag_quiet) printf("  Closed image %s\n\n",filename);
	return(0);
}
