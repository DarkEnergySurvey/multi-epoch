#include "imageproc.h"

/*	Program selects src imagelists from db and creates
/*	several lists of similar length for the ingestion
/*	program
*/

main(argc,argv)
	int argc;
	char *argv[];
{
	char	nite[100],sqlcall[1000],sqlqueryfile[1000],fileroot[1000],
		filename[1000],imagename[100],line1[1000],
		dblogin[500];
	int	i,flag_quiet=0,numlists;	
	FILE	*out,*pip,**fileout;
	void	select_dblogin();

	if (argc<4) {
	  printf("makeingestlist <nite> <# lists> <output root>\n");
	  printf(" -quiet\n");
	  exit(0);
	}
	sprintf(nite,"%s",argv[1]);
	sscanf(argv[2],"%d",&numlists);
	sprintf(fileroot,"%s",argv[3]);
	for (i=4;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}

	select_dblogin(dblogin);

	sprintf(sqlqueryfile,"makeingestlist.sqlquery");
	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",
	  dblogin,sqlqueryfile);

	/* prepare query */
	out=fopen(sqlqueryfile,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' '");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        fprintf(out,"SELECT nite,imagename FROM FILES\n");
        fprintf(out,"WHERE nite='%s' and lower(imagetype)='src';\n",nite);
	fprintf(out,"exit;\n");
	fclose(out);

	/* prepare file pointers */
	fileout=(FILE **)calloc(numlists,sizeof(FILE));
	for (i=0;i<numlists;i++) {
	  sprintf(filename,"%s_%02d",fileroot,i+1);
	  fileout[i]=fopen(filename,"w");
	}

	i=0;
	pip=popen(sqlcall,"r");
	while (fgets(line1,1000,pip)!=NULL) {
	  if (!strncmp(line1,nite,strlen(nite))) {
	    sscanf(&line1[strlen(nite)],"%s",imagename);
	    fprintf(fileout[i%numlists],"raw/%s/src/%s.fits\n",nite,imagename);
	    if (!flag_quiet) {
	      line1[strlen(line1)-1]=0;
	      fprintf(stdout,"%s    %s/%s.fits   %s_%02d\n",
	      line1,nite,imagename,fileroot,i%numlists+1);
	    }
	    i++;
	  }
	}
	pclose(pip);

	/* close the files */
	for (i=0;i<numlists;i++) fclose(fileout[i]);
	system("rm makeingestlist.sqlquery");
	
}
