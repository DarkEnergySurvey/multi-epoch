/* DECam_convert <> <> ....
/*
/* This program will transform DECam images from the Blanco 4m into 62 FITS 
/* files, each containing the imaging data from a single CCD. 
/*
/* The program also makes the crosstalk correction.
*/
#include "imageproc.h"

#define DOUBLE 1
#define STRING 2
#define FLOAT  3
#define INTEG  4

#define NUM_KEYS 200

main(argc,argv)
	int argc;
	char *argv[];
{
	char	filename[500],trash[200],nfilename[500],tag1[100],tag2[100],
		comment[100],command[100],longcomment[10000];
		
	/*  ********************************************************  */
	/*  ************  List the Headers Keywords NOT to be ******  */
	/*  ************      copied from 0th Header          ******  */
	/*  ********************************************************  */
		
	char	*zeroheader,**exclkey,exclist[20][10]={"SIMPLE",
		"BITPIX","NAXIS","NAXIS1","NAXIS2","EXTEND","NEXTEND","CHECKSUM",
		"DATASUM","CHECKVER","XTENSION","GCOUNT","PCOUNT",
		"BZERO","BSCALE"},*nthheader;
	int	numzerokeys,nexc=15,numnthkeys,numwrittenkeys=0;
	
	/*  ********************************************************  */
	/*  ***************  Set up for db field reading ***********  */
	/*  ********************************************************  */
			
	char	dbfieldkey[NUM_KEYS][9],*dbfieldval[NUM_KEYS];
	int	num_dbfields,dbfieldtyp[NUM_KEYS],n_OBJECT,n_OBSTYPE,
		n_OBSERVER,n_PROPID,n_DETECTOR,n_TELESCOP,n_OBSERVAT,
		n_TELRA,n_TELDEC,n_TELEQUIN,n_HA,n_ZD,n_AIRMASS,n_FILTER,
		n_TELFOCUS,n_DATE_OBS,n_TIME_OBS,n_EXPTIME,n_DARKTIME,
		n_WINDSPD,n_WINDDIR,n_AMBTEMP,n_HUMIDITY,
		n_PRESSURE,n_SKYVAR, n_FLUXVAR,n_GAINA,n_RDNOISEA,
		n_SATURATA,n_GAINB,n_RDNOISEB,n_SATURATB,n_WCSDIM,
		n_EQUINOX,n_CTYPE1,n_CTYPE2,n_CRVAL1,n_CRVAL2,
		n_CRPIX1,n_CRPIX2,n_CD1_1,n_CD1_2,n_CD2_1,n_CD2_2,
		n_PV1[11],n_PV2[11];
	char	*rastring,*decstring;
	float	*telequin;
	double	ra,dec;

	/*  ******************************************************** */
	/*  ******** array for storing crosstalk coefficients ****** */
	/*  ******************************************************** */
	float	xtalk[124][124],fix_gain[62],fix_rdnoise[62],fix_saturate[62];

	
	
	int	status=0,anynull,nfound,i,rah,ram,decd,
		decm,imtype,hdunum,x,y,hdutype,chdu,j,k,len,crpix=1,
		ltv,keynum,totkeynum,*hdultv,flag,wcsdim=2,
		chip_i,count,frameidlen,keyflag,
		ccdnum,locin,locout,locin2,v,flag_pv=0,flag_cd=0;
	int	flag_crosstalk=0,flag_quiet=0,ext1,ext2,flag_bpm=0,
		flag_gzip=0,ampextension,ampA,ampB,locA,locB,ampoffset1,
		ampoffset2,bpmx,bpmy,grabkeywordval(),
		bpmlocA,bpmlocB,flag_phot=0;
	float	exptime,airmass=1.0,darktime,equinox=2000.0,
		ltv1,radecequinox=2000.0,*hdurdnoise,*hdugain,
	        ras,decs,rightascension,declination,ltm1=1.0,
	        equinoxkeyword,gain,rdnoise,
		gain_a,gain_b,rdnoise_a,rdnoise_b,saturate_a,saturate_b,
		significance,uncertainty,value;
	long	axes[2],naxes[2],taxes[2],pixels,npixels,fpixel,oldpixel,
		bscale,bzero,bitpix;
	float	nullval,*outdata,**indata,min,max;
	double	*hducrpix1,*hducrpix2,*hducrdpix1_1,*hducrdpix1_2,
		mjdobs=53392.8868,radouble,decdouble,
		*hducrdpix2_1,*hducrdpix2_2,xtalkcorrection,
		cd1_1,cd1_2,cd2_1,cd2_2,crval1,crval2,crpix1,crpix2;
	double	raconvert(),decconvert();
	float   xoff[63], yoff[63];
	char 	nite[200],runid[100],band[100],tilename[100],imagetype[100],
		newimagename[100],imagename[100],imageclass[100];
	char	tag[20];
	void	printerror(),filename_resolve(),rd_desimage(),decodesection();
	fitsfile *fptr,*nfptr;
	FILE	*inp,*pip, *foffset;


	if (argc<3) {
	  printf("%s <infile.fits> <root/outfile> <options>\n",argv[0]);
          printf("  -crosstalk <crosstalk matrix- file>\n");
	  printf("  -gzip\n");
	  printf("  -photflag <0 or 1>\n");
          printf("  -quiet\n");
          exit(0);
	}
	sprintf(filename,"%s",argv[1]);
	sprintf(nfilename,"%s",argv[2]);


	/* ****************************************************** */
	/* ****************  Process Command Line *************** */
	/* ****************************************************** */

        for (i=3;i<argc;i++) 
          if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
        for (i=3;i<argc;i++) {
          if (!strcmp(argv[i],"-gzip")) flag_gzip=1;
          if (!strcmp(argv[i],"-photflag"))
	    sscanf(argv[++i],"%d",&flag_phot);
	  if (!strcmp(argv[i],"-crosstalk")) {
	    flag_crosstalk=1;
	    for (j=0;j<124;j++) for (k=0;k<124;k++) xtalk[j][k]=0.0;
	    i++;
	    /* read through crosstalk matrix file */
	    if (!flag_quiet) printf("  Reading file %s\n",argv[i]);
	    inp=fopen(argv[i],"r");
	    if (inp==NULL) {
	      printf("**  Crosstalk file %s not found\n",argv[i]);
	      exit(0);
	    }
	    while (fgets(trash,200,inp)!=NULL) {
	      if (!strncmp(trash,"ccd",3) || !strncmp(trash,"CCD",3)) {
	        /* first replace parentheses and commas with spaces */
		for (j=0;j<strlen(trash);j++) 
		  if (!strncmp(&(trash[j]),"(",1) || 
		  !strncmp(&(trash[j]),")",1)) trash[j]=32; 
	        sscanf(trash,"%s %s %f %f %f",tag1,tag2,
		  &value,&uncertainty,&significance);
		if (!strncmp(tag1+strlen(tag1)-1,"A",1)) ampoffset1=0;
		else if (!strncmp(tag1+strlen(tag1)-1,"B",1)) ampoffset1=1;
		else {
		  printf("  Amp not properly noted as A/B in %s\n",tag1);
		  exit(0);
		}
		if (!strncmp(tag2+strlen(tag2)-1,"A",1)) ampoffset2=0;
		else if (!strncmp(tag2+strlen(tag2)-1,"B",1)) ampoffset2=1;
		else {
		  printf("  Amp not properly noted as A/B in %s\n",tag2);
		  exit(0);
		}
		/* strip off the A/B suffix */	
		tag1[strlen(tag1)-1]=0;tag2[strlen(tag2)-1]=0; 
		/* read in the ccd number */       
		sscanf(&(tag1[3]),"%d",&ext1);
	        sscanf(&(tag2[3]),"%d",&ext2);
		ext1=(ext1-1)*2+ampoffset1;
		if (ext1<0 || ext1>=124) {
		  printf("  CCD number out of range:  %s %d\n",tag1,ext1);
		  exit(0);
		}
		ext2=(ext2-1)*2+ampoffset2;
		if (ext2<0 || ext2>=124) {
		  printf("  CCD number out of range:  %s %d\n",tag2,ext2);
		  exit(0);
		}
	        xtalk[ext1][ext2]=value;
	      }
	    }
	    fclose(inp);
	    if (!flag_quiet) /* print the crosstalk coefficients */
	      for (j=0;j<124;j++) {
	        if (j%2==0) printf("  ccd%02dA",j/2+1);
		else printf("  ccd%02dB",j/2+1);
	        for (k=0;k<124;k++) {
		  printf("  %7.5f",xtalk[j][k]);
		}
		printf("\n");
	      }
	  }
	}

	/* **************************************************************** */
	/* ********  Set up data types and keywords for db Fields ********* */
	/* **************************************************************** */
	
	sprintf(dbfieldkey[i],"OBJECT  ");dbfieldtyp[i]=STRING;n_OBJECT=i++;	
	sprintf(dbfieldkey[i],"OBSTYPE ");dbfieldtyp[i]=STRING;n_OBSTYPE=i++;	
	sprintf(dbfieldkey[i],"OBSERVER");dbfieldtyp[i]=STRING;n_OBSERVER=i++;
	sprintf(dbfieldkey[i],"PROPID  ");dbfieldtyp[i]=STRING;n_PROPID=i++;
	sprintf(dbfieldkey[i],"DETECTOR");dbfieldtyp[i]=STRING;n_DETECTOR=i++;
	sprintf(dbfieldkey[i],"TELESCOP");dbfieldtyp[i]=STRING;n_TELESCOP=i++;
	sprintf(dbfieldkey[i],"OBSERVAT");dbfieldtyp[i]=STRING;n_OBSERVAT=i++;
	sprintf(dbfieldkey[i],"TELRA   ");dbfieldtyp[i]=STRING;n_TELRA=i++;
	sprintf(dbfieldkey[i],"TELDEC  ");dbfieldtyp[i]=STRING;n_TELDEC=i++;
	sprintf(dbfieldkey[i],"TELEQUIN");dbfieldtyp[i]=FLOAT; n_TELEQUIN=i++;
	sprintf(dbfieldkey[i],"HA      ");dbfieldtyp[i]=STRING;n_HA=i++;
	sprintf(dbfieldkey[i],"ZD      ");dbfieldtyp[i]=FLOAT; n_ZD=i++;
	sprintf(dbfieldkey[i],"AIRMASS ");dbfieldtyp[i]=FLOAT; n_AIRMASS=i++;	
	sprintf(dbfieldkey[i],"FILTER  ");dbfieldtyp[i]=STRING;n_FILTER=i++;	
	sprintf(dbfieldkey[i],"TELFOCUS");dbfieldtyp[i]=STRING;n_TELFOCUS=i++;

	sprintf(dbfieldkey[i],"DATE-OBS");dbfieldtyp[i]=STRING;n_DATE_OBS=i++;	
	sprintf(dbfieldkey[i],"EXPTIME ");dbfieldtyp[i]=FLOAT; n_EXPTIME=i++;	
	sprintf(dbfieldkey[i],"DARKTIME");dbfieldtyp[i]=FLOAT; n_DARKTIME=i++;	

	sprintf(dbfieldkey[i],"WINDSPD ");dbfieldtyp[i]=STRING;n_WINDSPD=i++;
	sprintf(dbfieldkey[i],"WINDDIR ");dbfieldtyp[i]=STRING;n_WINDDIR=i++;
	sprintf(dbfieldkey[i],"AMBTEMP ");dbfieldtyp[i]=STRING;n_AMBTEMP=i++;
	sprintf(dbfieldkey[i],"HUMIDITY");dbfieldtyp[i]=STRING;n_HUMIDITY=i++;
	sprintf(dbfieldkey[i],"PRESSURE");dbfieldtyp[i]=STRING;n_PRESSURE=i++;
	sprintf(dbfieldkey[i],"SKYVAR  ");dbfieldtyp[i]=FLOAT; n_SKYVAR=i++;
	sprintf(dbfieldkey[i],"FLUXVAR ");dbfieldtyp[i]=FLOAT; n_FLUXVAR=i++;
	
	sprintf(dbfieldkey[i],"GAINA   ");dbfieldtyp[i]=FLOAT; n_GAINA=i++;	
	sprintf(dbfieldkey[i],"RDNOISEA");dbfieldtyp[i]=FLOAT; n_RDNOISEA=i++;	
	sprintf(dbfieldkey[i],"SATURATA");dbfieldtyp[i]=FLOAT; n_SATURATA=i++;	
	sprintf(dbfieldkey[i],"GAINB   ");dbfieldtyp[i]=FLOAT; n_GAINB=i++;	
	sprintf(dbfieldkey[i],"RDNOISEB");dbfieldtyp[i]=FLOAT; n_RDNOISEB=i++;	
	sprintf(dbfieldkey[i],"SATURATB");dbfieldtyp[i]=FLOAT; n_SATURATB=i++;	

	sprintf(dbfieldkey[i],"WCSDIM  ");dbfieldtyp[i]=INTEG; n_WCSDIM=i++;	
	sprintf(dbfieldkey[i],"EQUINOX ");dbfieldtyp[i]=FLOAT; n_EQUINOX=i++;	
	sprintf(dbfieldkey[i],"CTYPE1  ");dbfieldtyp[i]=STRING;n_CTYPE1=i++;	
	sprintf(dbfieldkey[i],"CTYPE2  ");dbfieldtyp[i]=STRING;n_CTYPE2=i++;	
	sprintf(dbfieldkey[i],"CRVAL1  ");dbfieldtyp[i]=DOUBLE;n_CRVAL1=i++;	
	sprintf(dbfieldkey[i],"CRVAL2  ");dbfieldtyp[i]=DOUBLE;n_CRVAL2=i++;	
	sprintf(dbfieldkey[i],"CRPIX1  ");dbfieldtyp[i]=DOUBLE;n_CRPIX1=i++;	
	sprintf(dbfieldkey[i],"CRPIX2  ");dbfieldtyp[i]=DOUBLE;n_CRPIX2=i++;	
	sprintf(dbfieldkey[i],"CD1_1   ");dbfieldtyp[i]=DOUBLE;n_CD1_1=i++;	
	sprintf(dbfieldkey[i],"CD1_2   ");dbfieldtyp[i]=DOUBLE;n_CD1_2=i++;	
	sprintf(dbfieldkey[i],"CD2_1   ");dbfieldtyp[i]=DOUBLE;n_CD2_1=i++;	
	sprintf(dbfieldkey[i],"CD2_2   ");dbfieldtyp[i]=DOUBLE;n_CD2_2=i++;	
	for (v=0;v<=10;v++) {
	 sprintf(dbfieldkey[i],"PV1_%-4d",v);dbfieldtyp[i]=DOUBLE;n_PV1[v]=i++;
	}
	for (v=0;v<=10;v++) {
	 sprintf(dbfieldkey[i],"PV2_%-4d",v);dbfieldtyp[i]=DOUBLE;n_PV2[v]=i++;
	}
	

	/* initialize space for field values */
	if (!flag_quiet) 
	  printf("  Seeking the following keywords for db ingestion:\n  ");
	num_dbfields=i;
	for (i=0;i<num_dbfields;i++) {
	  if (dbfieldtyp[i]==FLOAT) 
	    dbfieldval[i]=(float *)calloc(1,sizeof(float));
	  if (dbfieldtyp[i]==STRING) 
	    dbfieldval[i]=(char *)calloc(80,sizeof(char));
	  if (dbfieldtyp[i]==DOUBLE) 
	    dbfieldval[i]=(double *)calloc(1,sizeof(double));
	  if (dbfieldtyp[i]==INTEG) 
	    dbfieldval[i]=(int *)calloc(1,sizeof(int));
	  if (!flag_quiet)  {
	    printf(" %s",dbfieldkey[i]);
	    if ((i+1)%8==0) printf("\n  ");
	  }
	}
	if (!flag_quiet) printf("\n");


	/**********************************************************/
	/**************  Open Image, Extract db tags **************/
	/**********************************************************/
	
	/* extract nite and imagetype from the input name */
	filename_resolve(filename,imageclass,runid,nite,tilename,
	  imagetype,imagename,band,&ccdnum);
        if (!flag_quiet) 
	  printf("  %s\n imageclass=%s runid=%s nite=%s band=%s tilename=%s imagetype=%s ccdnum=%d imagename=%s\n",
	    filename,imageclass,runid,nite,band,tilename,imagetype,
	    ccdnum,imagename);

        /* open the file */
        if (fits_open_file(&fptr,filename,READONLY,&status)) 
	  printerror(status);
	if (!flag_quiet) printf("  Opened %s\n",filename);	
		
	/* get the number of HDUs in the image */
	if (fits_get_num_hdus(fptr,&hdunum,&status)) printerror(status);
	if (!flag_quiet) printf("  %s has %d HDUs\n",filename,hdunum);
	if (hdunum!=63) {printf(" *** Not standard DECam format ***\n");exit(0);}
	
	indata=(float **)calloc(hdunum,sizeof(float *));


	/**********************************************************/
	/*************  Read Header from 0th Extension ************/
	/**********************************************************/
	
        exclkey=(char **)calloc(nexc,sizeof(char *));
        for (j=0;j<nexc;j++) {
          exclkey[j]=(char *)calloc(10,sizeof(char));
          sprintf(exclkey[j],"%s",exclist[j]);
        }
        if (fits_hdr2str(fptr,1,exclkey,nexc,&zeroheader,&numzerokeys,&status))
          printerror(status);
	/* cut off last bogus "END" line */
	zeroheader[strlen(zeroheader)-80]=0;
        if (!flag_quiet) {
	  printf("  Read %d header keywords from 0th header\n",numzerokeys);
	  printf("  *************************************************\n");
	  printf("  %s",zeroheader);
	  printf("*************************************************\n");
	}
	
	

	/**********************************************************/
	/**********  Read All Image Data into Memory **************/
	/**********    Enables Crosstalk Correction  **************/
	/********    Also Read BPM images if needed    ************/
	/**********************************************************/

	if (!flag_quiet) printf("  Reading image data from %s\n  ",
	    filename);	  
	for (i=2;i<=hdunum;i++) {

	  /* Move to the correct HDU */
	  if (fits_movabs_hdu(fptr,i,&hdutype,&status)) printerror(status);	

          /* read the NAXIS1 and NAXIS2 keyword to get image size */
          if (fits_read_keys_lng(fptr,"NAXIS",1,2,axes,&nfound,&status))
            printerror(status);

          /* double check image size */
          pixels  = axes[0]*axes[1];
          fpixel   = 1; nullval  = 0;
	  if (i>2) {
	    if (pixels!=oldpixel) {
	      printf("  Image extensions have different sizes:  %d  vs %d\n",
	        pixels,oldpixel);
	      exit(0);
	    }
	  }
	  else oldpixel=pixels;
	 
	  indata[i-1]=(float *)calloc(pixels,sizeof(float));
	  
          /* read the CHDU image  */
          if (fits_read_img(fptr,TFLOAT,fpixel,pixels,&nullval,
            indata[i-1],&anynull,&status)) printerror(status);
	    
	  if (!flag_quiet) {printf(".");fflush(stdout);}
	
	}
	if (!flag_quiet) {printf("\n");fflush(stdout);}
	

	/**********************************************************/
	/**********     Cycle through extensions        ***********/
	/**********    preparing and writing data       ***********/
	/**********************************************************/

	naxes[0]=axes[0];naxes[1]=axes[1];
	npixels=naxes[0]*naxes[1];

	/* jump to 2nd HDU  */
	if (fits_movabs_hdu(fptr,2,&hdutype,&status)) printerror(status);	

	/* now cycle through the HDU's parsing properly and writing */
	/* useful FITS file */
	for (i=2;i<=hdunum;i++) {

	  ccdnum=i-1;
	  outdata=indata[i-1];
	  
	  fits_get_hdu_num(fptr,&chdu);
	  if (!flag_quiet) printf("  Currently located at HDU %d of %d\n",
	    chdu,hdunum);
	  if (chdu!=i) {
	    printf("*****Not located at correct HDU (%d instead of %d)\n",
	      chdu,i);
	    exit(0);
	  }


	  /**********************************************************/
	  /*************  Read Header from Nth Extension ************/
	  /**********************************************************/

          if (fits_hdr2str(fptr,1,exclkey,nexc,&nthheader,&numnthkeys,&status))
            printerror(status);
	  /* cut off last bogus "END" line */
	  nthheader[strlen(nthheader)-80]=0;
          if (!flag_quiet) {
	    printf("  Read %d header keywords\n",numnthkeys);
	    printf("  *************************************************\n");
	    printf("  %s",nthheader);
	    printf("*************************************************\n");
	  }


	  /****************************************************************** */
	  /* ************** apply the crosstalk correction ****************** */
	  /* **************************************************************** */

	  if (flag_crosstalk) {
	    for (y=0;y<naxes[1];y++) for (x=0;x<naxes[0];x++) {
	      locout=y*naxes[0]+x;
	      xtalkcorrection=0.0;
	        if (x<naxes[0]/2) { /* in amp A */
		   ampextension=(i-2)*2;
		  for (j=1;j<hdunum;j++) {
		    ampA=(j-1)*2;
		    locA=locout;
		    if (xtalk[ampextension][ampA]>1.0e-6) 
		      xtalkcorrection+=xtalk[ampextension][ampA]*indata[j][locA];
		    ampB=ampA+1; /* Amp B flipped because readout on far side */
		    locB=y*naxes[0]+(naxes[0]-x-1);
		    if (xtalk[ampextension][ampB]>1.0e-6) 
		      xtalkcorrection+=xtalk[ampextension][ampB]*indata[j][locB];   
		  }
		  outdata[locout]-=xtalkcorrection;
		}
		else { /* in amp B */
		   ampextension=(i-2)*2+1;
		  for (j=1;j<hdunum;j++) {
		    ampA=(j-1)*2;
		    locA=y*naxes[0]+(naxes[0]-x-1);
		    if (xtalk[ampextension][ampA]>1.0e-6)
		      xtalkcorrection+=xtalk[ampextension][ampA]*indata[j][locA];	   
		    ampB=ampA+1; /* Amp B flipped because readout on far side */
		    locB=locout;
		    if (xtalk[ampextension][ampB]>1.0e-6) 
		      xtalkcorrection+=xtalk[ampextension][ampB]*indata[j][locB];	    
		  }
		  outdata[locout]-=xtalkcorrection;
		}
	    }
	  }

	  /* **************************************************************** */
	  /* **************** Write New Single CCD Image ******************** */
	  /* ************ Store Processing History in Header **************** */
	  /* **************************************************************** */

	  if (flag_gzip) sprintf(nfilename,"!%s_%02d.fits.gz",argv[2],i-1);
	  else sprintf(nfilename,"!%s_%02d.fits",argv[2],i-1);
	  if (fits_create_file(&nfptr,nfilename,&status)) printerror(status);
	  if (!flag_quiet) 
	    printf("  Opened image %s for CCD %d\n",nfilename+1,i-1);
	  
	  /* copy the header from the CHDU in the input image */
	  if (fits_copy_header(fptr,nfptr,&status)) printerror (status);
	  if (!flag_quiet) 
	    printf("  Copied header from %s[%i]\n",filename,i);
          /* resize the image */
          if (fits_resize_img(nfptr,FLOAT_IMG,2,naxes,&status)) 
	    printerror(status);             

	  /* write the new image  */
	  if (fits_write_img(nfptr,TFLOAT,1,npixels,outdata,&status))
	    printerror(status);
	  if (!flag_quiet) printf("  Wrote image data, ");

	  /* copy zero header information into the new header */
	  /* note that last keyword is null keyword and truncates the header! */
	  /* only copy the headers that live within zerokeyword */
	  if (fits_read_key_str(nfptr,"NROW    ",comment,comment,&status)) 
	    printerror(status);
	  if (fits_get_hdrpos(nfptr,&totkeynum,&keynum,&status)) 
	    printerror(status);
	  keynum-=4;
	  numwrittenkeys=0;
	  for (j=0;j<numzerokeys-1;j++) {
	      /* don't write keyword if it is duplicate */
	      keyflag=0;
	      for (k=0;k<numnthkeys;k++) {
		if (!strncmp(zeroheader+j*80,nthheader+k*80,8)) {
	  	  keyflag=1;
		  break;
	        }
	      }
	      /* copy the record into the header */
	      if (!keyflag) {
		if (!numwrittenkeys) {
	          /* write COMMENT that begins 0hdr */
		  if (fits_insert_record(nfptr,keynum+numwrittenkeys++,
		    "COMMENT ------------------------",
		    &status)) printerror(status);
		  if (fits_insert_record(nfptr,keynum+numwrittenkeys++,
		    "COMMENT Keywords from 0th Header",
		    &status)) printerror(status);
		  if (fits_insert_record(nfptr,keynum+numwrittenkeys++,
		    "COMMENT ------------------------",
		    &status)) printerror(status);
	 	}
		if (fits_insert_record(nfptr,keynum+numwrittenkeys,
		  zeroheader+j*80,&status))
	          printerror(status);
	  	numwrittenkeys++;
	      }
	  }
	  if (!flag_quiet) printf("%d keywords from 0th header, ",
	    numwrittenkeys-3);
	    
	  /* want only the image name-- search for first occurence of "/" */
          for (j=strlen(nfilename);j>=0;j--) 
	    if (!strncmp(&(nfilename[j]),"/",1)) break;
	  sprintf(newimagename,"%s",(nfilename+j+1));
	  for (j=strlen(newimagename);j>=0;j--) 
	    if (!strncmp(&(newimagename[j]),".fits",5)) break;
	  newimagename[j]=0;
	  if (!strncmp(newimagename,"!",1)) 
	    sprintf(newimagename,"%s",newimagename+1);
	  if (fits_modify_key_str(nfptr,"FILENAME",newimagename,"",&status)) 
	    printerror(status);
	  if (!flag_quiet) printf("FILENAME = %s, ",newimagename);

	  if (fits_insert_record(nfptr,totkeynum+(++numwrittenkeys),
	    "COMMENT ------------------------",
	    &status)) printerror(status);

	  /* add photflag */
	  if (fits_update_key_lng(nfptr,"PHOTFLAG",flag_phot,
	    "Night Photometric (1) or not (0)",&status)) printerror(status);
	  if (!flag_quiet) printf("PHOTFLAG = %d\n",flag_phot);

	  /* leave history in the header */
	  pip=popen("date","r");
	  fgets(command,200,pip);
	  pclose(pip);
	  /* must remove the newline */
	  command[strlen(command)-1]=0;
	  if (fits_write_key_str(nfptr,"DESMScvt",command,
	    "DECam image conversion and crosstalk correction",
	    &status)) printerror(status);
	  sprintf(longcomment,"DESDM:");
	  for (j=0;j<argc;j++) sprintf(longcomment,"%s %s",longcomment,argv[j]);
	  if (fits_write_comment(nfptr,longcomment,&status)) printerror(status);
	  if (!flag_quiet) {
	    printf("  => %s\n",longcomment);
	  }
	  /* mark this as an image extension */
	  if (fits_write_key_str(nfptr,"DES_EXT","IMAGE","Image extension",
	    &status)) printerror(status);

          /* close the new image */
          if (fits_close_file(nfptr,&status)) printerror(status);
	  if (!flag_quiet) printf("  Closed image %s\n",nfilename+1);
	  /* Move to the next HDU */
	  if (i<hdunum) 
	    if (fits_movrel_hdu(fptr,1,&hdutype,&status)) printerror(status);



	  /* ********************************************************* */
	  /* **********  Pull Keywords for DB from Header(s) ********* */
	  /* ***********  Print the DB data for ingestion ************ */
	  /* ********************************************************* */
	
	  /*  Cycle through the Keywords of interest to pull our info  */
	  for (j=0;j<num_dbfields;j++) {
	    /*printf("  %3d  ",j);*/
	    if (!grabkeywordval(zeroheader,numzerokeys,dbfieldkey[j],
	      dbfieldtyp[j],dbfieldval[j])) 
	      if (!grabkeywordval(nthheader,numnthkeys,dbfieldkey[j],
		dbfieldtyp[j],dbfieldval[j])) {
	        printf("  **  Keyword %s missing from DECam image %s\n",
		  dbfieldkey[j],filename);
		exit(0); 
	      }
	  }	
	    	  
	  rastring=(char *)dbfieldval[n_TELRA];
	  decstring=(char *)dbfieldval[n_TELDEC];
	  telequin=(float *)dbfieldval[n_TELEQUIN];
	  ra=raconvert(rastring,&rah,&ram,&ras);
	  dec=decconvert(decstring,&rah,&ram,&ras);
	  if (!flag_quiet) 
	    printf("  Pointing:  %0.7f  %0.7f %.0f\n",ra,dec,*telequin);

	  /* make some changes to the formatting */
	  /*if (!strcmp(dbfieldval[n_TELESCOP],"CTIO 4.0 meter telescope")
	    || !strcmp(dbfieldval[n_TELESCOP],"ct4m")) 
	    sprintf(dbfieldval[n_TELESCOP],"Blanco 4m");
	  sprintf(date,"%sT%s",dbfieldval[n_DATE_OBS],dbfieldval[n_TIME_OBS]);
	  sprintf(imagetype,"%s",dbfieldval[n_OBSTYPE]);
	  if (!strncmp(imagetype,"OBJECT",6)) sprintf(imagetype,"object");
	  if (!strncmp(imagetype,"ZERO",4)) sprintf(imagetype,"zero");
	  if (!strncmp(imagetype,"FLAT",4)) sprintf(imagetype,"flat");
	  if (!strncmp(imagetype,"DOME FLAT",9)) sprintf(imagetype,"dome flat");
	  if (!strncmp(dbfieldval[n_DETECTOR],"decam",5)) 
	    sprintf(dbfieldval[n_DETECTOR],"DECam");
	*/
	
	  /* output database ingest line */
	  if (1) {
	    printf("%.6f|%.6f|%.2f|%s",ra,dec,*telequin,dbfieldval[n_DATE_OBS]);
	    printf("|%.1f|%.1f|%.1f|%.1f|%0.3f",
	      *((float *)dbfieldval[n_GAINA]),
	      *((float *)dbfieldval[n_RDNOISEA]),
	      *((float *)dbfieldval[n_GAINB]),
	      *((float *)dbfieldval[n_RDNOISEB]),
	      *((float *)dbfieldval[n_AIRMASS]));
	    printf("|%s|%s|%s|%s|%s|%d|%.3f|%.3f|",
	      dbfieldval[n_FILTER],imageclass,dbfieldval[n_OBSTYPE],imagename,
	      nite,ccdnum,*((float *)dbfieldval[n_EXPTIME]),
	      *((float *)dbfieldval[n_DARKTIME]));
	    printf("%s|%s|%s|%s|%.4f|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|",
	      dbfieldval[n_OBJECT],dbfieldval[n_OBSERVAT],
	      dbfieldval[n_TELESCOP],dbfieldval[n_HA],
	      *((float *)dbfieldval[n_ZD]),dbfieldval[n_DETECTOR],
	      dbfieldval[n_OBSERVER],dbfieldval[n_PROPID],
	      dbfieldval[n_DATE_OBS],dbfieldval[n_WINDSPD],
	      dbfieldval[n_WINDDIR],dbfieldval[n_AMBTEMP],
	      dbfieldval[n_HUMIDITY],dbfieldval[n_PRESSURE],"");
	    printf("%7.2f|%d|%s|%s|%16.12f|%16.12f|%10.4f|%10.4f|",
	      *((float *)dbfieldval[n_EQUINOX]),*((int *)dbfieldval[n_WCSDIM]),
	      dbfieldval[n_CTYPE1],dbfieldval[n_CTYPE2],
	      *((double *)dbfieldval[n_CRVAL1]),
	      *((double *)dbfieldval[n_CRVAL2]),
	      *((double *)dbfieldval[n_CRPIX1]),
	      *((double *)dbfieldval[n_CRPIX2]));
	    printf("%19.13e|%19.13e|%19.13e|%19.13e|",
	      *((double *)dbfieldval[n_CD1_1]),
	      *((double *)dbfieldval[n_CD1_2]),
	      *((double *)dbfieldval[n_CD2_1]),
	      *((double *)dbfieldval[n_CD2_2]));
	    printf("%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|",
	      *((double *)dbfieldval[n_PV1[0]]),
	      *((double *)dbfieldval[n_PV1[1]]),
	      *((double *)dbfieldval[n_PV1[2]]),
	      *((double *)dbfieldval[n_PV1[3]]),
	      *((double *)dbfieldval[n_PV1[4]]),
	      *((double *)dbfieldval[n_PV1[5]]));
	    printf("%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|",
	      *((double *)dbfieldval[n_PV1[6]]),
	      *((double *)dbfieldval[n_PV1[7]]),
	      *((double *)dbfieldval[n_PV1[8]]),
	      *((double *)dbfieldval[n_PV1[9]]),
	      *((double *)dbfieldval[n_PV1[10]]));
	    printf("%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|",
	      *((double *)dbfieldval[n_PV2[0]]),
	      *((double *)dbfieldval[n_PV2[1]]),
	      *((double *)dbfieldval[n_PV2[2]]),
	      *((double *)dbfieldval[n_PV2[3]]),
	      *((double *)dbfieldval[n_PV2[4]]),
	      *((double *)dbfieldval[n_PV2[5]]));
	    printf("%11.10e|%11.10e|%11.10e|%11.10e|%11.10e|",
	      *((double *)dbfieldval[n_PV2[6]]),
	      *((double *)dbfieldval[n_PV2[7]]),
	      *((double *)dbfieldval[n_PV2[8]]),
	      *((double *)dbfieldval[n_PV2[9]]),
	      *((double *)dbfieldval[n_PV2[10]]));
	    printf("%d|%d|%d|%d\n",axes[0],axes[1],1,flag_phot);
	  }

	}
	  

	/* ********************************************************* */
	/* ************** Close Input Image and Exit *************** */
	/* ********************************************************* */

        if (fits_close_file(fptr,&status)) printerror(status);
	if (!flag_quiet) printf("  Closed image %s\n\n",filename);
	return(0);
}

int grabkeywordval(header,numkeys,keyword,keytyp,keyval)
  char header[],keyword[];
  int numkeys,keytyp;
  void *keyval;
{
	int	i,loc,j=0,found=0;
	char 	*val_str;
	float	*val_flt;
	double	*val_dbl;
	int	*val_int;
	
	for (i=0;i<numkeys;i++) {
	  loc=i*80;
	  /* find the keyword */
	  if (!strncmp(keyword,header+loc,strlen(keyword))) {
	    loc+=9;
	    /* strip of any leading spaces or ' */
	    while (!strncmp(header+loc," ",1) || 
	      !strncmp(header+loc,"'",1)) loc++;
	    found++;
	    if (keytyp==STRING) {
	      val_str=(char *)keyval;
	      /* copy over until "/" */
	      while (strncmp(header+loc,"/",1)) val_str[j++]=header[loc++];
	      j--;
	      /* strip off any trailing spaces of ' */
	      while (!strncmp(val_str+j," ",1) || 
	        !strncmp(val_str+j,"'",1)) j--;
	      val_str[j+1]=0;
	      /*printf("  KW: %8s %s\n",keyword,val_str);*/
	      break;
	    }
	    if (keytyp==FLOAT)  {
	      val_flt=(float *)keyval;
	      sscanf(header+loc,"%f",val_flt);
	      /*printf("  KW: %8s %f\n",keyword,*val_flt);*/
	      break;
	    }
	    if (keytyp==DOUBLE) {
	      val_dbl=(double *)keyval;
	      sscanf(header+loc,"%lf",val_dbl);
	      /*printf("  KW: %8s %f\n",keyword,*val_dbl);*/
	      break;
	    }
	    if (keytyp==INTEG) {
	      val_int=(int *)keyval;
	      sscanf(header+loc,"%d",val_int);
	      /*printf("  KW: %8s %d\n",keyword,*val_int);*/
	      break;
	    }
	  }
	}
	return(found);
}

#undef STRING
#undef FLOAT
#undef DOUBLE
#undef INTEG
