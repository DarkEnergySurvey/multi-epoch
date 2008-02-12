#include "imageproc.h" 

#define VERSION 1.0

main(argc,argv)
     int argc;
     char *argv[];
{
  char   dblogin[500],sqlcall[1000],command[1000],band[2],nite[100],runid[200];
  int    i,j,k,bandkey;
  int    flag_nite=0,flag_runid=0,flag_quiet=0,flag_band=0,flag_ra=0,flag_dec=0,flag_mag=0;
  float  maglow,maghigh,tmpf;
  double ralow,rahigh,declow,dechigh,tmpd;
  FILE   *fout,*smscript;
  void   select_dblogin();

  if (argc<2) {
    printf("Usage: skycoverage -nite <nite> -runid <runid> -ra <low(in degree)> <high(in degree)> -dec <low(in degree)> <high(in degree)> -band <band(g,r,i,z)> -mag <low> <high>\n");
    printf("                OPTION:\n");
    printf("                  -version\n");
    printf("                  -quiet\n");
    exit(0);
  }

  /* process the command line */
  for (i=1;i<argc;i++) {
    if (!strcmp(argv[i],"-nite"))  {
      flag_nite=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s error: input for -nite option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {     
        sprintf(nite,"%s",argv[i+1]);
        if (!strncmp(&nite[0],"-",1)) {
          printf(" ** %s error: wrong input for <nite>\n",argv[0]);
          exit(0);
        }
      }
    }

    if (!strcmp(argv[i],"-runid"))  {
      flag_runid=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s error: input for -runid option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {     
        sprintf(runid,"%s",argv[i+1]);
        if (!strncmp(&runid[0],"-",1)) {
          printf(" ** %s error: wrong input for <runid>\n",argv[0]);
          exit(0);
        }
      }
    }

    if (!strcmp(argv[i],"-band"))  {
      flag_band=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s error: input for -band option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {     
        sprintf(band,"%s",argv[i+1]);
        if (!strncmp(&band[0],"-",1)) {
          printf(" ** %s error: wrong input for <band>\n",argv[0]);
          exit(0);
        }
      }
    }
     
    if (!strcmp(argv[i],"-ra"))  {
      flag_ra=1;
      if(argv[i+1]==NULL || argv[i+2]==NULL) {
        printf(" ** %s error: input for -ra option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {     
        sscanf(argv[i+1],"%lg",&ralow);
	sscanf(argv[i+2],"%lg",&rahigh);
      }
    }

    if (!strcmp(argv[i],"-dec"))  {
      flag_dec=1;
      if(argv[i+1]==NULL || argv[i+2]==NULL) {
        printf(" ** %s error: input for -dec option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {     
        sscanf(argv[i+1],"%lg",&declow);
	sscanf(argv[i+2],"%lg",&dechigh);
      }
    }

    if (!strcmp(argv[i],"-mag"))  {
      flag_mag=1;
      if(argv[i+1]==NULL || argv[i+2]==NULL) {
        printf(" ** %s error: input for -mag option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {     
        sscanf(argv[i+1],"%f",&maglow);
	sscanf(argv[i+2],"%f",&maghigh);
      }
    }

    /* option */
    if (!strcmp(argv[i],"-version")) {
      printf("%s Version %2.2f\n",argv[0],VERSION);
      exit(0);
    }

    if (!strcmp(argv[i],"-quiet"))  
      flag_quiet=1;
  }

  /* make sure necesary inputs are set */
  if(!flag_nite) { printf(" ** %s error: -nite is not set, abort\n", argv[0]); exit(0); }
  if(!flag_runid) { printf(" ** %s error: -runid is not set, abort\n", argv[0]); exit(0); }
  if(!flag_band) { printf(" ** %s error: -band is not set, abort\n", argv[0]); exit(0); }
  if(!flag_ra) { printf(" ** %s error: -ra is not set, abort\n", argv[0]); exit(0); }
  if(!flag_dec) { printf(" ** %s error: -dec is not set, abort\n", argv[0]); exit(0); }
  if(!flag_mag) { printf(" ** %s error: -mag is not set, abort\n", argv[0]); exit(0); }

  /* make sure the orders are correct */
  if(ralow>rahigh) {tmpd=ralow;ralow=rahigh;rahigh=tmpd;}
  if(declow>dechigh) {tmpd=declow;declow=dechigh;dechigh=tmpd;}
  if(maglow>maghigh) {tmpf=maglow;maglow=maghigh;maghigh=tmpf;}


  /* grab dblogin */
  select_dblogin(dblogin);

  /* sql script here */
  fout=fopen("skycoverage.sql","w");
  fprintf(fout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fout,"SET TERMOUT OFF;\n");
  fprintf(fout,"select objects.alpha_j2000,objects.delta_j2000 from files,objects where ");
  fprintf(fout,"files.nite='%s' and files.runiddesc like '%s%%' and files.band='%s' ",nite,runid,band);
  fprintf(fout,"and files.imageid=objects.imageid ");
  fprintf(fout,"and (objects.alpha_j2000 between %2.8f and %2.8f) and (objects.delta_j2000 between %2.8f and %2.8f) and (objects.mag_auto between %2.3f and %2.3f) ",ralow,rahigh,declow,dechigh,maglow,maghigh);
  fprintf(fout,"order by objects.alpha_j2000;\n");
  fprintf(fout,"exit"); 
  fclose(fout);

  /* sql call */
  sprintf(sqlcall,"sqlplus -S %s < skycoverage.sql > skycoverage.dat",dblogin);
  system(sqlcall);

  /* poorman's plotting for the histogram, replace with PLPLOT etc later */
  smscript=fopen("plot.sm","w");
  fprintf(smscript,"pt\n");
  sprintf(command,"\tdevice postencap %s_%s_skycoverage.eps\n",nite,runid);
  //sprintf(command,"\tdevice postencap %s_%s_skycoverage_%2.2f_%2.2f_",nite,runid,ralow,rahigh);
  //if(declow<0.0)
  //sprintf(command,"%sn%2.2f_",command,fabs(declow));
  //else
  //sprintf(command,"%s%2.2f_",command,declow);
  //if(dechigh<0.0)
  //sprintf(command,"%sn%2.2f_",command,fabs(dechigh));
  //else
  //sprintf(command,"%s%2.2f_",command,dechigh);  
  //sprintf(command,"%s%2.1f_%2.1f.eps\n",command,maglow,maghigh);
  fprintf(smscript,"%s",command);
  fprintf(smscript,"\tdata skycoverage.dat\n");
  fprintf(smscript,"\tread \{ra 1 dec 2\}\n");
  fprintf(smscript,"\texpand 1.0\n");
  fprintf(smscript,"\tlimits ra dec\n");
  fprintf(smscript,"\tbox\n");
  fprintf(smscript,"\texpand 0.5\n");
  fprintf(smscript,"\tptype 4 0\n");
  fprintf(smscript,"\tpoints ra dec\n");
  fprintf(smscript,"\texpand 1.25\n");
  fprintf(smscript,"\tylabel RA\n");
  fprintf(smscript,"\txlabel DEC\n");
  fprintf(smscript,"\thardcopy\n");
  fprintf(smscript,"\tquit\n");
  fclose(smscript);

  /* run sm to get plot */
  system("sm macro read plot.sm pt > tmp");

  /* clean up */
  system("rm skycoverage.sql skycoverage.dat plot.sm tmp");

}

#undef VERSION

