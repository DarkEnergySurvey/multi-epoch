#include "imageproc.h"

/*  For quick ingestion of src images
/*	nite
/*	imagetype
/*	imagename
*/

main(argc, argv)
	int argc;
	char *argv[];
{

	char	archive[500],nite[100],imagename[100],command[1000],
		imagetype[100],imageclass[10]="raw",sqlcall[500],
		sqlquery[500],line[1000],image[1000],dblogin[2000],
		arnode[100],arroot[1000],arsites[100];
	int	i,count=0,flag_quiet=0;
	FILE	*pip,*out;
	void	select_dblogin(),select_archivenode();

	if (argc<3) {
	  printf("  src_ingest <nite> <archive node>\n");
	  printf("   -quiet\n");
	  exit(0);
	}
	sprintf(nite,"%s",argv[1]);
	sprintf(arnode,"%s",argv[2]);

	sprintf(imagetype,"src");
	
        /* process command line */
        for (i=3;i<argc;i++) {
          if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}

	if (!flag_quiet) 
	  printf("  Creating image list and ingestion call for nite %s\n",
	  nite);

	/* select the db login from desdm config file */
	select_dblogin(dblogin);

	/* grab the archive node information */
	select_archivenode(dblogin,arnode,arroot,arsites);

	/* set up for the ingest */
	sprintf(sqlquery,"src_ingest.sql_%s",nite);
	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s > %s.out",
	  dblogin,sqlquery,sqlquery);

	/* first create directory structure needed within the archive */
	out=fopen(sqlquery,"w");
	sprintf(command,"ls %s/raw/%s/src/*.fits*",
	  arroot,nite);
	if (!flag_quiet) printf("      %s\n",command);
	pip=popen(command,"r");
	while (fgets(line,1000,pip)!=NULL) {
	  /* strip the .fits off the filename */
	  sprintf(image,"%s",line);
	  for (i=strlen(image);i>=0;i--) {
	    if (!strncmp(&(image[i]),".fits",5)) {
	      image[i]=0;
	      break;
	     }
	  }
	 
	  /* strip off only the image name */
	  for (i=strlen(image);i>=0;i--)
	    if (!strncmp(&(image[i]),"/",1)) {
	      sprintf(imagename,"%s",image+i+1);
	      break;
	    }
	  fprintf(out,"INSERT into FILES (IMAGEID, IMAGETYPE, NITE, IMAGENAME,ARCHIVESITES,IMAGECLASS) VALUES ( files_seq.nextval,'%s', '%s','%s', '%s', '%s');\n",
	    imagetype,nite,imagename,arsites,imageclass); 
	  count++;
	}
	fclose(out);
	pclose(pip);
	if (!flag_quiet) {
	  printf("      Ingesting file %s \n",sqlquery);
	  printf("      STDOUT captured in %s.out\n",sqlquery);
	}
	system(sqlcall);
	if (!flag_quiet) printf("  Discovered and ingested %d images for nite %s\n",
	  count,nite);
}
