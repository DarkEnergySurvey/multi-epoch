#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
	char	file[500],nite[100],tilename[100],imagetype[100],band[10],
		runid[100],imagename[100];
	int	flag_band=0,flag_nite=0,flag_ccd_number=0,flag_tilename=0,
		flag_imagetype=0,flag_runid=0,flag_file=0,
		ccd_number,*imageids,numids,i;
	FILE	*out,*pip;
	void	filename_resolve(),archive_select();

	if (argc<2) {
	  printf("ar_removefile \n");
	  printf("  -file <filename>\n");
	  printf("  -nite <nite>\n");
	  printf("  -runid <runid>\n");
	  printf("  -ccd_number <#>\n");
	  printf("  -tilename <tilename>\n");
	  printf("  -imagetype <imagetype>\n");
	  printf("  -band <band>\n");
	  exit(0);
	}
	

	/* process the command line */
	for (i=1;i<argc;i++) {
	  if (!strcmp(argv[i],"-file")) {
	    flag_file=1;
	    i++;
	    sscanf(argv[i],"%s",file);
	  }
	  if (!strcmp(argv[i],"-nite")) {
	    flag_nite=1;
	    i++;
	    sscanf(argv[i],"%s",nite);
	  }
	  if (!strcmp(argv[i],"-ccd_number")) {
	    flag_ccd_number=1;
	    i++;
	    sscanf(argv[i],"%d",&ccd_number);
	  }
	  if (!strcmp(argv[i],"-tilename")) {
	    flag_tilename=1;
	    i++;
	    sscanf(argv[i],"%s",tilename);
	  }
	  if (!strcmp(argv[i],"-band")) {
	    flag_band=1;
	    i++;
	    sscanf(argv[i],"%s",band);
	  }
	  if (!strcmp(argv[i],"-runid")) {
	    flag_runid=1;
	    i++;
	    sscanf(argv[i],"%s",runid);
	  }
	  if (!strcmp(argv[i],"-imagetype")) {
	    flag_imagetype=1;
	    i++;
	    sscanf(argv[i],"%s",imagetype);
	  }
	}

	if (flag_file) {
	  filename_resolve(file,runid,nite,band,tilename,imagetype,
	    imagename,&ccd_number);
	  printf("  resolve- runid: %s nite: %s band: %s tilename: %s imagetype: %s imagename: %s ccd_number: %d\n",
	    runid,nite,band,tilename,imagetype,imagename,ccd_number);
	  /* query for the file */
	  archive_select(runid,nite,band,tilename,imagetype,imagename,ccd_number,imageids,&numids); 
	  if (i==1) printf("  Archive db contains %d file matching this name\n",numids);
	  else printf("  Archive db contains %d files matching this name\n",numids);
	  printf("  Imageids: ");
	  for (i=0;i<numids;i++) {
	    printf(" %d",imageids[i]);
	    if (i && !(i%10)) printf("\n    ");
	  }
	  printf("\n");
	}
	else {
	  printf("** Not yet implemented\n");
	  exit(0);
	}
}

void archive_select(runid,nite,band,tilename,imagetype,imagename,ccd_number,imageids,numids)
	char 	runid[],band[],nite[],tilename[],imagetype[],imagename[];
	int	ccd_number,imageids[],*numids;
{

	FILE	*pip,*out;
	int	i;
	char	command[100]="sqlplus -S pipeline/dc01user@charon.ncsa.uiuc.edu/des < ar_removefile.sql";

	/* write sqlplus query to extract information from archive */
	out=fopen("ar_removefile.sql","w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' | '");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(out,"select count(*) from files where nite='%s' and ccd_number=%d and band='%s' and tilename='%s' and runid='%s' and imagename='%s';\n",
	      nite,imagetype,ccd_number,band,tilename,runid,imagename);
	fprintf(out,"select count(*) from files where nite='%s' and imagetype='%s' and ccd_number=%d and band='%s' and tilename='%s' and runid='%s' and imagename='%s';\n",
	      nite,imagetype,ccd_number,band,tilename,runid,imagename);
	fprintf(out,"select imageid from files where nite='%s' and imagetype='%s' and ccd_number=%d and band='%s' and tilename='%s' and runid='%s' and imagename='%s';\n",
	      nite,imagetype,ccd_number,band,tilename,runid,imagename);
        fprintf(out,"exit;\n");
        fclose(out); 

	/* make database call */
	pip=popen(command,"r");
	fscanf(pip,"%d",numids);
	if ((*numids)) imageids=(int *)calloc((*numids),sizeof(int));
	for (i=0;i<*numids;i++) fscanf(pip,"%d",imageids+i);
	pclose(pip);
}
