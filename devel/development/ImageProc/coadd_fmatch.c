#include "imageproc.h"

#define TOLERANCE (2.0/60.0) /* arcmin */

main(argc,argv)
        int argc;
        char *argv[];
{
  char tilelist[1000],tile1[100],tile2[100];
  char runid[100],band[5],sqlcall[1000],line[1000];
  char outname[500],dblogin[500];

  int i,j,len;
  int imageid1,imageid2;
  int flag_quiet=0;

  FILE *fsql,*pip;
  void select_dblogin();

  if (argc<2) {
    printf("  coadd_grabmatches <tile1,tile2> -runid <runid> \n");
    printf("  Options:\n");
    printf("     -band <g,r,i,z> (default is g-band)\n");
    printf("     -outname <output name> (default is g-band)\n");
    printf("     -quiet\n");
    exit(0);
  }

  sprintf(tilelist,"%s",argv[1]);

  /* defaults */
  sprintf(band,"%s","g");
  sprintf(outname,"%s","coadd_fmatch.dat");
  
  /* get tile1 and tile2 to match */
  len=strlen(tilelist);
  for (j=len;j>0;j--) {
    if (!strncmp(&(tilelist[j]),",",1)) {
      tilelist[j]=32;
      break;
    }
  }
  sscanf(tilelist,"%s %s",tile1,tile2);
  
  /*  process the rest of the command line */
  for (i=2;i<argc;i++) {
    if (!strcmp("-quiet",argv[i])) flag_quiet=1;
    if (!strcmp("-band",argv[i])) {
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -band option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else 
	sprintf(band,"%s",argv[i+1]);
    }
    if (!strcmp("-runid",argv[i])) {
      if(argv[i+1]==NULL) {
	printf(" ** %s ERROR: input for -runid option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else 
      sprintf(runid,"%s",argv[i+1]);
    }
    if (!strcmp("-outname",argv[i])) {
      if(argv[i+1]==NULL) {
	printf(" ** %s ERROR: input for -outname option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else 
	sprintf(outname,"%s",argv[i+1]);
    }
  }

  /* grab the dblogin */
  select_dblogin(dblogin);
  
  /* construct sql call to get imageids */
  sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < coadd_imageid.sql",dblogin);

  fsql=fopen("coadd_imageid.sql","w");
  fprintf(fsql,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsql,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsql,"SELECT imageid from files where tilename='%s' and imagetype='coadd' and band='%s' and runiddesc like '%%%s%%';\n",tile1,band,runid);
  fprintf(fsql,"SELECT imageid from files where tilename='%s' and imagetype='coadd' and band='%s' and runiddesc like '%%%s%%';\n",tile2,band,runid);
  fprintf(fsql,"exit\n");
  fclose(fsql);

  pip=popen(sqlcall,"r");
  fgets(line,1000,pip);
  sscanf(line,"%d",&imageid1);
  fgets(line,1000,pip);
  sscanf(line,"%d",&imageid2);
  pclose(pip);

  system ("rm coadd_imageid.sql");

  /* construct Dora/Patrick fmatch call */
  sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < coadd_fmatch.sql",dblogin);

  fsql=fopen("coadd_fmatch.sql","w");
  fprintf(fsql,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsql,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsql,"SPOOL %s;\n",outname);
  fprintf(fsql,"SELECT ALPHA_J2000_1,DELTA_J2000_1");
  if(!strcmp(band,"g")) fprintf(fsql,",MAG_AUTO_G_1,MAGERR_AUTO_G_1");
  if(!strcmp(band,"r")) fprintf(fsql,",MAG_AUTO_R_1,MAGERR_AUTO_R_1");
  if(!strcmp(band,"i")) fprintf(fsql,",MAG_AUTO_I_1,MAGERR_AUTO_I_1");
  if(!strcmp(band,"z")) fprintf(fsql,",MAG_AUTO_Z_1,MAGERR_AUTO_Z_1");
  fprintf(fsql,",ALPHA_J2000_2,DELTA_J2000_2");
  if(!strcmp(band,"g")) fprintf(fsql,",MAG_AUTO_G_2,MAGERR_AUTO_G_2");
  if(!strcmp(band,"r")) fprintf(fsql,",MAG_AUTO_R_2,MAGERR_AUTO_R_2");
  if(!strcmp(band,"i")) fprintf(fsql,",MAG_AUTO_I_2,MAGERR_AUTO_I_2");
  if(!strcmp(band,"z")) fprintf(fsql,",MAG_AUTO_Z_2,MAGERR_AUTO_Z_2");
  fprintf(fsql," from table(fMatchImages_coadd(%ld,%ld,%2.4f));\n",imageid1,imageid2,TOLERANCE);
  fprintf(fsql,"SPOOL OFF;\n");
  fprintf(fsql,"exit\n");
  fclose(fsql);

  system(sqlcall);
  system("rm coadd_fmatch.sql");

}

#undef TOLERANCE
