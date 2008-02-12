/* Reads image and converts (x,y) to (RA,DEC) */
#include "imageproc.h"

main(argc,argv)
        int argc;
        char *argv[];
{

        int     status=0,hdunum,x,y,nfound,imtype,
                length,xmin,xmax,anynull;
        float   *vecsort0,*vecsort1,*image,nullval;
        double  dbzero,dbscale;
        double  *ra,*dec,xval,yval,crpix1,crpix2,crval1,crval2,crdpix1_1,
		racenter,deccenter,raoffset,decoffset,decsize,rasize,ra1,dec1,ra2,dec2,
		crdpix1_2,crdpix2_1,crdpix2_2,rotation,dpix1,dpix2,dra,ddec;
        long    axes[2],pixels,npixels,bscale,bzero,bitpix,
                fpixel;
        unsigned long width;
        void    printerror();
        int     i,mode,decd,decm,decsign,rah,ram,ct;
	float	decs,ras;
        char    comment[FLEN_COMMENT],coordtype[200],name[200];
        float   scale,offset;
        int     flag_quiet=0;
	fitsfile *fptr;

        if (argc<2) {
          printf("imreadRADEC <image>\n");
          exit(0);
        }
        
        /* check if FITS file*/
        if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5))  {
          sprintf(name,"%s",argv[1]);
        } else {
          printf("  ** Must be FITS file\n");
          exit(0);
        }
        
        /* open the file */
        if (fits_open_file(&fptr,name,mode,&status)) printerror(status);
        /* read the NAXIS1 and NAXIS2 keyword to get image size */
        if (fits_read_keys_lng(fptr,"NAXIS",1,2,axes,&nfound,&status))
          printerror(status);
        if ( nfound>2) {
          printf("  ** Image dimension too large (%s,%d)\n",name,nfound);
          exit(0);
        }
          
        /* read the data type, BSCALE and BZERO */
        if (fits_read_key_lng( fptr,"BITPIX",&(bitpix),comment,&status))
          printerror(status);   
        if (fits_read_key_lng( fptr,"BSCALE",&(bscale),comment,&status))
          printerror(status);   
        if (fits_read_key_lng( fptr,"BZERO",&(bzero),comment,&status))
          printerror(status);
          
        if (!flag_quiet) {
          printf("  Opened %s:  %dD ( %d X %d )\n",
           name, nfound, axes[0], axes[1]);
          printf("    BITPIX   = %d", bitpix);
          printf("    BZERO    = %d", bzero);
          printf("    BSCALE   = %d\n", bscale);
        }
        
        /* get the number of HDUs in the image */
        if (fits_get_num_hdus(fptr,&hdunum,&status)) printerror(status);
        if (!flag_quiet) {
          printf("  %s has %d HDU",name,hdunum);
          if (hdunum>1) printf("s\n");
          else printf("\n");
        }
        if (hdunum>1) {printf("Currently not configured to work on MEFs\n");exit(0);}             
          
        /* prepare to read the image */
         npixels  =  axes[0]* axes[1];
         image=(float *)calloc( npixels,sizeof(float));
         /* create vectors for the RA and DEC as well */
         ra=(double *)calloc(npixels,sizeof(double));
         dec=(double *)calloc(npixels,sizeof(double));
        /* current hardwiring these FITS parameters */
         fpixel=1;  nullval=0.0;  
        /* read the image */
        if (fits_read_img( fptr,TFLOAT, fpixel, npixels,
          &nullval,image,&anynull,&status)) printerror(status);
            
        /* retrieve wcs information from header */
        if (fits_read_key_dbl(fptr,"CRPIX1",&crpix1,comment,&status)) printerror(status);
        if (fits_read_key_dbl(fptr,"CRPIX2",&crpix2,comment,&status)) printerror(status);
        if (fits_read_key_dbl(fptr,"CRVAL1",&crval1,comment,&status)) printerror(status);
        if (fits_read_key_dbl(fptr,"CRVAL2",&crval2,comment,&status)) printerror(status);
        /* read element of the coordinate rotation matrix */
        if (fits_read_key_dbl(fptr,"CD1_1",&crdpix1_1,comment,&status)) printerror(status);
        if (fits_read_key_dbl(fptr,"CD2_2",&crdpix2_2,comment,&status)) printerror(status);
        if (fits_read_key_dbl(fptr,"CD1_2",&crdpix1_2,comment,&status)) printerror(status);
        if (fits_read_key_dbl(fptr,"CD2_1",&crdpix2_1,comment,&status)) printerror(status);
 
        /* calculate the angle and delta pixel from the rotation matrix*/
	/*rotation=crdpix1_2/crdpix1_1;/*rotation=0.0;*/
        rotation=atan2(crdpix1_2,crdpix1_1);
	/*rotation=-0.5*M_PI;*/
	dpix1=crdpix1_1/fabs(crdpix1_1)*sqrt(Squ(crdpix1_1)+Squ(crdpix1_2));
	dpix1*=-1.0;
	dpix2=crdpix2_2/fabs(crdpix2_2)*sqrt(Squ(crdpix2_1)+Squ(crdpix2_2));
	dpix2*=-1.0;
	/*if (crdpix1_1/fabs(crdpix1_1)<0.0) rotation+=M_PI;*/
        printf("  DeltaX: %5.3f  DeltaY: %5.3f  Rotation: %7.2f\n",
	  dpix1*3600.0,dpix2*3600.0,rotation*180.0/M_PI);

	/* calculate the offset of the chip center from the boresite (crval1,crval2) */
	xval=axes[0]/2;yval=axes[1]/2;
        if (fits_pix_to_world(xval,yval,crval1,crval2,crpix1,crpix2,
          dpix1,dpix2,rotation,"-TAN",&racenter,&deccenter,&status)) 
          printerror(status);
	decoffset=deccenter-crval2;
	raoffset=(racenter-crval1)*cos(0.5*(deccenter+crval2)*M_PI/180.0);
        if (fits_pix_to_world(1.0,yval,crval1,crval2,crpix1,crpix2,
          dpix1,dpix2,rotation,"-TAN",&ra1,&dec1,&status)) printerror(status);
	if (fabs(ra1-racenter)>fabs(dec1-deccenter)) 
	  rasize=fabs(ra1-racenter)*cos(0.5*(dec1+deccenter)*M_PI/180.0);
	else decsize=fabs(dec1-deccenter);
        if (fits_pix_to_world(xval,1.0,crval1,crval2,crpix1,crpix2,
          dpix1,dpix2,rotation,"-TAN",&ra2,&dec2,&status)) printerror(status);
	if (fabs(ra2-racenter)>fabs(dec2-deccenter)) 
	  rasize=fabs(ra2-racenter)*cos(0.5*(dec2+deccenter)*M_PI/180.0);
	else decsize=fabs(dec2-deccenter);
	printf("  Positions:  Center (%.4f,%.4f)  1,C(%.4f,%.4f) C,1(%.4f, %.4f)\n",
	  racenter,deccenter,ra1,dec1,ra2,dec2);
	printf("  Chip's offsets and search sizes:  RA-  %.4f +/- %.4f DEC-  %.4f +/- %.4f\n",
	  raoffset,rasize,decoffset,decsize);
	
exit(0);
/*	  
	xval=crpix1;yval=crpix2;
	ct=0;
	while (ct<10) {
          if (fits_pix_to_world(xval,yval,0.0,0.0,crpix1,crpix2,
            dpix1,dpix2,rotation,"-TAN",&dra,&ddec,&status)) 
            printerror(status);
	  if (dra>0.0 && dra<360.0) dra-=360.0;
	  dec[0]=ddec+crval2;
	  ra[0]=crval1+dra/cos(M_PI/180.0*(dec[0]+crval2)/2.0);if (ra[0]>=360.00) ra[0]-=360.0;

	  printf("%9.3e %9.3e\n",dra,ddec);
	  rah=(int)(ra[0]/15.0);
	  ram=(int)((ra[0]/15.0-rah)*60.0);
	  ras=(ra[0]/15.0-rah-ram/60.0)*3600.0;
	  decsign=dec[0]/fabs(dec[0]);
	  decd=(int)(fabs(dec[0]));
	  decm=(int)((fabs(dec[0])-decd)*60.0);
	  decs=(fabs(dec[0])-decd-decm/60.0)*3600.0;
	  decd*=decsign;
	  printf("  **  (%7.2f,%7.2f)==(%02d:%02d:%06.3f,%3d:%02d:%5.2f)  **\n",
	    xval,yval,rah,ram,ras,decd,decm,decs);
	  xval+=1.0;ct++;
	}


	xval=878.0;yval=2043.0;
        if (fits_pix_to_world(xval,yval,0.0,0.0,crpix1,crpix2,
          dpix1,dpix2,rotation,"-TAN",&dra,&ddec,&status)) 
          printerror(status);
	if (dra>0.0 && dra<360.0) dra-=360.0;
	dec[0]=ddec+crval2;
	ra[0]=crval1+dra/cos(M_PI/180.0*(dec[0]+crval2)/2.0);if (ra[0]>=360.00) ra[0]-=360.0;
	printf("%9.3e %9.3e\n",dra,ddec);
	rah=(int)(ra[0]/15.0);
	ram=(int)((ra[0]/15.0-rah)*60.0);
	ras=(ra[0]/15.0-rah-ram/60.0)*3600.0;
	decsign=dec[0]/fabs(dec[0]);
	decd=(int)(fabs(dec[0]));
	decm=(int)((fabs(dec[0])-decd)*60.0);
	decs=(fabs(dec[0])-decd-decm/60.0)*3600.0;
	decd*=decsign;
	printf("  **  (%7.2f,%7.2f)==(%02d:%02d:%05.2f,%3d:%02d:%4.1f)  **\n",
	  xval,yval,rah,ram,ras,decd,decm,decs);
	  exit(0);
*/

	/* cycle through the image calculating the sky position (RA,DEC)*/
        for (i=0;i<npixels;i++) {
          yval=(i/axes[0]);
          xval=(i%axes[0]);
          if (fits_pix_to_world(xval,yval,0.0,0.0,crpix1,crpix2,
            dpix1,dpix2,rotation,"-TAN",ra+i,dec+i,&status)) 
            printerror(status);
	    if (ra[i]>0.0 && ra[i]<360.0) ra[i]-=360.0;
	    dec[i]+=crval2;
	    ra[i]=crval1+ra[i]/cos(M_PI/360.0*(dec[i]+crval2));
	    if (ra[i]>=360.00) ra[i]-=360.0;
        }

        /* close the image */
        if (fits_close_file(fptr,&status)) printerror(status);
        if (!flag_quiet) printf("  Closed %s: 2D ( %d X %d )\n",
          name,axes[0],axes[1]);
          
        /* free memory allocated to data arrays */
        free(image);
}
