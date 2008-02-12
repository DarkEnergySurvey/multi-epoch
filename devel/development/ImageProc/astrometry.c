#include "imageproc.h"

main(argc,argv)
     int argc;
     char *argv[];
{  
  char	imagelist[1000],imagename[800],name[800],binpath[1000],etcpath[1000];
  char  catname[2000],command[10000],rootname[100],tempname[1000],incat[100];
  char  ctype1[100],ctype2[100],comment[100],server[100];
  int	flag_quiet=0,flag_list=0,imnum,im,i,starnum=100,status=0;
  int   flag_weightthreshold=0,flag_saturation=0,flag_detectthreshold=0;
  int   flag_delw=0,flag_server=0,flag_incat=0,flag_localcat=0;
  int   flag_satfix=0,flag_distort_deg=0,flag_crossrad=0,distort_deg;
  int   flag_fgroup=0,flag_nsigma=0;
  float weight_threshold,saturation,detect_threshold,saturation_in;
  float crossrad,fgroup_radius,astrclip_nsigma;
  FILE	*inp;
  fitsfile *fptr;
  void	printerror();

  if(argc < 2) {
    printf("Usage: %s <image/list>\n", argv[0]);
    printf("    Options:\n");
    printf("          -binpath <binpath>\n");
    printf("          -etcpath <etcpath>\n");
    printf("          -delete_wat (delete the WAT parameters)\n");
    printf("          -weight_threshold <#>\n");
    printf("          -saturation \n");
    printf("          -saturationfix <#> \n");
    printf("          -detect_threshold <#>\n");
    printf("          -distort_degree <#>\n");
    printf("          -crossid_radius <#>\n");
    printf("          -astrclip_nsigma <#>\n");
    printf("          -fgroup_radius <#>\n");
    printf("          -server <server_name>\n");
    printf("          -catalog <NONE,USNO-A1,USNO-A2,USNO-B1,GSC-1.3,GSC-2.2,UCAC-1,UCAC-2,2MASS (default is USNO-B1)>\n");
    printf("          -catalogname <local input catalog name>\n");
    printf("          -quiet\n");
    exit(0);
  }
  sprintf(imagelist, "%s", argv[1]);

  /* default */
  sprintf(binpath,".");
  sprintf(etcpath,".");
  
  for (i=2;i<argc;i++) {
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
    if (!strcmp(argv[i],"-binpath")) sprintf(binpath,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-etcpath")) sprintf(etcpath,"%s",argv[i+1]);
    if (!strcmp(argv[i],"-delete_wat")) flag_delw=1;
    if (!strcmp(argv[i],"-weight_threshold")) {
      flag_weightthreshold=1;
      sscanf(argv[i+1],"%f",&weight_threshold);
    }
    if (!strcmp(argv[i],"-detect_threshold")) {
      flag_detectthreshold=1;
      sscanf(argv[i+1],"%f",&detect_threshold);
    }
    if (!strcmp(argv[i],"-saturation")) {
      flag_saturation=1;
    }
    if (!strcmp(argv[i],"-saturationfix")) {
      flag_satfix=1;
      sscanf(argv[i+1],"%f",&saturation_in);
    }
    if (!strcmp(argv[i],"-crossid_radius")) {
      flag_crossrad=1;
      sscanf(argv[i+1],"%f",&crossrad);
    }
    if (!strcmp(argv[i],"-fgroup_radius")) {
      flag_fgroup=1;
      sscanf(argv[i+1],"%f",&fgroup_radius);
    }
    if (!strcmp(argv[i],"-astrclip_nsigma")) {
      flag_nsigma=1;
      sscanf(argv[i+1],"%f",&astrclip_nsigma);
    }
    if (!strcmp(argv[i],"-distort_degree")) {
      flag_distort_deg=1;
      distort_deg=atoi(argv[i+1]);
    }
    if (!strcmp(argv[i],"-server")) {
      flag_server=1;
      sprintf(server,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-catalog")) {
      flag_incat=1;
      sprintf(incat,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-catalogname")) {
      flag_incat=0;
      flag_localcat=1;
      sprintf(catname,"%s",argv[i+1]);
    }
  }

  /* copy input image name if FITS file*/
  if (!strncmp(&(imagelist[strlen(imagelist)-5]),".fits",5))  {
    sprintf(imagename,"%s",imagelist);
    imnum=1;
  }
  else { /* expect file containing list of images */
    imnum=0;flag_list=1;
    inp=fopen(imagelist,"r");
    while (fscanf(inp,"%s",imagename)!=EOF) {
      imnum++;
      if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5)) {
	printf("  ** File must contain list of FITS images **\n");
	exit(0);
      }
    }
    fclose(inp);
  }
  if (!flag_quiet) printf("  Input list %s contains %d FITS images\n",argv[1],imnum);
  
  /* ************************************* */
  /* ****** Cycle through Images  ******** */
  /* ************************************* */
  if(flag_list) inp=fopen(imagelist,"r");
  for (im=0;im<imnum;im++) {
    /* get next image name */
    
    if (flag_list) fscanf(inp,"%s",imagename);

    /* get root name by trimming .fits off the imagename */
    sprintf(name,"%s",imagename);
    name[strlen(imagename)-5]=0;
    for (i=strlen(name);i>0;i--) if (!strncmp(&(name[i]),"/",1)) break;
    sprintf(rootname,"%s",name+i+1);

    /* modify the CTYPE keywords */
    tempname[0]=0;
    sprintf(tempname,"%s_im.fits",name);

    if(fits_open_file(&fptr,tempname,READWRITE,&status)) printerror(status);

    if(fits_read_key_str(fptr,"CTYPE1",ctype1,comment,&status)) printerror(status);
    if(fits_read_key_str(fptr,"CTYPE2",ctype2,comment,&status)) printerror(status);
    
    if(strcmp(ctype1,"RA---TAN")) {
      if(fits_modify_key_str(fptr,"CTYPE1","RA---TAN","Changed to -TAN",&status))
	printerror(status);
    }
    if(strcmp(ctype2,"DEC--TAN")) {
      if(fits_modify_key_str(fptr,"CTYPE2","DEC--TAN","Changed to -TAN",&status))
	printerror(status);
    }

    if(flag_delw) {
      if(fits_delete_key(fptr,"WAT0_001",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT1_001",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT1_002",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT1_003",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT1_004",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT1_005",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT2_001",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT2_002",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT2_003",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT2_004",&status))
	printerror(status);
      if(fits_delete_key(fptr,"WAT2_005",&status))
	printerror(status);
    }
    
    if(flag_saturation) {
	if(fits_read_key_flt(fptr,"SATURATE",&saturation,comment,&status)) 
	  printerror(status);
    }
    
    if(fits_close_file(fptr,&status)) printerror(status);

    for(i=1;i<=1;i++) {
      /* set up cataloging run */
      sprintf(command,"%s/SExtractor %s_im.fits -c %s/default.sex -CATALOG_NAME %s.cat -CATALOG_TYPE FITS_LDAC -WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE %s_var.fits  -FLAG_IMAGE %s_bpm.fits -PARAMETERS_NAME %s/sex.param_scamp -FILTER_NAME %s/sex.conv -STARNNW_NAME %s/sex.nnw ",
	      binpath,name,etcpath,name,name,name,etcpath,etcpath,etcpath);
      if(flag_weightthreshold)
	sprintf(command,"%s -WEIGHT_THRESH %2.4f ",command,weight_threshold);
      if(flag_saturation)
	sprintf(command,"%s -SATUR_LEVEL %5.4f ",command,saturation);
      if(flag_satfix)
	sprintf(command,"%s -SATUR_LEVEL %5.4f ",command,saturation_in);
      if(flag_detectthreshold)
	sprintf(command,"%s -DETECT_THRESH %2.4f ",command,detect_threshold);
      printf("loop %d: %s\n",i,command);
      system(command);

      /* run scamp */
      sprintf(command,"%s/scamp %s.cat -c %s/default.scamp -MOSAIC_TYPE LOOSE -WRITE_XML N ",
	      binpath,name,etcpath);

      if(flag_distort_deg)
	sprintf(command,"%s -DISTORT_DEGREES %d ",command,distort_deg);
      if(flag_crossrad)
	sprintf(command,"%s -CROSSID_RADIUS %2.2f ",command,crossrad);
      if(flag_fgroup)
	sprintf(command,"%s -FGROUP_RADIUS %2.2f ",command,fgroup_radius);
      if(flag_nsigma)
	sprintf(command,"%s -ASTRCLIP_NSIGMA %2.2f ",command,astrclip_nsigma);

      if(flag_server)
	sprintf(command,"%s -REF_SERVER %s ",command,server);
      if(flag_incat)
	sprintf(command,"%s -ASTREF_CATALOG %s ",command,incat);
      if(flag_localcat) {
	sprintf(command,"%s -ASTREF_CATALOG FILE ",command);
	sprintf(command,"%s -ASTREFCAT_NAME %s ",command,catname); 
      }
      else
	sprintf(command,"%s -CDSCLIENT_EXEC %s/aclient ",command,binpath);

      printf("loop %d: %s\n",i,command);
      system(command);

      /* clean up .cat */
      sprintf(command,"rm %s.cat",name);
      system(command);

      /* insert .head keywords back to the image */
      sprintf(command,"%s/insert_head %s_im.fits %s.head",binpath,name,name);
      system(command);
      sprintf(command,"%s/insert_head %s_var.fits %s.head",binpath,name,name);
      system(command);
      sprintf(command,"%s/insert_head %s_bpm.fits %s.head",binpath,name,name);
      system(command);
    }	  
  }
  
  /* finish up */
  if(flag_list) fclose(inp);
  return (0);
}
