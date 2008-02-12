/* run at this level:
    ../uid/data/nite/
*/

#include "imageproc.h"

main(argc,argv)
        int argc;
        char *argv[];
{
  	char   	listname[800], binpath[1200], etcpath[1200], nite[500], band[10], 
		project[500], imagepath[1200],outlistname[1800];
  	char   	command[10000], command1[1500], imagename[800], tempname[800], 
		rootname[800], varname[800], tiletempname[1800];
  	char   	line[5000],image[800],var[800],imtemp[800],vartemp[800],
		bpmtemp[800],bpm[800],comment[1000],imscamp[800],runid[1000];
  	int    	i, j, len, imnum=0,count=0, ccdnum, imm, nslash=0, status=0;
  	int    	flag_nite=0,flag_band=0,flag_project=0,flag_binpath=0,
		flag_etcpath=0,flag_imagepath=0,flag_quiet=0,flag_noskysub=0,
		flag_list=0,flag_out=0,flag_scamp=0,flag_runid=0,nstar;
	float   scampchi2;
  	db_tiles *tileinfo;
  	FILE   	*in,*pip,*out,*outlist;  
  	fitsfile *fptr,*fptr_var,*fptr_bpm;
  	void 	printerror();

  if (argc<2) {
    printf("Usage: %s <imagename.fits or list (format: rootname/rootname_ccdnum.fits)> ", argv[0]);
    printf("  -nite <nite> ");
    printf("  -band <band> ");
    printf("  -project <project> ");
    printf("  -binpath <binpath> ");
    printf("  -etcpath <etcpath> ");
    printf("  -imagepath <imagepath>\n");
    printf("  Options:\n");
    printf("  -no_skysubtract \n");
    printf("  -outlist <outlist> \n");
    printf("  -runid <runid> \n");
    printf("  -quiet \n");
    exit(0);
  }
  
  /* process the command line */
  for (i=2;i<argc;i++) {
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
    if (!strcmp(argv[i],"-nite"))
      {
	flag_nite=1;
	sprintf(nite,"%s",argv[i+1]);
      }
    if (!strcmp(argv[i],"-band"))
      {
	flag_band=1;
	sprintf(band,"%s",argv[i+1]);
      }
    if (!strcmp(argv[i],"-project"))
      {
	flag_project=1;
	sprintf(project,"%s",argv[i+1]);
      }
    if (!strcmp(argv[i],"-binpath"))
      {
	flag_binpath=1;
	sprintf(binpath,"%s",argv[i+1]);
      }
    if (!strcmp(argv[i],"-etcpath"))
      {
	flag_etcpath=1;
	sprintf(etcpath,"%s",argv[i+1]);
      }
    if (!strcmp(argv[i],"-imagepath"))
      {
	flag_imagepath=1;
	sprintf(imagepath,"%s",argv[i+1]);
      }
    if (!strcmp(argv[i],"-outlist"))
      {
	flag_out=1;
	sprintf(outlistname,"%s",argv[i+1]);
	if(!flag_quiet) printf(" ** Output list for remap images is: %s\n",outlistname); 
      }
    if (!strcmp(argv[i],"-runid"))
      {
	flag_runid=1;
	sprintf(runid,"%s",argv[i+1]);	
      }
    if (!strcmp(argv[i],"-no_skysubtract")) flag_noskysub=1;
  }

  /* check input commands--> add in auxiliaryproc? */
  if(!flag_nite) { printf(" ** -nite <nite> is not set, abort!\n"); exit(0); }
  if(!flag_band) { printf(" ** -band <band> is not set, abort!\n"); exit(0); }
  if(!flag_project) { printf(" ** -project <project> is not set, abort!\n"); exit(0); }
  if(!flag_binpath) { printf(" ** -binpath <binpath> is not set, abort!\n"); exit(0); }
  if(!flag_etcpath) { printf(" ** -etcpath <etcpath> is not set, abort!\n"); exit(0); }
  if(!flag_imagepath) { printf(" ** -imagepath <imagepath> is not set, abort!\n"); exit(0); }

  /* check if single image or a list */
  if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5))  {
    sprintf(imagename,"%s",argv[1]);
    imnum=1;
    /* run findtile to get the tile info for the image */
    sprintf(command1, "%s/findtile %s %s %s %s -quiet", binpath, imagename, nite, band, project);
    if(!flag_quiet) printf(" -Running: %s\n\n", command1);
  }
  else { /* expect file containing list of images */
    imnum=0;flag_list=1;
    in=fopen(argv[1],"r");
    if(in==NULL) {
      printf("  ** File %s not found\n",argv[1]);
      exit(0);
    }
    
    while (fscanf(in,"%s",imagename)!=EOF) {
      imnum++;
      if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5)) {
	printf("  ** File must contain list of FITS images **\n");
	exit(0);
      }
    }
    fclose(in);
    
    /* reopen for process */
    sprintf(listname, "%s", argv[1]);
    in=fopen(listname,"r");
  }
  
  if(!flag_quiet && flag_list) printf("\n ** Input list %s contains %d images \n", argv[1], imnum);
  
  /* check the filename: imagenam/imagename_ccdnum.fits--> put in auxiliaryproc, same as name_resolve? */
  for (imm=0;imm<imnum;imm++) {  
    
    /* get next image name */
    if (flag_list) fscanf(in,"%s",imagename);
    
    len=strlen(imagename);
    for (i=len;i>0;i--) {
      if (!strncmp(&(imagename[i]),"/",1)) {
	imagename[i]=32;
	nslash++;
	break;
      }
    }	
    if(nslash == 0) {
      printf(" ** Warning: input file(s) name is not in imagename/imagename_ccdnum.fits format! Abort.\n");
      exit(0);
    }
  }
  
  if(flag_list) {
    fclose(in);
    /* run findtile to get the tile info for each image */
    if(flag_runid)
      sprintf(command, "%s/findtile %s %s %s %s -runid %s -quiet", binpath, listname, nite, band, project,runid);
    else
      sprintf(command, "%s/findtile %s %s %s %s -quiet", binpath, listname, nite, band, project);
    if(!flag_quiet) printf(" -Running: %s\n\n", command);
  }
  
  out=fopen("temp.list","w");
  if(flag_list) pip=popen(command,"r");
  else pip=popen(command1,"r");
  while (fgets(line,5000,pip)!=NULL) {
    if(!flag_quiet) printf("%s", line);
    len=strlen(line);
    for(i=len;i>0;i--) {
      /* in case is "no found" */ 
      if(!strncmp(&(line[i]),"found",5)) break;
    }
    if (i==0)  { /* line found */
      fprintf(out, "%s",line);
      count++;
    }
  }
  pclose(pip);
  fclose(out);
  
  if(count == 0) {
    printf(" ** No tile found for image(s), abort %s\n", argv[0]);
    system("/bin/rm temp.list");
    exit(0);
  }
  
  if(!flag_quiet) {
    if(flag_list) printf(" ** Total number of tiles for images in %s = %d\n", 
			 listname, count);
    else printf(" ** Number of tiles for image %s = %d\n", imagename, count);
  }
  
  /* memory allocation for the tiles info */
  tileinfo=(db_tiles *)calloc(count+1,sizeof(db_tiles));
  
  /* input tile info and run the swarp for remap */
  in=fopen("temp.list","r");
  if(flag_out) outlist=fopen(outlistname,"w");
  
  for(i=0;i<count;i++) {
    fscanf(in, "%s %s %s %lf %lf %f %d %d",tileinfo[i].basedir, 
	   tileinfo[i].imagename, tileinfo[i].tilename, &(tileinfo[i].ra), 
	   &(tileinfo[i].dec), &(tileinfo[i].pixelsize), &(tileinfo[i].npix_ra), 
	   &(tileinfo[i].npix_dec));
    
    /* output list of remap images */
    if(flag_out) {
      sprintf(tiletempname,"%s",tileinfo[i].imagename);
      len=strlen(tiletempname);
      for (j=len;j>0;j--) {
	if (!strncmp(&(tiletempname[j]),".fits",5)) {
	  tiletempname[j]=0;
	  break;
	}
      }
      fprintf(outlist,"%s/%s.%s.fits\n",band, tiletempname, tileinfo[i].tilename);
    }
    
    sprintf(imtemp,"%s",tileinfo[i].imagename);
    len=strlen(imtemp);
    for (j=len;j>0;j--) {
      if (!strncmp(&(imtemp[j]),".fits",5)) {
	imtemp[j]=0;
	break;
      }
    }
    
    /* check the image header for SCAMP info */
    sprintf(imscamp,"%s/%s/%s_im.fits",imagepath,band,imtemp);
    status=0;
    if(fits_open_file(&fptr,imscamp,READONLY,&status))
      printerror(status);
    if(fits_read_key(fptr,TINT,"SCAMPFLG",&flag_scamp,comment,&status))
      printerror(status);
    if(fits_read_key(fptr,TINT,"SCAMPNUM",&nstar,comment,&status))
      printerror(status);
    if(fits_read_key_flt(fptr,"SCAMPCHI",&scampchi2,comment,&status))
      printerror(status);
    if(fits_close_file(fptr,&status))
      printerror(status);

    if(!flag_scamp) {

      /* Swarp query for the image */
      sprintf(tempname,"%s_im.fits",imtemp);
      sprintf(command, "%s/swarp %s/%s/%s -c %s/default.swarp ",binpath, 
	      imagepath, band, tempname, etcpath);
      sprintf(command, "%s-PIXELSCALE_TYPE MANUAL -PIXEL_SCALE %2.5f -CENTER_TYPE MANUAL -CENTER %2.7f,%2.7f  -IMAGE_SIZE %d,%d ",
	      command,tileinfo[i].pixelsize,tileinfo[i].ra,tileinfo[i].dec,
	      tileinfo[i].npix_ra,tileinfo[i].npix_dec);
      if(!flag_noskysub) sprintf(command, "%s-SUBTRACT_BACK Y ", command);
      else sprintf(command, "%s-SUBTRACT_BACK N ", command);
      sprintf(command, "%s-WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE %s/%s/%s_var.fits ", 
	      command, imagepath, band, imtemp);
      sprintf(command, "%s-RESAMPLE Y -RESAMPLE_DIR %s/%s/%s -FSCALASTRO_TYPE NONE -RESAMPLING_TYPE LANCZOS3 -RESAMPLE_SUFFIX .%s_im.fits ", 
	      command, imagepath, band, tileinfo[i].basedir, tileinfo[i].tilename);
      sprintf(command, "%s-COMBINE N -COMBINE_TYPE WEIGHTED ", command);
      sprintf(command, "%s-COPY_KEYWORDS OBJECT,OBSERVER,CCDNUM,FILTER,OBSTYPE,FILENAME,TELRA,TELDEC,TELEQUIN,AIRMASS,DATE-OBS,EXPTIME,SATURATE,DETECTOR,TELESCOP,OBSERVAT,SKYBRITE,PHOTFLAG,FWHM,ELLIPTIC,SCAMPCHI,SCAMPFLG,SCAMPNUM ", command);
      sprintf(command, "%s-DELETE_TMPFILES Y  -HEADER_ONLY N -VERBOSE_TYPE NORMAL\n", command);
      if(!flag_quiet) printf(" -Running: %s\n", command);
      system(command);
      
      /* Swarp query for the bpm image */
      sprintf(bpmtemp,"%s_bpm.fits",imtemp);
      sprintf(command, "%s/swarp %s/%s/%s -c %s/default.swarp ",binpath, 
	      imagepath, band, bpmtemp, etcpath);
      sprintf(command, "%s-PIXELSCALE_TYPE MANUAL -PIXEL_SCALE %2.5f -CENTER_TYPE MANUAL -CENTER %2.7f,%2.7f  -IMAGE_SIZE %d,%d ",
	      command,tileinfo[i].pixelsize, tileinfo[i].ra, tileinfo[i].dec, 
	      tileinfo[i].npix_ra, tileinfo[i].npix_dec);
    
      sprintf(command, "%s-SUBTRACT_BACK N ", command);
    
      sprintf(command, "%s-WEIGHT_TYPE NONE ", command);
      sprintf(command, "%s-RESAMPLE Y -RESAMPLE_DIR %s/%s/%s -FSCALASTRO_TYPE NONE -RESAMPLING_TYPE NEAREST -RESAMPLE_SUFFIX .%s_bpm.fits ", 
	      command, imagepath, band, tileinfo[i].basedir, tileinfo[i].tilename);
      sprintf(command, "%s-COMBINE N ", command);
      sprintf(command, "%s-DELETE_TMPFILES Y  -HEADER_ONLY N -VERBOSE_TYPE NORMAL\n", 
	      command);
      if(!flag_quiet) printf(" -Running: %s\n", command);
      system(command);
    
      /* mv the produced images to correct format */
      sprintf(command, "/bin/mv %s/%s/%s_im.%s_im.weight.fits %s/%s/%s.%s_var.fits\n", 
	      imagepath, band, imtemp, tileinfo[i].tilename, imagepath, band, imtemp, 
	      tileinfo[i].tilename);
      sprintf(command+strlen(command), 
	      "/bin/mv %s/%s/%s_im.%s_im.fits %s/%s/%s.%s_im.fits\n", 
	      imagepath, band, imtemp, tileinfo[i].tilename, imagepath, band, imtemp, 
	      tileinfo[i].tilename);
      /* remove the weight map produced for the bpm */
      sprintf(command+strlen(command),"/bin/rm %s/%s/%s_bpm.%s_bpm.weight.fits\n",
	      imagepath,band,imtemp,tileinfo[i].tilename);
      /* remove the head file produced by SCAMP, not needed at this stage */
      //sprintf(command+strlen(command),"rm %s/%s/%s.head\n",
      //imagepath,band,imtemp);
      sprintf(command+strlen(command), 
	    "/bin/mv %s/%s/%s_bpm.%s_bpm.fits %s/%s/%s.%s_bpm.fits\n", 
	      imagepath, band, imtemp, tileinfo[i].tilename, imagepath, band, imtemp, 
	      tileinfo[i].tilename);
      if (!flag_quiet) printf("  %s\n",command);
      system(command);
    
      /* open the *_im.fits, *_var.fits, *_bpm.fits and put the DES_EXT keyword */
      status=0;
      
      sprintf(image, "%s/%s/%s.%s_im.fits",imagepath,band, imtemp, tileinfo[i].tilename);
      if(fits_open_file(&fptr,image,READWRITE,&status))
	printerror(status);
      if(fits_write_key_str(fptr,"DES_EXT","IMAGE","Image extension",&status))
	printerror(status);
      if(fits_close_file(fptr,&status))
	printerror(status);
      
      sprintf(var, "%s/%s/%s.%s_var.fits", imagepath, band, imtemp, tileinfo[i].tilename);
      if(fits_open_file(&fptr_var,var,READWRITE,&status))
	printerror(status);
      if(fits_write_key_str(fptr_var,"DES_EXT","VARIANCE","Image extension",&status))
	printerror(status);
      if(fits_close_file(fptr_var,&status))
	printerror(status);
      
      sprintf(bpm, "%s/%s/%s.%s_bpm.fits", imagepath, band, imtemp, tileinfo[i].tilename);
      if(fits_open_file(&fptr_bpm,bpm,READWRITE,&status))
	printerror(status);
      if(fits_write_key_str(fptr_bpm,"DES_EXT","BPM","Image extension",&status))
	printerror(status);
      if(fits_close_file(fptr_bpm,&status))
	printerror(status);
    }
    else {
      if(!flag_quiet)
	printf(" ** Image %s is not remapped: SCAMPFLG=%d\tSCAMPNUM=%d\tSCAMPCHI=%2.2f\n ",imscamp,flag_scamp,nstar,scampchi2);
    }
  }
  fclose(in);

  /* clean up and etc */
  system("/bin/rm temp.list");
  
  if(flag_out) fclose(outlist);
  
  cfree(tileinfo);
  
  return(0);
}
  
