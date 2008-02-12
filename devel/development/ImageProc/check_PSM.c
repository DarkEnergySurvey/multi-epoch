#include "imageproc.h" 

#define VERSION 1.01
/* REVISION HISTORY 

1.00 - Initial Version
1.01 - Add output of CCD without PSM solution

*/

main(argc,argv)
     int argc;
     char *argv[];
{
  char  dblogin[500],sqlcall[1000],command[1000],nite[100],runid[200],band[2];
  char  date[20],time[20],apm[2],**timestamp;
  int   i,j,kk,nccd,ccd,bandstart,bandstop,nband,photflag,dof,*ccdin;
  int   flag_quiet=0,flag_runid=0,flag_nite=0,flag_band=0;
  float k,kerr,rms,chi2,a,aerr,min,max;
  FILE  *fdat,*fout,*smscript,*pip;
  void  getband(),select_dblogin();

  if (argc<2) {
    printf("Usage: check_PSM -nite <nite> -runid <runid>\n");
    printf("                OPTION:\n");
    printf("                  -band <band (g,r,i,z)>\n");
    printf("                  -version\n");
    printf("                  -quiet\n");
    exit(0);
  }

  /* set the default values */
  bandstart=GBAND;
  bandstop=ZBAND;

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

    if (!strcmp(argv[i],"-version")) {
      printf("check_PSM Version %2.2f\n",VERSION);
      exit(0);
    }
    
    if (!strcmp(argv[i],"-quiet"))  
      flag_quiet=1;
  }

  /* make sure -nite is set */
  if(!flag_nite) { printf(" ** %s error: -nite is not set, abort\n", argv[0]); exit(0); }
  if(!flag_runid) { printf(" ** %s error: -runid is not set, abort\n", argv[0]); exit(0); }

  /* grab dblogin */
  select_dblogin(dblogin);

  /* first query the psmfit table to find out how many ccds */
  fout=fopen("psm_ccd.sql","w");
  fprintf(fout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fout,"SET TERMOUT OFF;\n");
  fprintf(fout,"select count(unique(ccd_number)) from files where nite='%s' and runiddesc like '%s%%';\n",nite,runid);
  fprintf(fout,"exit\n");
  fclose(fout);

  sprintf(sqlcall,"sqlplus -S %s < psm_ccd.sql",dblogin);
  pip=popen(sqlcall,"r");
  fscanf(pip,"%d",&nccd);
  pclose(pip);

  ccdin=(int *)calloc(nccd+1,sizeof(int *));

  /* query the psmfit table to find out latest timestamp in each bands */
  /* not sure how to link the PSMFIT to FILES table */
  timestamp=(char **)calloc(bandstop+1,sizeof(char *));
  for(j=0;j<=bandstop;j++) timestamp[j]=(char *)calloc(100,sizeof(char ));

  if(!flag_quiet) {
    printf("=======================================================================================\n");
    printf("band\tK\tKerr\tRMS\tCHI2\tDOF\tPHOTOMETRICFLAG\tTIMESTAMP\n");
    printf("---------------------------------------------------------------------------------------\n");
  }
  nband=0; min=0.0;max=-100;
  for(j=bandstart;j<=bandstop;j++) {
    /* get the band */
    getband(j,band);

    /* construct sql for timestamp */
    fout=fopen("psm_timestamp.sql","w");
    fprintf(fout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
    fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    fprintf(fout,"SET TERMOUT OFF;\n");
    fprintf(fout,"select distinct(FIT_TIMESTAMP) from psmfit where nite='%s' and FILTER='%s' order by FIT_TIMESTAMP;\n",nite,band);
    fprintf(fout,"exit\n");
    fclose(fout);

    /* get latest timestamp */
    sprintf(sqlcall,"sqlplus -S %s < psm_timestamp.sql",dblogin);
    pip=popen(sqlcall,"r");
    while(fscanf(pip,"%s %s %s",date,time,apm)!=EOF) 
      sprintf(timestamp[j],"%s %s %s",date,time,apm);
    pclose(pip);

    /* construct sql for o get K,chi2,dof,rms,photometricflag */
    fout=fopen("psm.sql","w");
    fprintf(fout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
    fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    fprintf(fout,"SET TERMOUT OFF;\n");
    fprintf(fout,"select distinct(K) from psmfit where nite='%s' and FILTER='%s' and fit_timestamp='%s';\n",nite,band,timestamp[j]);
    fprintf(fout,"select distinct(KERR) from psmfit where nite='%s' and FILTER='%s' and fit_timestamp='%s';\n",nite,band,timestamp[j]);
    fprintf(fout,"select distinct(RMS) from psmfit where nite='%s' and FILTER='%s' and fit_timestamp='%s';\n",nite,band,timestamp[j]);
    fprintf(fout,"select distinct(CHI2) from psmfit where nite='%s' and FILTER='%s' and fit_timestamp='%s';\n",nite,band,timestamp[j]);
    fprintf(fout,"select distinct(DOF) from psmfit where nite='%s' and FILTER='%s' and fit_timestamp='%s';\n",nite,band,timestamp[j]);
    fprintf(fout,"select distinct(PHOTOMETRICFLAG) from psmfit where nite='%s' and FILTER='%s' and fit_timestamp='%s';\n",nite,band,timestamp[j]);
    fprintf(fout,"exit\n");
    fclose(fout);

    /* get the info */
    sprintf(sqlcall,"sqlplus -S %s < psm.sql",dblogin);
    pip=popen(sqlcall,"r");
    fscanf(pip,"%f %f %f %f %d %d",&k,&kerr,&rms,&chi2,&dof,&photflag); 
    pclose(pip);
    
    printf("%s\t%2.4f\t%2.4f\t%2.4f\t%2.4f\t%d\t%d\t\t%s\n",band,k,kerr,rms,chi2,dof,photflag,timestamp[j]); 

    /* another query for the A term for each ccd */
    fout=fopen("psm_zp.sql","w");
    fprintf(fout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
    fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    fprintf(fout,"SET TERMOUT OFF;\n");
    fprintf(fout,"select CCDID,A,AERR from psmfit where nite='%s' and FILTER='%s' and fit_timestamp='%s' order by ccdid;\n",nite,band,timestamp[j]);
    fprintf(fout,"exit\n");
    fclose(fout);

    /* run the query and get the data */
    sprintf(sqlcall,"sqlplus -S %s < psm_zp.sql",dblogin);
    sprintf(command,"psm_%s.dat",band);
    fdat=fopen(command,"w");
    pip=popen(sqlcall,"r");
    while(fscanf(pip,"%d %f %f",&ccd,&a,&aerr)!=EOF) {
      fprintf(fdat,"%d\t%2.5f\t%2.5f\n",ccd,a,aerr);
      if(a>max) max=a;
      if(a<min) min=a;
    }
    pclose(pip);
    fclose(fdat);

    nband++;
  }
  if(!flag_quiet) 
    printf("---------------------------------------------------------------------------------------\n");

  /* check which CCD do not have solution */
  for(j=bandstart;j<=bandstop;j++) {

    /* get the band */
    getband(j,band);
    
    /* normalize the vector */
    for(i=1;i<=nccd;i++) 
      ccdin[i]=0;

    /* find out which CCD has solution */
    sprintf(command,"psm_%s.dat",band);
    fout=fopen(command,"r");
    while(fscanf(fout,"%d %f %f",&ccd,&a,&aerr)!=EOF) 
      ccdin[ccd]=1;
    fclose(fout);
    
  
    /* output those dont have solution */
    for(i=1;i<=nccd;i++) {
      if(!ccdin[i])
	printf(" ** For %s band, CCD %d does not have PSM solution **\n",band,i);
    } 
  }

  /* poorman's plotting for the ZP, replace with PLPLOT etc later */
  smscript=fopen("plot.sm","w");
  fprintf(smscript,"pt\n");
  fprintf(smscript,"\tdevice postencap %s_%s_ZP.eps\n",nite,runid);
  fprintf(smscript,"\tptype 4 0\n");
  kk=0;
  for(j=bandstop;j>=bandstart;j--) {
    /* get the band */
    getband(j,band);

    /* create sm script */
    fprintf(smscript,"\twindow -1 -%d 1 %d\n",nband,kk+1);
    fprintf(smscript,"\tdata psm_%s.dat\n",band);
    fprintf(smscript,"\tread \{ccd 1 a 2 aerr 3\}\n");
    fprintf(smscript,"\tlimits -2 %d %2.3f %2.3f\n",nccd+3,min-0.15,max+0.15);
    fprintf(smscript,"\tpoints ccd a\n");
    fprintf(smscript,"\terror_y ccd a aerr\n");
    fprintf(smscript,"\tylabel ZP(%s)\n",band);
    if(j==bandstop) {
      fprintf(smscript,"\txlabel CCD\n");
      fprintf(smscript,"\tbox\n");
    }
    else
      fprintf(smscript,"\tbox 0 2 0 0\n");

    kk++;
  }
  fprintf(smscript,"\thardcopy\n");
  fprintf(smscript,"\tquit\n");
  
  fclose(smscript);

  /* run sm to get plot */
  system("sm macro read plot.sm pt > tmp");
  
  /* clean up and free vector */
  free(timestamp); free(ccdin);

  system("rm psm*.sql psm*.dat plot.sm tmp");
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
