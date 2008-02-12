#include "imageproc.h" 

#define VERSION 1.0

main(argc,argv)
     int argc;
     char *argv[];
{
  char  dblogin[500],sqlcall[1000],command[1000],imname[100],nite[100],runid[200],tmp[10],**imagename;
  int   i,j,k,kstart,nimage,scampflag,scampnum;
  int   flag_nite=0,flag_runid=0,flag_quiet=0,match=0;
  float scampchi;
  FILE  *fin,*fout,*pip;
  void  select_dblogin();

  if (argc<2) {
    printf("Usage: check_scamp -nite <nite> -runid <runid>\n");
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

    /* option */
    if (!strcmp(argv[i],"-version")) {
      printf("check_scamp Version %2.2f\n",VERSION);
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

  /* create sqlscript to get imagename from reduced image */
  fout=fopen("check_scamp_imagename.sql","w");
  fprintf(fout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fout,"SET TERMOUT OFF;\n");
  fprintf(fout,"select distinct(imagename) from files where nite='%s' and runiddesc like '%s%%' and imagetype='reduced' order by imagename;\n ",nite,runid);
  fprintf(fout,"exit"); 
  fclose(fout);

  /* set up sql call */
  sprintf(sqlcall,"sqlplus -S %s < check_scamp_imagename.sql > image.list",dblogin);
  system(sqlcall);

  /* count how many imagename and stored them */
  sprintf(command,"wc -l image.list");
  pip=popen(command,"r");
  fscanf(pip,"%d %s",&nimage,tmp);
  pclose(pip);

  imagename=(char **)calloc(nimage+1,sizeof(char *));
  for(j=1;j<=nimage;j++) imagename[j]=(char *)calloc(100,sizeof(char ));

  fin=fopen("image.list","r");
  for(i=1;i<=nimage;i++)
    fscanf(fin,"%s",imagename[i]);
  fclose(fin);

  /* create sqlscript for each imagename */
  fout=fopen("check_scamp.sql","w");
  fprintf(fout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fout,"SET TERMOUT OFF;\n");
  fprintf(fout,"select distinct(scampnum),imagename,scampflg,scampchi from files where nite='%s' and runiddesc like '%s%%' and imagetype='remap' and scampnum>0 order by imagename;\n ",nite,runid);
  fprintf(fout,"exit"); 
  fclose(fout);

  /* set up sql call */
  sprintf(sqlcall,"sqlplus -S %s < check_scamp.sql ",dblogin);

  /* read in the result and printout */
  printf("IMAGENAME\t\tSCAMPFLG\tSCAMPNUM\tSCAMPCHI\n");
  pip=popen(sqlcall,"r");
  kstart=1;
  while (fscanf(pip,"%d %s %d %f",&scampnum,imname,&scampflag,&scampchi)!=EOF) {

    /* check the imagename with stored imagename from reduced images */
    for(k=kstart;k<=nimage;k++) {
       match=0;
      if(!strcmp(imagename[k],imname) && scampnum>0) {
	if(strlen(imname)>15)
	  printf("%s\t%d\t\t%d\t\t%2.2f\n",imname,scampflag,scampnum,scampchi);
	else
	  printf("%s\t\t%d\t\t%d\t\t%2.2f\n",imname,scampflag,scampnum,scampchi);
	kstart=k+1;
	match=1;
	break;
      }

      if(!match) {
	if(strlen(imagename[k])>15)
	  printf("%s\t-\t\t---\t\t---\n",imagename[k]);
	else
	  printf("%s\t\t-\t\t---\t\t---\n",imagename[k]);
      }
    }
  }
  pclose(pip);
  
  /* clean up  */
  //system("rm check_scamp.sql check_scamp_imagename.sql image.list");
  
  free(imagename);
}

#undef VERSION
