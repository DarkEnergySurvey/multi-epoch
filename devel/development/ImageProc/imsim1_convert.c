/* This program will transform ImSim1 image from Fermilab into 62 FITS files, each
/* containing the imaging data from a single CCD. Initial WCS parameters are also
/* added to each image header using a model of the DECam focal plane and the telescope
/* pointing information that is already present.
*/
#include "imageproc.h"

#define VERBOSE 0

main(argc,argv)
	int argc;
	char *argv[];
{
	char	filename[200],nfilename[200],record[120],
		comment[FLEN_COMMENT],**hdumapping,rdfilename[200],
		longstring[2000],tr[100],strrightascension[20],
	  	strdeclination[20],command[200],longcomment[200],
		astfilename[200],nmstring[100],
		filter[FLEN_VALUE],obstype[FLEN_VALUE];
	int	status=0,anynull,nfound,keysexist,morekeys,i,rah,ram,decd,decm,
		imtype,hdunum,quiet,x,y,hdutype,chdu,j,len,crpix=1,ltv,
		*hdultv,flag,wcsdim=2,mef_flag;
	float	exptime,airmass=1.0,equinox=2000.0,*hdurdnoise,*hdugain,ras,decs,
		rightascension,declination,ltm1=1.0;
	long	axes[2],naxes[2],taxes[2],pixels,npixels,fpixel,bscale,
		bzero,bitpix;
	short	*data,nullval,*data0,*data1,*data3,saturate=50000;
	double	*hducrpix1,*hducrpix2,*hducrdpix1_1,*hducrdpix1_2,
		mjdobs=53392.8868,radouble,decdouble,
		*hducrdpix2_1,*hducrdpix2_2;
	void	printerror();
	fitsfile *fptr,*nfptr;
	FILE	*inp,*pip;

	if (argc<4) {printf("imsim1_convert <infile.fits> <DECam model> <root/outfile.fits> <quiet=0/1>\n");exit(0);}
	sprintf(filename,"%s",argv[1]);
	sprintf(astfilename,"%s",argv[2]);
	sprintf(nfilename,"%s",argv[3]);
	if (argc<5) quiet=0;
	else sscanf(argv[4],"%d",&quiet);
	mef_flag=0; /* write out individual files */

        /* open the file */
        if (fits_open_file(&fptr,filename,READONLY,&status)) printerror(status);
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
	  
	
        /* prepare to read the image */
        pixels  = axes[0]*axes[1];
        fpixel   = 1; nullval  = 0;
	/* prepare data vector */
        data=(short *)calloc(pixels,sizeof(short));
	  
	
	/* get the Telescope RA and DEC */
	if (fits_read_key_flt(fptr,"TELRA",&rightascension,comment,&status))
	  printerror(status);
	if (rightascension<0.0) rightascension+=360.0;
	radouble=rightascension;
	rightascension/=15.0;
	rah=rightascension;
	ram=(int)((rightascension-rah)*60.0);
	ras=((rightascension-(float)rah-(float)ram/60.0)*3600.0);
	sprintf(strrightascension,"%02d:%02d:%05.2f",rah,ram,ras);
	if (fits_read_key_flt(fptr,"TELDEC",&declination,comment,&status))
	  printerror(status);
	decdouble=declination;
	decd=(int)(fabs(declination));
	decm=(int)((fabs(declination)-(float)decd)*60.0);
	decs=((fabs(declination)-(float)decd-(float)decm/60.0)*3600.0);

	if (declination<0.0) sprintf(strdeclination,"-%02d:%02d:%04.1f",decd,decm,decs);
	else sprintf(strdeclination,"%02d:%02d:%04.1f",decd,decm,decs);
	if (!quiet) printf("  Pointing RA is %13.6e/%s DEC is %13.6e/%s\n",rightascension,strrightascension,
	  declination,strdeclination);
		
	/* get the number of HDUs in the image */
	if (fits_get_num_hdus(fptr,&hdunum,&status)) printerror(status);
	if (!quiet) printf("  %s has %d HDUs\n",filename,hdunum);
		
	/* now read the astrometry .db information from the input file */
	hducrpix1=(double *)calloc(hdunum*2,sizeof(double));
	hducrpix2=(double *)calloc(hdunum*2,sizeof(double));
	hducrdpix1_1=(double *)calloc(hdunum*2,sizeof(double));
	hducrdpix2_1=(double *)calloc(hdunum*2,sizeof(double));
	hducrdpix1_2=(double *)calloc(hdunum*2,sizeof(double));
	hducrdpix2_2=(double *)calloc(hdunum*2,sizeof(double));
	hdultv=(int *)calloc(hdunum*2,sizeof(int));
	inp=fopen(astfilename,"r");
	i=-1;
	while(fgets(longstring,2000,inp)!=NULL) {
	  if (!strncmp(longstring,"begin",5)) {
	  i++;if (i==hdunum*2) 
	    {printf("Too many header units found\n");exit(0);}
	  flag=0;
	  /* set up offsets from detector edge to */
	  if (i%2==0) hdultv[i]=28;
	  else hdultv[i]=-1024;
	  while(fgets(longstring,2000,inp)!=NULL && !flag) {
	    sscanf(longstring,"%s",nmstring);
	    /* grab the x and y reference pix information */
	    if (!strncmp(nmstring,"xpixref",7)) {
	      sscanf(longstring,"%s %lf",nmstring,hducrpix1+i);
	      if (hdultv[i]==28) hducrpix1[i]-=hdultv[i];
	      else hducrpix1[i]-=1052;
	    hducrpix1[i]-=14070.7;
	    }
	    if (!strncmp(nmstring,"ypixref",7)) {
	      sscanf(longstring,"%s %lf",nmstring,hducrpix2+i);
	    }
	    /* grab the delta pix information */
	    if (!strncmp(nmstring,"surface1",8)) {
	      fgets(longstring,2000,inp); fgets(longstring,2000,inp); fgets(longstring,2000,inp);
	      fgets(longstring,2000,inp); fgets(longstring,2000,inp); fgets(longstring,2000,inp);
	      fgets(longstring,2000,inp); fgets(longstring,2000,inp); fgets(longstring,2000,inp);
	      fscanf(inp,"%lf %lf",hducrdpix1_1+i,hducrdpix1_2+i);
	      fscanf(inp,"%lf %lf",hducrdpix2_1+i,hducrdpix2_2+i);
	      hducrdpix1_1[i]/=3600.; hducrdpix1_2[i]/=3600.;
	      hducrdpix2_1[i]/=3600.; hducrdpix2_2[i]/=3600.;
	      flag=1;
	    }
	  }
	  if (!quiet) printf("  %03d  %12.5e %5d %12.5e %12.5e %12.5e\n",
	    i,hducrpix1[i],hdultv[i],hducrpix2[i],hducrdpix1_1[i],
	    hducrdpix1_2[i],hducrdpix2_1[i],hducrdpix2_2[i]);
	  }
	}
	fclose(inp);
	

	/* simply create a single image array that fits a full CCD */
	naxes[0]=axes[0];naxes[1]=axes[1];
	npixels=naxes[0]*naxes[1];
	data3=(short *)calloc(npixels,sizeof(short));

	/*strip .fits and any prefix off the input filename*/
	for (i=0;i<strlen(filename);i++) if (!strncmp(&filename[i],".fit",4)) filename[i]=0;
	for (i=strlen(filename)-1;i>=0;i--) if (!strncmp(&filename[i],"/",1)) {
	  i++;
	  for (j=i;j<=strlen(filename);j++) filename[j-i]=filename[j];
	  break;
	}
	if (!quiet) printf("  Input filename root is %s\n",filename);
	/* now cycle through the HDU's parsing properly and writing useful FITS file */
	for (i=1;i<=hdunum;i++) {
	  
	  fits_get_hdu_num(fptr,&chdu);
	  if (chdu!=i) {
	    printf("Not located at correct HDU (%d instead of %d)\n",chdu,i);
	    exit(0);
	  }

          /* read the CHDU image  */
          if (fits_read_img(fptr,USHORT_IMG,fpixel,pixels,&nullval,
            data,&anynull,&status)) printerror(status);
	  /* copy the image into the new data arrays*/
	  for (y=0;y<naxes[1];y++) for (x=0;x<axes[0];x++) 
	    data3[y*naxes[0]+x]=data[y*axes[0]+x];
	  if (!quiet) printf("  Read image data from CHDU=%d of %s\n",chdu,filename);
	  	  
	  /* open new image */
	  sprintf(nfilename,"!%s_%02d.fits",argv[3],i);
	  if (fits_create_file(&nfptr,nfilename,&status)) printerror(status);
	  if (!quiet) printf("  Opened image %s for extension %d\n",nfilename+1,i);
	  
	  /* copy the header from the CHDU in the input image */
	  if (fits_copy_header(fptr,nfptr,&status)) printerror (status);
	  if (!quiet) printf("  Copied header from %s[%i] into new image\n",filename,i);

	  /* write the new image and copy the header */
	  /*if (fits_create_img(nfptr,USHORT_IMG,2,naxes,&status)) printerror(status);*/
	  if (fits_write_img(nfptr,TUSHORT,1,npixels,data3,&status))
	    printerror(status);

	  /* write initial WCS solution into header */
	  if (fits_write_comment(nfptr,"--------------------------------",&status)) 
	    printerror(status);
	  if (fits_write_comment(nfptr,"WCS Section",&status)) 
	    printerror(status);
	  if (fits_write_comment(nfptr,"--------------------------------",&status)) 
	    printerror(status);
	  if (fits_write_key(nfptr,TSTRING,"CTYPE1","RA---TAN","",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TSTRING,"CTYPE2","DEC--TAN","",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TINT,"LTV1",&hdultv[i*2-2],"",&status))
	    printerror(status);	 
	  if (fits_write_key(nfptr,TINT,"WCSDIM",&wcsdim,"",&status))
	    printerror(status);	 
	  if (fits_write_key(nfptr,TDOUBLE,"CRPIX1",&hducrpix1[i*2-2],"",&status))
	    printerror(status);	 
	  if (fits_write_key(nfptr,TDOUBLE,"CRPIX2",&hducrpix2[i*2-2],"",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TDOUBLE,"CRVAL1",&radouble,"",&status))
	    printerror(status);	 
	  if (fits_write_key(nfptr,TDOUBLE,"CRVAL2",&decdouble,"",&status))
	    printerror(status);	   
	  if (fits_write_key(nfptr,TDOUBLE,"CD1_1",&hducrdpix1_1[i*2-2],"",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TDOUBLE,"CD2_1",&hducrdpix1_2[i*2-2],"",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TDOUBLE,"CD1_2",&hducrdpix2_1[i*2-2],"",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TDOUBLE,"CD2_2",&hducrdpix2_2[i*2-2],"",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TFLOAT,"LTM1_1",&ltm1,"",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TFLOAT,"LTM2_2",&ltm1,"",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TSTRING,"WAT0_001","system=image","",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TSTRING,"WAT1_001","wtype=tan axtype=ra","",&status))
	    printerror(status);	  
	  if (fits_write_key(nfptr,TSTRING,"WAT2_001","wtype=tan axtype=dec","",&status))
	    printerror(status);	  
	  
	  
	  /* leave history in the header */
	  pip=popen("date","r");
	  fgets(command,200,pip);
	  pclose(pip);
	  /* must remove the newline */
	  command[strlen(command)-1]=0;
	  sprintf(longcomment,"DESDM:  imsim1_convert on %s",command);
	  if (!quiet) printf("  %s\n",longcomment);
	  if (fits_write_comment(nfptr,longcomment,&status)) printerror(status);


	  if (!quiet) printf("  **Wrote image data and Header Keywords to CHDU of %s\n",
	    nfilename+1);	    

          /* close the new image */
          if (fits_close_file(nfptr,&status)) printerror(status);
	  if (!quiet) printf("  Closed image %s\n",nfilename+1);

	  /* Move to the next HDU */
	  if (i<hdunum) if (fits_movrel_hdu(fptr,1,&hdutype,&status)) printerror(status);

	}
	  
        /* close the image */
        if (fits_close_file(fptr,&status)) printerror(status);
	if (!quiet) printf("  Closed image %s\n",filename);
	
}


