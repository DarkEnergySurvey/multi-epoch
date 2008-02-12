/* 

Revision:
1.00 -- original version
1.01 -- take out the flag_zp option because it is not necessary
     -- take out the part that inserting WCS info to the image header 
1.02 -- add MAP_WEIGHT to SExtractor call

*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "imageproc.h"   // change the path later

#define VERSION 1.02

main(argc,argv)
     int argc;
     char *argv[];
{
  char tilename[500],imagename[2000],weightname[2000],
       binpath[2000],etcpath[2000],archpath[1000],
       comment[1000],command[2000],sqlcall[1000],
       line[1000],bandlist[1000],paramfile[1000],
       dblogin[500],**band,arnode[100],arroot[1000],
       arsite[100];

  int flag_quiet=0,flag_binpath=0,flag_etcpath=0;
  int flag_proj=0,flag_tilename=0,flag_archpath=0;
  int flag_bandlist=0,flag_check=0;
  int flag_param=0,status=0;

  int i,j,k,npix_ra,npix_dec,len,bandnum;
  float pixscale,magzp;
  double ra,dec,crpix1,crpix2,crval1,crval2;
  time_t curtime=time (NULL),lsttime;

  FILE *pip,*fsqlout;
  fitsfile *fptr;
  void select_dblogin(),select_archivenode();

  if (argc<2) {
    printf("Usage: %s \n",argv[0]);
    printf("       -tilename <tilename>\n");
    printf("       -bandlist <list of bands> (in griz format) \n");

    printf("    Option:\n");
    printf("          -binpath <binpath>\n");
    printf("          -etcpath <etcpath>\n");
    printf("          -archive_node <archive node> \n");
    //printf("          -param <parameter filename> (default is sex.param) \n");
    printf("          -quiet\n");
    exit(0);
  }

  /* process the command line */
  for (i=1;i<argc;i++) {
    if (!strcmp(argv[i],"-tilename"))  {
      flag_tilename=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -tilename option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(tilename,"%s",argv[i+1]);
	if (!strncmp(&tilename[0],"-",1)) {
	  printf(" ** %s error: wrong input of <tilename>\n",argv[0]);
	  exit(0);
	}
      }
    }
    
    if (!strcmp(argv[i],"-bandlist"))  {
      flag_bandlist=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -bandlist option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(bandlist,"%s",argv[i+1]);
	if (!strncmp(&bandlist[0],"-",1)) {
	  printf(" ** %s error: wrong input of <bandlist>\n",argv[0]);
	  exit(0);
	}
      }
    }
    
    /* options */
    if (!strcmp(argv[i],"-archive_node"))  {
      flag_archpath=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -archive_node option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else  sprintf(arnode,"%s",argv[i+1]);
    }    

    if (!strcmp(argv[i],"-binpath"))  {
      flag_binpath=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -binpath option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(binpath,"%s",argv[i+1]);
	if (!strncmp(&binpath[0],"-",1)) {
	  printf(" ** %s error: wrong input of <binpath>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-etcpath"))  {
      flag_etcpath=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -etcpath option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(etcpath,"%s",argv[i+1]);
	if (!strncmp(&etcpath[0],"-",1)) {
	  printf(" ** %s error: wrong input of <etcpath>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-param"))  {
      flag_param=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -param option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(paramfile,"%s",argv[i+1]);
	if (!strncmp(&paramfile[0],"-",1)) {
	  printf(" ** %s error: wrong input of <parameter file>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
  }

  /* print out the time of processing */
  if(!flag_quiet)
    printf("\n ** Running %s (Version %2.2f) on %s \n",argv[0],VERSION,asctime(localtime (&curtime)));

  if(!flag_tilename) { printf(" ** %s error: -tilename is not set, abort!\n", argv[0]); flag_check=1; }
  if(!flag_bandlist) { printf(" ** %s error: -bandlist is not set, abort!\n", argv[0]); flag_check=1; }
  if(flag_check) exit(0);

  /* construct band vector */
  bandnum=strlen(bandlist);

  band=(char **)calloc(bandnum,sizeof(char *));
  for(i=0;i<bandnum;i++) band[i]=(char *)calloc(2,sizeof(char ));

  for(i=0;i<bandnum;i++)
    strncpy(band[i],&bandlist[i],1);

  /* grab dblogin */
  select_dblogin(dblogin);
  /* grab archive root */
  select_archivenode(dblogin,arnode,arroot,arsite);

  /* construct sql call to get tileinfo */
  sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < coadd.sql",dblogin);  

  fsqlout=fopen("coadd.sql", "w");
  fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsqlout,"SELECT RA,DEC,NPIX_RA,NPIX_DEC,PIXELSIZE ");
  fprintf(fsqlout,"FROM coaddtile WHERE ");
  fprintf(fsqlout,"tilename=\'%s\';\n",tilename);
  fprintf(fsqlout,"exit;\n");
  fclose(fsqlout);

  pip=popen(sqlcall, "r");
  fgets(line,1000,pip); 
  sscanf(line,"%lg %lg %d %d %f",&ra,&dec,&npix_ra,&npix_dec,&pixscale); 
  pclose(pip);

  system("rm coadd.sql");
  
  if(flag_archpath) {
    for(i=0;i<bandnum;i++) {

      /* assume the coadd filename is tile_band.fits with tile_band.weight.fits */
      /* cp to local working dir */
      
      sprintf(command,"cp %s/%s_%s.fits* .",arroot,tilename,band[i]);
      system(command);
      sprintf(command,"cp %s/%s_%s.weight.fits* .",arroot,tilename,band[i]);
      system(command);
    }
  }

  /* construct swarp call and run swarp */
  if(flag_binpath) sprintf(command,"%s/swarp ",binpath);
  else sprintf(command,"swarp ");

  for(i=0;i<bandnum;i++) {
      if(!i)
	  sprintf(command,"%s %s_%s.fits",command,tilename,band[i]);
      else
	  sprintf(command,"%s,%s_%s.fits",command,tilename,band[i]);
  }

  if(flag_etcpath) sprintf(command,"%s -c %s/default.swarp ",command,etcpath);
  else sprintf(command,"%s -c default.swarp ",command);

  sprintf(command,"%s -IMAGEOUT_NAME %s_chi2.fits ",command,tilename);
  sprintf(command,"%s -RESAMPLE N -COMBINE Y -COMBINE_TYPE CHI2 ",command);
  sprintf(command,"%s -SUBTRACT_BACK Y ",command);
  sprintf(command,"%s -DELETE_TMPFILES Y ",command);
  sprintf(command,"%s -WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE ",command);
  for(i=0;i<bandnum;i++) {
      if(!i)
	  sprintf(command,"%s %s_%s.weight.fits",command,tilename,band[i]);
      else
	  sprintf(command,"%s,%s_%s.weight.fits",command,tilename,band[i]);
  }
  sprintf(command,"%s ",command);
  sprintf(command,"%s -IMAGE_SIZE %d,%d -PIXELSCALE_TYPE MANUAL -PIXEL_SCALE %2.3f ",command,npix_ra,npix_dec,pixscale); 
  sprintf(command,"%s -CENTER_TYPE MANUAL -CENTER %3.8f,%3.8f ",command,ra,dec);
  sprintf(command,"%s -HEADER_ONLY N ",command);
  sprintf(command,"%s -WRITE_XML N ",command);
  if(flag_quiet)
    sprintf(command,"%s -VERBOSE_TYPE QUIET ",command);
  
  if(!flag_quiet)
    printf("%s\n",command);

  system(command);
  //system("rm coadd.weight.fits");

  /* construct the SExtractor call and run */
  for(i=0;i<bandnum;i++) {

    /* grep SEXMAGZP keyword */
    sprintf(imagename,"%s_%s.fits",tilename,band[i]);
    sprintf(weightname,"%s_%s.weight.fits",tilename,band[i]);
    printf(" ** ZP for %s = ",imagename);
    status=0;
    if(fits_open_file(&fptr,imagename,READONLY,&status)) printerror(status);
    if(fits_read_key_flt(fptr,"SEXMGZPT",&magzp,comment,&status)==KEY_NO_EXIST) {
      status=0;
      magzp=25.0; /* use dummy zeropoint */
    }
    if(fits_close_file(fptr,&status)) printerror(status);
    printf("%2.4f\n",magzp);

    /* SExtractor */
    if(flag_binpath) sprintf(command,"%s/SExtractor ",binpath);
    else sprintf(command,"SExtractor ");

    /* run in double-image mode */
    sprintf(command,"%s %s_chi2.fits,%s_%s.fits ",command,tilename,tilename,band[i]);

    if(flag_etcpath) sprintf(command,"%s -c %s/sex.config ",command,etcpath);
    else sprintf(command,"%s -c sex.config ",command);
    
    if(flag_etcpath) 
      sprintf(command, "%s -FILTER_NAME %s/sex.conv -STARNNW_NAME %s/sex.nnw -PARAMETERS_NAME %s/sex.param ", 
	      command,etcpath,etcpath,etcpath);
    else
      sprintf(command, "%s -FILTER_NAME sex.conv -STARNNW_NAME sex.nnw -PARAMETERS_NAME sex.param ",command);

    /* take out at the moment */
    //sprintf(command,"%s -PARAMETERS_NAME ",command);
    //sprintf(command,"%s %s ",command,paramfile);
    //if(flag_param) {
    //if(flag_etcpath) sprintf(command,"%s %s/%s ",command,etcpath,paramfile);
    //else sprintf(command,"%s %s ",command,paramfile);
    //}
    //else {
    //if(flag_etcpath) sprintf(command,"%s %s/sex.param ",command,etcpath);
    //else sprintf(command,"%s sex.param ",command);
    //}

    sprintf(command,"%s -CATALOG_TYPE FITS_1.0 -CATALOG_NAME %s_%s_cat.fits ",command,tilename,band[i]);
    sprintf(command,"%s -WEIGHT_TYPE NONE,MAP_WEIGHT -WEIGHT_IMAGE %s -PIXEL_SCALE %2.3f -MEMORY_BUFSIZE 2046 ",command,weightname,pixscale);

    sprintf(command,"%s -MAG_ZEROPOINT %2.4f ",command,magzp);
    
    if(flag_quiet)
      sprintf(command,"%s -VERBOSE_TYPE QUIET ",command);
    else
      sprintf(command,"%s -VERBOSE_TYPE NORMAL ",command);

    if(!flag_quiet)
      printf("%s\n",command);
    
    system(command);
  }

  /* free memory */
  free(band);

  lsttime=time (NULL);
  if(!flag_quiet)
      printf("\n ** Done on %s \n",asctime(localtime (&lsttime)));
}

#undef VERSION
