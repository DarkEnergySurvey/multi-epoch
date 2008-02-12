/* run the Swarp for quick coadd to check the images

it will run the select_remaps first with, e.g.: select_remaps BCS BCS0531-5225 g -nite bcs051124 -quiet -coadd


*/

#include "imageproc.h"

main(argc,argv)
     int argc;
     char *argv[];
{
  char binpath[1024],etcpath[1024],basedir[1024],output[1024];
  char nite_in[4096],band_in[64],project[128],tilename_in[4096];
  char command[4096],command1[4096],line[1024],err[64],tem[100];
  char ccdlist[1024],templist[4096],imgfullpath[10000],combinetype[50];
  char runidlist[2048];
  int flag_quiet=0,flag_binpath=0,flag_etcpath=0,flag_ccd=0,flag_nite=0;
  int flag_combinetype=0,flag_runid=0;
  int i,j,k,count=0,nimage,len,ncomma,nccd,*ccd_num,s;
  db_tiles *tileinfo;
  FILE *pip,*fout,*ferr;

  if (argc<2) {
    printf("Usage: %s \n",argv[0]);
    printf("       -project <project> -band <band#1,band#2,...> -tilename <tile name>\n");
    printf("       -output <filename.fits> (fits filename for coadded output, can include full path) \n");
    printf("       -basedir <basedir> (dir level before runid/data/nite/band/...) \n");
    printf("       Options:\n");
    printf("                -binpath <binpath> \n"); 
    printf("                -etcpath <etcpath> \n");
    printf("                -nite <nite#1,nite#2,...> \n");
    printf("                -ccdnum <ccd_number#1,ccd_number#2,...> \n");
    printf("                -runid <runid#1,runid#2,...> \n");
    printf("                -combinetype <median,average,min,max,weighted,chi2,sum; default is weighted> \n");
    printf("                -quiet\n");
    exit(0);
  }

  if(!flag_quiet) printf(" ** Begin quick_coadd \n");

  /* setup default value */
  ccdlist[0]=0;
    
  /* process the command lines */
  for(i=1;i<argc;i++) {
    if (!strcmp(argv[i],"-project")) {
      sprintf(project,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-band")) {
      sprintf(band_in,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-tilename")) {
      sprintf(tilename_in,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-basedir")) {
      sprintf(basedir,"%s",argv[i+1]);
    }
    if (!strcmp(argv[i],"-output")) {
      sprintf(output,"%s",argv[i+1]);
      if (strncmp(&(output[strlen(output)-5]),".fits",5)) {
        printf("  ** quick_coadd error: output file must end with .fits \n");
        exit(0);
      }
    }

    /* options */
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
    if (!strcmp(argv[i],"-binpath")) {
      sprintf(binpath,"%s",argv[i+1]);
      flag_binpath=1;
    }
    if (!strcmp(argv[i],"-etcpath")) {
      sprintf(etcpath,"%s",argv[i+1]);
      flag_etcpath=1;
    }
    if (!strcmp(argv[i],"-ccdnum")) {
      sprintf(ccdlist,"%s",argv[i+1]);
      flag_ccd=1;
      ncomma=0;
      len=strlen(ccdlist);
      for (j=len;j>0;j--) {
        if (!strncmp(&(ccdlist[j]),",",1)) { 
          ncomma++;
          ccdlist[j]=32;
        }
      }
      nccd=ncomma+1;
      ccd_num=(int *)calloc(nccd,sizeof(int));
      s=0;
      for(k=0;k<nccd;k++) {
	sscanf(ccdlist+s,"%s%[\0]",tem);
	ccd_num[k]=atoi(tem);
	len=strlen(tem);
	s+=len+1;
      }
    }
    if (!strcmp(argv[i],"-nite")) {
      sprintf(nite_in,"%s",argv[i+1]);
      flag_nite=1;
    }
    if (!strcmp(argv[i],"-runid")) {
      sprintf(runidlist,"%s",argv[i+1]);
      flag_runid=1;
    }
    if (!strcmp(argv[i],"-combinetype")) {
      if (!strcmp(argv[i+1],"median")) sprintf(combinetype,"MEDIAN");
      else if (!strcmp(argv[i+1],"average")) sprintf(combinetype,"AVERAGE");
      else if (!strcmp(argv[i+1],"min")) sprintf(combinetype,"MIN");
      else if (!strcmp(argv[i+1],"max")) sprintf(combinetype,"MAX");
      else if (!strcmp(argv[i+1],"weighted")) sprintf(combinetype,"WEIGHTED");
      else if (!strcmp(argv[i+1],"chi2")) sprintf(combinetype,"CHI2");
      else if (!strcmp(argv[i+1],"sum")) sprintf(combinetype,"SUM");
      else {
	printf(" ** quick_coadd error: wrong input for <combinetype>, reset to default\n");
	sprintf(combinetype,"WEIGHTED");
      }
      flag_combinetype=1;
    }
  }

  /* construct the select_remaps call */
  if(flag_binpath) {
    sprintf(command, "%s/select_remaps %s %s %s %s -quiet -type remap -coadd ",
	    binpath,project,tilename_in,band_in,basedir);
    
    if(flag_nite)
      sprintf(command, "%s -nite %s ", command,nite_in);
    if(flag_runid)
      sprintf(command, "%s -runid %s ", command,runidlist);
  }
  else {
    sprintf(command, "select_remaps %s %s %s %s -quiet -type remap -coadd ",
	    project,tilename_in,band_in,basedir);
    
    if(flag_nite)
      sprintf(command, "%s -nite %s ", command,nite_in);
    if(flag_runid)
      sprintf(command, "%s -runid %s ", command,runidlist);
  }
  
  /* count the images in the given tile from select_remaps */
  sprintf(command1, "%s | wc -l",command);

  if (!flag_quiet) {
    printf("%s\n",command1);
    fflush(stdout);
  }
  pip=popen(command1,"r");
  while (fgets(line,1024,pip)!=NULL) 
    sscanf(line,"%d",&nimage);
  pclose(pip);
  
  if(!flag_quiet) {
    printf(" ** Found %d images from <select_remaps>\n",nimage);
    if(flag_ccd) {
      printf(" ** But only process ccdnum = ");
      for(k=0;k<nccd;k++) printf("%d ",ccd_num[k]);
      printf("\n");
    }
  }

  if(!nimage) {
    if(!flag_quiet)
      printf(" ** No image found, abort\n\n");
    exit(0);
  }

  /* memory allocation */
  tileinfo=(db_tiles *)calloc(nimage,sizeof(db_tiles));

  /* input information from select_remaps output */
  if(!flag_quiet)
    printf(" ** Running: %s\n",command);

  i=0;
  pip=popen(command,"r");
  while (fgets(line,1024,pip)!=NULL) {
    sscanf(line, "%d %s %s %s %s %s %d %d %lg %lg %f %d %d %lg %lg %lg %lg", 
	   &(tileinfo[i].id),tileinfo[i].tilename,tileinfo[i].runiddesc,tileinfo[i].nite,
	   tileinfo[i].band,tileinfo[i].imagename,&(tileinfo[i].ccdnum),&(tileinfo[i].imageid),
	   &(tileinfo[i].ra),&(tileinfo[i].dec),&(tileinfo[i].pixelsize),
	   &(tileinfo[i].npix_ra),&(tileinfo[i].npix_dec),
	   &(tileinfo[i].ra_lo),&(tileinfo[i].ra_hi),&(tileinfo[i].dec_lo),&(tileinfo[i].dec_hi));
    i++;
  }
  pclose(pip);

  /* output the swarp calls */
  fout=fopen("swarp.cmd", "w");
  if(flag_binpath) fprintf(fout,"%s/swarp ",binpath);
  else fprintf(fout,"swarp ");
  if(flag_etcpath) fprintf(fout,"-c %s/default.swarp ",etcpath);
  else fprintf(fout,"-c default.swarp ");

  /* cycle through the images */
  j=1;
  for(i=0;i<nimage;i++) {

    /* set the full path to the image */
    imgfullpath[0]=0;
    sprintf(imgfullpath, "%s/%s/data/%s/%s/%s",
	    basedir,tileinfo[i].runiddesc,tileinfo[i].nite,tileinfo[i].band,tileinfo[i].imagename);

    /* split the MEF images */
    if(!flag_ccd) {
      if(!flag_quiet) printf(" ** %d\t splitting image %s\n",i+1,imgfullpath);
      if(flag_binpath) 
	sprintf(command,"%s/splitimage %s -quiet -output image%d > error", binpath,imgfullpath, i+1);
      else
	sprintf(command,"splitimage %s -quiet -output image%d > error", imgfullpath,i+1);
      system(command);
    
      /* check error message: check if the fits file exist or not */
      ferr=fopen("error","r");
      fscanf(ferr,"%s",err);
      if (strcmp(err,"writing")) 
	if(!flag_quiet) printf(" ** quick_coadd error: file %s not found\n",imgfullpath);
      fclose(ferr);
    }
    else {
      /* split the images match with input ccdnum_list */
      for(k=0;k<nccd;k++) {
	if(tileinfo[i].ccdnum == ccd_num[k]) {
	  if(!flag_quiet) printf(" ** %d\t splitting image %s\n",j,imgfullpath);
	  if(flag_binpath) 
	    sprintf(command,"%s/splitimage %s -quiet -output image%d > error", binpath,imgfullpath, j);
	  else
	    sprintf(command,"splitimage %s -quiet -output image%d > error", imgfullpath,j);
	  system(command);
      
	  ferr=fopen("error","r");
	  fscanf(ferr,"%s",err);
	  if (strcmp(err,"writing")) 
	    if(!flag_quiet) printf(" ** quick_coadd error: file %s not found\n",imgfullpath);
	  fclose(ferr);
	  j++;
	}
      }
    }
  }

  fprintf(fout, "*_im.fits ");

  /* assume all pixscale,tilera,tiledec etc are all the same at the moment */
  fprintf(fout, "-PIXELSCALE_TYPE MANUAL -PIXEL_SCALE %2.5f ",tileinfo[0].pixelsize);
  fprintf(fout, "-CENTER_TYPE MANUAL -CENTER %3.7f,%3.7f ",tileinfo[0].ra,tileinfo[0].dec);
  fprintf(fout, "-IMAGE_SIZE %d,%d ",tileinfo[0].npix_ra,tileinfo[0].npix_dec);
  fprintf(fout, "-SUBTRACT_BACK Y ");
  fprintf(fout, "-DELETE_TMPFILES Y ");
  fprintf(fout, "-RESAMPLE N ");
  fprintf(fout, "-COMBINE Y ");
  if(!flag_combinetype) fprintf(fout,"-COMBINE_TYPE WEIGHTED ");
  else fprintf(fout,"-COMBINE_TYPE %s ", combinetype);
  fprintf(fout, "-IMAGEOUT_NAME %s ",output);
  len=strlen(output);
  for (j=len;j>0;j--) {
    if (!strncmp(&(output[j]),".fits",5)) 
      output[j]=0;
  }
  fprintf(fout, "-WEIGHTOUT_NAME %s.weight.fits ",output);
  fprintf(fout, "-HEADER_ONLY N ");
  fprintf(fout, "\n");
  fclose(fout);

  /* run the shell script */
  system ("csh swarp.cmd");

  /* clean up */
  sprintf(command, "rm image*.fits swarp.cmd error");
  system(command);

  
  /* free memory alloc */
  cfree(tileinfo); 
  if(flag_ccd) cfree(ccd_num);

  if(!flag_quiet) printf(" ** End quick_coadd \n");
}
