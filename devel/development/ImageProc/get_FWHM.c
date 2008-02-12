#include "imageproc.h" 

#define VERSION 1.0

main(argc,argv)
     int argc;
     char *argv[];
{
  char  dblogin[500],sqlcall[1000],command[1000],band[2],nite[100],runid[200],imtype[100],fname[100],temp[20];
  int   i,j,k,bandstart,bandstop,nband,ntotal,n;
  int   flag_nite=0,flag_runid=0,flag_quiet=0,flag_band=0,flag_imtype=0;
  float *indat,max,min,sig,mean,sum,binsize;
  FILE  *fin,*fout,*smscript,*pip;
  void  getband(),select_dblogin();
  
  if (argc<2) {
    printf("Usage: get_FWHM -nite <nite> -runid <runid>\n");
    printf("                OPTION:\n");
    printf("                  -band <band (g,r,i,z)>\n");
    printf("                  -imagetype <reduced or remap (default: reduced)>\n");
    printf("                  -version\n");
    printf("                  -quiet\n");
    exit(0);
  }

  /* set the default values */
  bandstart=GBAND;
  bandstop=ZBAND;
  sprintf(imtype,"%s","reduced");

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

    /* options */
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
	else {
	  if(!strcmp(band,"g")) {
	    bandstart=GBAND; bandstop=GBAND; }
	  else if(!strcmp(band,"r")) {
	    bandstart=RBAND; bandstop=RBAND; }
	  else if(!strcmp(band,"i")) {
	    bandstart=IBAND; bandstop=IBAND; }
	  else if(!strcmp(band,"z")) {
	    bandstart=ZBAND; bandstop=ZBAND; }
	  else {
	    printf(" ** %s error: wrong input band, abort!\n",argv[0]);
	    exit(0);
	  }
	}
      }
    }
     
    if (!strcmp(argv[i],"-imagetype"))  {
      flag_imtype=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s error: input for -imagetype option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {     
        sprintf(imtype,"%s",argv[i+1]);
        if (!strncmp(&imtype[0],"-",1) && strcmp(imtype,"-reduced") && strcmp(imtype,"remap")) {
          printf(" ** %s error: wrong input for <imagetype>\n",argv[0]);
          exit(0);
        }
      }
    }
 
    if (!strcmp(argv[i],"-version")) {
      printf("get_FWHM Version %2.2f\n",VERSION);
      exit(0);
    }

    if (!strcmp(argv[i],"-quiet"))  
      flag_quiet=1;
  }

  /* make sure -nite and -runid is set */
  if(!flag_nite) { printf(" ** %s error: -nite is not set, abort\n", argv[0]); exit(0); }
  if(!flag_runid) { printf(" ** %s error: -runid is not set, abort\n", argv[0]); exit(0); }

  /* grab dblogin */
  select_dblogin(dblogin);
 
  /* sql call here */
  nband=0;
  for(j=bandstart;j<=bandstop;j++) {
    /* get the band */
    getband(j,band);
    
    /* create sqlscript */
    fout=fopen("fwhm.sql","w");
    fprintf(fout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
    fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    fprintf(fout,"SET TERMOUT OFF;\n");
    fprintf(fout,"select MNSEEING from files where nite='%s' and runiddesc like '%s%%' and imagetype='%s' ",nite,runid,imtype);
    fprintf(fout,"and band='%s'",band);
    fprintf(fout,";\n");
    fprintf(fout,"exit"); 
    fclose(fout);
  
    /* set up sql call */
    sprintf(sqlcall,"sqlplus -S %s < fwhm.sql > fwhm_%s.dat",dblogin,band);
    system(sqlcall);

    nband++;
  }

  /* count the total data */
  ntotal=0;
  for(j=bandstart;j<=bandstop;j++) {
    /* get the band */
    getband(j,band);

    sprintf(command,"wc -l fwhm_%s.dat",band);
    pip=popen(command,"r");
    while (fscanf(pip,"%d %s",&n,temp)!=EOF) 
      ntotal+=n;
    pclose(pip);
  }

  /* memory allocation */
  indat=(float *)calloc(ntotal+1,sizeof(float));

  /* input the data and calculate mean */
  sum=0.0; k=1;
  for(j=bandstart;j<=bandstop;j++) {
    /* get the band */
    getband(j,band);
    
    sprintf(fname,"fwhm_%s.dat",band);
    fin=fopen(fname,"r");
    while(fscanf(fin,"%f",&(indat[k]))!=EOF) {
      sum+=indat[k];
      k++;
    }
    fclose(fin);
  }
  
  mean=sum/(float)ntotal;
 
  /* get the standard deviation and max/min */
  max=mean; min=mean; sum=0.0;
  for(k=1;k<=ntotal;k++) {
    sum+=Squ(indat[k]-mean);
    if(indat[k]>max) 
      max=indat[k];
    if(indat[k]<min)
      min=indat[k];
  }

  sig=sqrt(sum/(float)(ntotal-1));
 
  /* estimate binsize of histogram using Scott's method */
  binsize=3.5*sig/pow((float)ntotal,0.333);

  /* poorman's plotting for the histogram, replace with PLPLOT etc later */
  smscript=fopen("plot.sm","w");
  fprintf(smscript,"pt\n");
  fprintf(smscript,"\tdevice postencap %s_%s_FWHM.eps\n",nite,runid);
  k=0;
  for(j=bandstop;j>=bandstart;j--) {
    /* get the band */
    getband(j,band);

    /* create sm script */
    fprintf(smscript,"\twindow -1 -%d 1 %d\n",nband,k+1);
    fprintf(smscript,"\tdata fwhm_%s.dat\n",band);
    fprintf(smscript,"\tread \{x 1\}\n");
    fprintf(smscript,"\tget_hist x xx yy %2.3f %2.3f %2.5f\n",min,max,binsize);
    fprintf(smscript,"\tlimits xx yy\n");
    fprintf(smscript,"\thistogram xx yy\n");
    fprintf(smscript,"\tylabel N(%s)\n",band);
    if(j==bandstop) {
      fprintf(smscript,"\txlabel FWHM (arcsec)\n");
      fprintf(smscript,"\tbox\n");
    }
    else
      fprintf(smscript,"\tbox 0 2 0 0\n");

    k++;
  }
  fprintf(smscript,"\thardcopy\n");
  fprintf(smscript,"\tquit\n");
  
  fclose(smscript);

  /* run sm to get plot */
  system("sm macro read plot.sm pt > tmp");

  /* clean up and free vector */
  free(indat);

  system("rm fwhm.sql fwhm_*.dat plot.sm tmp");
}  

void getband(int index,char *band[])
{
  switch(index) {
  case GBAND: sprintf(band,"%s","g"); break;
  case RBAND: sprintf(band,"%s","r"); break;
  case IBAND: sprintf(band,"%s","i"); break;
  case ZBAND: sprintf(band,"%s","z"); break;
  }
}

#undef VERSION
