/* run at this level:
  
    ../uid/data/nite/band
*/

#include "imageproc.h"

#define VERSION 1.0

main(argc,argv)
	int argc;
	char *argv[];
{
	char	listname[350],binpath[500],etcpath[500],nite[500], band[10], project[500],
		imagename[800],command[15000],root[800],tilename[800],comment[1000],
		linein[5000],tr1[1000],tr2[1000],tempname[1000],psfprofile[1000];
  	FILE	*fin, *pip;
	int	i,len,imm,imnum=0,nslash=0,flag_quiet=0,flag_ascii=0,flag_nobpm=0,flag_check=0,flag_remap=0,flag_list=0,flag_reduce=0,flag_psfex=0;
	int     flag_scamp=0,flag_weightthreshold=0,flag_saturation=0,flag_detectthreshold=0,status=0,flag_setsaturation=0,flag_gain=0,flag_photoaperture=0,flag_detectminarea=0,flag_psfprofile=0,flag_version=0,flag_nopsfex=0;
	float   weight_threshold,saturation,detect_threshold,gain,photoaperture,detectminarea;
	float   cd1_1,cd1_2,cd2_1,cd2_2;
	float   rho,rho_a,rho_b;
	float   cdelt1,cdelt2;
	float   fwhm,pixscale,fwhm_arcsec;
	time_t  curtime=time (NULL);
	fitsfile *fptr;
	void  printerror();

  	if ( argc < 2 ) {
    	  printf ("Usage: %s <imagename.fits or list (format: rootname/rootname_ccdnum.fits)> <bin-path> <etc-path> <-reduce and/or -remap and/or -scamp and/or -psfex>\n", argv [0]);
	  printf("  -reduce (use reduced image only; will need output from psfex)\n");
	  printf("  -remap (use remap image only; will need output from psfex)\n");
	  printf("  -scamp (output catalog for scamp only)\n");
	  printf("  -psfex (output catalog for psfex only)\n");
	  printf("  For -reduce and -remap options:\n");
	  printf("          -ascii (output as ASCII)\n");
	  printf("          -no_bpm (not using bpm.fits as flag image)\n");
	  printf("          -check (output check images of reg.fits and obj.fits)\n");
	  printf("          -no_psfex\n");
	  printf("          -psfprofile <profile> (profile option; default: DEVAUCOULEURS,EXPONENTIAL)\n");
	  printf("  For -remap option only:\n");
	  printf("          -nite <nite>\n");
	  printf("          -band <band> \n");
	  printf("          -project <project>\n");
	  printf("  For -scamp option only:\n");
	  printf("          -weight_threshold <#>\n");
	  printf("          -saturation (use the saturation given in the image)\n");
	  printf("          -set_saturation <#>\n");
	  printf("          -detect_threshold <#>\n");
	  printf("  For -psfex option only:\n");
	  printf("          -saturation (use the saturation given in the image)\n");
	  printf("          -set_saturation <#>\n");
	  printf("          -photo_aperture <#>\n");
	  printf("          -detect_minarea <#>\n");
	  printf("          -gain <#>\n");
	  printf("  -version\n");
	  printf("  -quiet\n");
    	  exit(0);
  	}
	
	/* process the command line */

	if (!strcmp(argv[1],"-version")) {
	    printf("%s: Version %2.2f\n",argv[0],VERSION);
	    exit(0);
	}
	else
	  sprintf(listname, "%s", argv[1]);
  	sprintf(binpath, "%s", argv[2]);
	sprintf(etcpath, "%s", argv[3]);

	sprintf(psfprofile, "%s","DEVAUCOULEURS,EXPONENTIAL");

	for (i=4;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-ascii")) flag_ascii=1;
	  if (!strcmp(argv[i],"-no_bpm")) flag_nobpm=1;
	  if (!strcmp(argv[i],"-reduce")) flag_reduce=1;
	  if (!strcmp(argv[i],"-check")) flag_check=1;
	  if (!strcmp(argv[i],"-remap")) flag_remap=1;
	  if (!strcmp(argv[i],"-scamp"))  flag_scamp=1;
	  if (!strcmp(argv[i],"-psfex")) flag_psfex=1;
	  if (!strcmp(argv[i],"-no_psfex")) flag_nopsfex=1;
	  
	  if (!strcmp(argv[i],"-nite")) 
	    sprintf(nite, "%s", argv[i+1]);
	  if (!strcmp(argv[i],"-band")) 
	    sprintf(band, "%s", argv[i+1]);    
	  if (!strcmp(argv[i],"-project")) 
	    sprintf(project, "%s", argv[i+1]);
	  
	  if (!strcmp(argv[i],"-weight_threshold")) {
	    flag_weightthreshold=1;
	    sscanf(argv[i+1],"%f",&weight_threshold);
	  }
	  if (!strcmp(argv[i],"-detect_threshold")) {
	    flag_detectthreshold=1;
	    sscanf(argv[i+1],"%f",&detect_threshold);
	  }
	  if (!strcmp(argv[i],"-set_saturation")) {
	    flag_setsaturation=1;
	    sscanf(argv[i+1],"%f",&saturation);
	  }
	  if (!strcmp(argv[i],"-gain")) {
	    flag_gain=1;
	    sscanf(argv[i+1],"%f",&gain);
	  }
	  if (!strcmp(argv[i],"-photo_aperture")) {
	    flag_photoaperture=1;
	    sscanf(argv[i+1],"%f",&photoaperture);
	  }
	  if (!strcmp(argv[i],"-detect_minarea")) {
	    flag_detectminarea=1;
	    sscanf(argv[i+1],"%f",&detectminarea);
	  }
	  if (!strcmp(argv[i],"-saturation")) 
	    flag_saturation=1;
	  if (!strcmp(argv[i],"-psfprofile")) {
	    flag_psfprofile=1;
	    sscanf(psfprofile,"%s",argv[i+1]);
	  }
	}

	/* print out the time of processing */
	if(!flag_quiet)
	  printf(" ** Running %s on %s \n",argv[0],asctime(localtime (&curtime)));
	
	/* check if single image or a list */

	if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5))  {
	  sprintf(imagename,"%s",argv[1]);
	  imnum=1;
	}
	else { /* expect file containing list of images */

	  /* check list file to confirm it exists */
	  if ( (fin=fopen (listname, "r")) == NULL ) {
	    printf ("List file \"%s\" not found.\n", listname);
	    exit (0);
	  }  
	  imnum=0;flag_list=1;
	  fin=fopen(argv[1],"r");
	  while (fscanf(fin,"%s",imagename)!=EOF) {
	    imnum++;
	    if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5)) {
	      printf("  ** File must contain list of FITS images **\n");
	      exit(0);
	    }
	  }
	  fclose(fin);
    
	  /* reopen for process */
	  sprintf(listname, "%s", argv[1]);
	  fin=fopen(listname,"r");
	}

	if(!flag_quiet && flag_list) printf("\n ** Input list %s contains %d images \n", argv[1], imnum);

	/* read through image list  */
	for (imm=0;imm<imnum;imm++) {  
	  
	  /* initiallize strings */
	  for(i=0;i<800;i++) {
	    root[i]=0;
	    if(flag_list) imagename[i]=0;
	  }

	  /* get next image name */
	  if (flag_list) fscanf(fin,"%s",imagename);

	  /* grab the root name of the image */
	  sprintf(root,"%s",imagename);
	  len=strlen(imagename);
	  for (i=len;i>0;i--) if (!strncmp(&(imagename[i]),".fits",5)) {
	    root[i]=0;
	    break;
	  }
	  
	  /* check if the imagename in the right format */
	  nslash=0;
	  for (i=len;i>0;i--) if (!strncmp(&(imagename[i]),"/",1)) {
	      nslash++;
	      break;
	  }
	  if(!nslash) {
	    printf(" ** Warning: input file(s) name is not in imagename/imagename_ccdnum.fits format! Abort.\n");
	    exit(0);
	  }	  

	  /* run SExtractor here */
	  if(flag_remap) {
	    /* find out the tiles for the image */
	    sprintf(command, "%s/findtile %s %s %s %s -quiet -tilename_only", binpath, imagename, nite, band, project);
	  
	    if (!flag_quiet) printf("  %s\n",command);
	    pip=popen(command,"r");
	    while (fgets(linein,5000,pip)!=NULL) {
	      sscanf(linein,"%s %s %s",tr1,tr2,tilename);
	      /* run SExtractor now */
	      if (strcmp(tr1,"**")) {

		/* read the CDi_j and FWHM keywords */
		tempname[0]=0;
		sprintf(tempname,"%s.%s_im.fits",root,tilename);

		if(fits_open_file(&fptr,tempname,READONLY,&status)) printerror(status);
		
		if(fits_read_key_flt(fptr,"CD1_1",&cd1_1,comment,&status)) printerror(status);
		if(fits_read_key_flt(fptr,"CD2_1",&cd2_1,comment,&status)) printerror(status);
		if(fits_read_key_flt(fptr,"CD1_2",&cd1_2,comment,&status)) printerror(status);
		if(fits_read_key_flt(fptr,"CD2_2",&cd2_2,comment,&status)) printerror(status);
		if(fits_read_key_flt(fptr,"FWHM",&fwhm,comment,&status)) printerror(status);
		
		if(fits_close_file(fptr,&status)) printerror(status);
		
		/* evaluate rho_a and rho_b as in Calabretta & Greisen (2002), eq 191 */
		if(cd2_1>0) rho_a=atan(cd2_1/cd1_1);
		else if(cd2_1<0) rho_a=atan(-cd2_1/-cd1_1);
		else rho_a=0.0;
		
		if(cd1_2>0) rho_b=atan(cd1_2/-cd2_2);
		else if(cd1_2<0) rho_b=atan(-cd1_2/cd2_2);
		else rho_b=0.0;
		
		/* evaluate rho and CDELTi as in Calabretta & Greisen (2002), eq 193 */
		rho=0.5*(rho_a+rho_b);
		cdelt1=cd1_1/cos(rho);
		cdelt2=cd2_2/cos(rho);

		/* convert the pixel to arcsec */
		pixscale=0.5*(fabs(cdelt1)+fabs(cdelt2))*3600;
		fwhm_arcsec=fwhm*pixscale;
		
		sprintf(command, "%s/SExtractor %s.%s_im.fits -c %s/sex.config ", 
			binpath,root,tilename,etcpath);
		sprintf(command, "%s -FILTER_NAME %s/sex.conv -STARNNW_NAME %s/sex.nnw  ", 
			command,etcpath,etcpath);
		sprintf(command, "%s -WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE %s.%s_var.fits -WEIGHT_THRESH 0.0 ", 
			command,root,tilename);
		sprintf(command, "%s -SEEING_FWHM %2.3f ", 
			command,fwhm_arcsec);
		if(flag_ascii) sprintf(command, "%s -CATALOG_TYPE ASCII ", command);
		if(!flag_nobpm) sprintf(command, "%s -FLAG_IMAGE %s_bpm.fits ", command, root);
		if(flag_check) sprintf(command, "%s -CHECKIMAGE_TYPE SEGMENTATION,OBJECTS -CHECKIMAGE_NAME %s.%s_reg.fits,%s.%s_obj.fits ", command, root,tilename,root,tilename);

		if(flag_ascii) sprintf(command, "%s -CATALOG_NAME %s.%s.cat", command,root,tilename);
		else if(flag_check) sprintf(command, "%s -CATALOG_NAME %s.%s_tab.fits", command,root,tilename);
		else sprintf(command, "%s -CATALOG_NAME %s.%s_cat.fits", command,root,tilename);
		
		if(flag_nopsfex) 
		  sprintf(command,"%s -PARAMETERS_NAME %s/sex.param_nopsfex ",command,etcpath);
		else
		  sprintf(command,"%s -PARAMETERS_NAME %s/sex.param -PSF_NAME %s.psf -PROFILE_TYPE %s ",command,etcpath,root,psfprofile);

		if (!flag_quiet) printf("%s\n",command);
		system (command);
	      }
	      else
		printf(" ** No tiles found for image %s.fits\n",root);
	    }
	    pclose(pip);  
	  }
	  
	  if(flag_reduce) {

	    /* read the CDi_j and FWHM keywords */
	    tempname[0]=0;
	    sprintf(tempname,"%s_im.fits",root);

	    if(fits_open_file(&fptr,tempname,READONLY,&status)) printerror(status);
	    
	    if(fits_read_key_flt(fptr,"CD1_1",&cd1_1,comment,&status)) printerror(status);
	    if(fits_read_key_flt(fptr,"CD2_1",&cd2_1,comment,&status)) printerror(status);
	    if(fits_read_key_flt(fptr,"CD1_2",&cd1_2,comment,&status)) printerror(status);
	    if(fits_read_key_flt(fptr,"CD2_2",&cd2_2,comment,&status)) printerror(status);
	    if(fits_read_key_flt(fptr,"FWHM",&fwhm,comment,&status)) printerror(status);
	    
	    if(fits_close_file(fptr,&status)) printerror(status);

	    /* evaluate rho_a and rho_b as in Calabretta & Greisen (2002), eq 191 */
	    if(cd2_1>0) rho_a=atan(cd2_1/cd1_1);
	    else if(cd2_1<0) rho_a=atan(-cd2_1/-cd1_1);
	    else rho_a=0.0;
	    
	    if(cd1_2>0) rho_b=atan(cd1_2/-cd2_2);
	    else if(cd1_2<0) rho_b=atan(-cd1_2/cd2_2);
	    else rho_b=0.0;
	      
	    /* evaluate rho and CDELTi as in Calabretta & Greisen (2002), eq 193 */
	    rho=0.5*(rho_a+rho_b);
	    cdelt1=cd1_1/cos(rho);
	    cdelt2=cd2_2/cos(rho);

	    /* convert the pixel to arcsec */
	    pixscale=0.5*(fabs(cdelt1)+fabs(cdelt2))*3600;
	    fwhm_arcsec=fwhm*pixscale;

	    /* run sextractor here */
	    sprintf(command, "%s/SExtractor %s_im.fits -c %s/sex.config ", 
		    binpath,root,etcpath);
	    sprintf(command, "%s -FILTER_NAME %s/sex.conv -STARNNW_NAME %s/sex.nnw ", 
		    command,etcpath,etcpath);
	    sprintf(command, "%s -WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE %s_var.fits -WEIGHT_THRESH 0.0 ", 
		    command,root);
	    sprintf(command, "%s -SEEING_FWHM %2.3f ", 
		    command,fwhm_arcsec);
	    if(flag_ascii) sprintf(command, "%s -CATALOG_TYPE ASCII ", command);
	    if(!flag_nobpm) sprintf(command, "%s -FLAG_IMAGE %s_bpm.fits ", command, root);
	    if(flag_check) sprintf(command, "%s -CHECKIMAGE_TYPE SEGMENTATION,OBJECTS -CHECKIMAGE_NAME %s_reg.fits,%s_obj.fits ", command, root,root);
	    
	    if(flag_ascii) sprintf(command, "%s -CATALOG_NAME %s.cat", command,root);
	    else if(flag_check) sprintf(command, "%s -CATALOG_NAME %s_tab.fits", command,root);
	    else sprintf(command, "%s -CATALOG_NAME %s_cat.fits", command,root);
	    
	    if(flag_nopsfex) 
	      sprintf(command,"%s -PARAMETERS_NAME %s/sex.param_nopsfex ",command,etcpath);
	    else
	      sprintf(command,"%s -PARAMETERS_NAME %s/sex.param -PSF_NAME %s.psf -PROFILE_TYPE %s ",command,etcpath,root,psfprofile);
	    
	    if (!flag_quiet) printf("%s\n",command);
	    system (command);
	  }

	  if(flag_scamp) {
	    
	    if(flag_saturation) {
	      tempname[0]=0;
	      sprintf(tempname,"%s_im.fits",root);
	    
	      if(fits_open_file(&fptr,tempname,READONLY,&status)) printerror(status);
	        
	      if(fits_read_key_flt(fptr,"SATURATE",&saturation,comment,&status))
		printerror(status);

	      if(fits_close_file(fptr,&status)) printerror(status);
	    }
	    
	    sprintf(command,"%s/SExtractor %s_im.fits -c %s/default.sex -CATALOG_NAME %s_scamp.fits -CATALOG_TYPE FITS_LDAC -WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE %s_var.fits  -FLAG_IMAGE %s_bpm.fits -PARAMETERS_NAME %s/sex.param_scamp -FILTER_NAME %s/sex.conv -STARNNW_NAME %s/sex.nnw ",
		 binpath,root,etcpath,root,root,root,etcpath,etcpath,etcpath);
	    if(flag_weightthreshold)
	      sprintf(command,"%s -WEIGHT_THRESH %2.4f ",command,
		weight_threshold);
	    if(flag_setsaturation)
	      sprintf(command,"%s -SATUR_LEVEL %5.4f ",command,saturation);
	    if(flag_saturation)
	      sprintf(command,"%s -SATUR_LEVEL %5.4f ",command,saturation);
	    if(flag_detectthreshold)
	      sprintf(command,"%s -DETECT_THRESH %2.4f ",command,
	 	detect_threshold);

	    if (!flag_quiet) printf("%s\n",command);
	    system(command);	    

	    /* extract the FWHM (in pixel) from the catalog file and insert back to image using fwhm.c */
	    sprintf(command,"%s/fwhm %s_scamp.fits %s_im.fits",binpath,root,root);
	 
	    if (!flag_quiet) printf("%s\n",command);
	    system(command);	    
	  }

	  if(flag_psfex) {
	    
	    if(flag_saturation) {
	      tempname[0]=0;
	      sprintf(tempname,"%s_im.fits",root);
	    
	      if(fits_open_file(&fptr,tempname,READONLY,&status)) printerror(status);
	        
	      if(fits_read_key_flt(fptr,"SATURATE",&saturation,comment,&status))
		printerror(status);

	      if(fits_close_file(fptr,&status)) printerror(status);
	    }
	    
	    /* run Sextractor to prepare the catalogs for psfex */
	    sprintf(command,"%s/SExtractor %s_im.fits -c %s/default.sex -CATALOG_NAME %s_vig.fits -CATALOG_TYPE FITS_LDAC -WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE %s_var.fits -PARAMETERS_NAME %s/sex.param_psfex -FILTER_NAME %s/sex.conv -STARNNW_NAME %s/sex.nnw ",
		 binpath,root,etcpath,root,root,etcpath,etcpath,etcpath);
	    if(flag_setsaturation)
	      sprintf(command,"%s -SATUR_LEVEL %5.4f ",command,saturation);
	    if(flag_saturation)
	      sprintf(command,"%s -SATUR_LEVEL %5.4f ",command,saturation);
	    if(flag_gain)
	      sprintf(command,"%s -GAIN %5.4f ",command,gain);
	    if(flag_photoaperture)
	      sprintf(command,"%s -PHOT_APERTURES %2.2f ",command,photoaperture);
	    else
	      sprintf(command,"%s -PHOT_APERTURES 15.0 ",command);
	    if(flag_detectminarea)
	      sprintf(command,"%s -DETECT_MINAREA %2.2f ",command,detectminarea);
	    else
	      sprintf(command,"%s -DETECT_MINAREA 3 ",command);

	    if (!flag_quiet) printf("%s\n",command);
	    system(command);	    

	    /* run the psfex here */
	    sprintf(command,"%s/psfex %s_vig.fits -c %s/default.psfex -WRITE_XML N -PSF_NAME %s.psf -CHECKIMAGE_TYPE NONE ",binpath,root,etcpath,root);
	    
	    if (!flag_quiet) printf("%s\n",command);
	    system(command);
	  }

	}
  	if(flag_list) fclose(fin);

	if(!flag_quiet)
	  printf(" ** Done %s on %s \n",argv[0],asctime(localtime (&curtime)));
	
	return 0;
}

#undef VERSION
