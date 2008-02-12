#include "imageproc.h"

/*  For quick ingestion of src images
/*	nite
/*	imagetype
/*	imagename
*/

#define MAXNODES 12

main(argc, argv)
	int argc;
	char *argv[];
{

	char	archive[500],nite[100],imagename[100],command[1000],
		archivenode[100],trash[1000],filename[500],
		arnode[100],arroot[100],arsite[100],
		imagetype[100],sqlcall[500],sqlquery[500],line[1000],
		image[1000],imageclass[100],srcstring[500],dblogin[500];
	int	i,count=0,flag_quiet=0,n;
	FILE	*pip,*out;
	void	select_dblogin(),select_archivenode();

	if (argc<3) {
	  printf("  SPT_ingest <nite> <archive node>\n");
	  printf("   -quiet\n");
	  exit(0);
	}
	sprintf(nite,"%s",argv[1]);
	sprintf(arnode,"%s",argv[2]);

        /* process command line */
        for (i=3;i<argc;i++) {
          if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}

	/* grab dblogin */
	select_dblogin(dblogin);
	/* grab arnode information*/
	select_archivenode(dblogin,arnode,arroot,arsite);



	sprintf(sqlquery,"SPT_ingest.sql_%s",nite);
	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s > %s.out",dblogin,sqlquery,sqlquery);

	/* first create directory structure needed within the archive */
	sprintf(srcstring,"%s/raw/%s/src",arroot,nite);
	out=fopen(sqlquery,"w");
	sprintf(command,"ls -d %s/*",srcstring);
	if (!flag_quiet) printf("      %s\n",command);
	pip=popen(command,"r");
	while (fgets(line,1000,pip)!=NULL) {
	  sprintf(image,"%s",line);
	 
	  /* strip off only the image name */
	  image[strlen(image)-1]=0;
	  for (i=strlen(image)-strlen(srcstring);i>=0;i--)
	    if (!strncmp(&(image[i]),srcstring,strlen(srcstring))) {
	      sprintf(imagename,"%s",image+i+1+strlen(srcstring));
	      break;
	    }
	  fprintf(out,"INSERT into FILES (IMAGEID, IMAGETYPE, NITE, IMAGENAME, ARCHIVESITES, IMAGECLASS) VALUES ( files_seq.nextval,'%s', '%s','%s','%s', '%s');\n",
	    imagetype,nite,imagename,arsite,imageclass); 
	  count++;
	}
	fclose(out);
	pclose(pip);
	if (!flag_quiet) {
	  printf("      Ingesting file %s \n",sqlquery);
	  printf("      STDOUT captured in %s.out\n",sqlquery);
	}
	system(sqlcall);
	if (!flag_quiet) printf("  Discovered and ingested %d data file(s) for nite %s\n",
	  count,nite);
}

#undef MAXNODES
