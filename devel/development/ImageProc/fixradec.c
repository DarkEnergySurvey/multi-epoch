#include "imageproc.h" 

main(argc,argv)
	 int argc;
	char *argv[];
{ 
	char	nite[100],command[500],**imagename,**imagetype,**band,
		line1[1000],tr[100];
	int	i,j,count,*imageid,*ccd_number,flag_quiet;
	double	*ra,*dec;
	FILE	*out,*inp;

	if (argc<1) {
	  printf("fixradec \n");
	  printf("  -quiet\n");
	  exit(0);
	}
	if (argc>1) flag_quiet=1;
	else flag_quiet=0;

	sprintf(nite,"des20061005");

	/* write sqlplus query to extract information from archive */
	out=fopen("fixradec.sql","w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' | '");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(out,"select count(*) from files where nite='%s' and imagetype!='src' and imagetype!='zero' and imagetype!='dome flat' and (lower(imagetype)!='object' or (lower(imagetype)='object' and ccd_number=1));\n",nite);
	fprintf(out,"select imageid,imagename,imagetype,ccd_number,band,ra,dec from files where nite='%s' and imagetype!='src' and imagetype!='zero' and imagetype!='dome flat' and (lower(imagetype)!='object' or (lower(imagetype)='object' and ccd_number=1));\n",nite);
        fprintf(out,"order by imagename;\nexit;\n");
        fclose(out); 

	/* make database call */

        sprintf(command,"sqlplus -S pipeline/dc01user@charon.ncsa.uiuc.edu/des < fixradec.sql > fixradec.out\n");
	system(command);

	/* now read in query */
	/* read in the number of files */
	inp=fopen("fixradec.out","r");
	fscanf(inp,"%d",&count);
	printf("  %d files for nite=%s   \n",count,nite);

	imageid=(int *)calloc(count,sizeof(int));
	imagename=(char **)calloc(count,sizeof(char *));
	for (i=0;i<count;i++) imagename[i]=(char *)calloc(100,sizeof(char));
	imagetype=(char **)calloc(count,sizeof(char *));
	for (i=0;i<count;i++) imagetype[i]=(char *)calloc(20,sizeof(char));
	ccd_number=(int *)calloc(count,sizeof(int));
	band=(char **)calloc(count,sizeof(char *));
	for (i=0;i<count;i++) band[i]=(char *)calloc(20,sizeof(char));
	ra=(double *)calloc(count,sizeof(double));
	dec=(double *)calloc(count,sizeof(double));
	
	i=0;
	while (fgets(line1,1000,inp)!=NULL) {
	  sscanf(line1,"%d %s %s %s %s %s %d %s %s %s %lf %s %lf",
	    imageid+i,tr,imagename[i],tr,imagetype[i],tr,ccd_number+i,
	    tr,band[i],tr,ra+i,tr,dec+i); 
	  if (!flag_quiet) printf("  %d: %6d %-25s %-15s %2d %-10s %8.4f %8.4f  ** %s\n",
	    i,imageid[i],imagename[i],imagetype[i],ccd_number[i],band[i],
	    ra[i],dec[i],line1);
	  i++;
	}
	fclose(inp);

	out=fopen("fixradec.update.sql","w");
	/* now cycle through creating update calls to fix the ra,dec */
	for (i=0;i<count;i++) {
	  if (strcmp(imagetype[i],"OBJECT") && strcmp(imagetype[i],"catalog")) {
	    for (j=0;j<count;j++) 
		if (!strcmp(imagename[i],imagename[j])) {
 	          if (!strcmp(band[i],band[j]) && 
		    !strcmp(imagetype[j],"OBJECT")) {
			ra[i]=ra[j];
			dec[i]=dec[j];
	              if (!flag_quiet) 
			printf("  %d: %6d %-25s %-15s %2d %-10s %8.4f %8.4f\n",
	              	i,imageid[i],imagename[i],imagetype[i],
		      	ccd_number[i],band[i],ra[i],dec[i]);
		      fprintf(out,"UPDATE files set ra=%.6f, dec=%.6f where imageid=%d;\n",
			ra[i],dec[i],imageid[i]);
		  }
	        }
	   }
	}
	fclose(out);

	printf("  Now updating FILES table\n");
        sprintf(command,"sqlplus -S pipeline/dc01user@charon.ncsa.uiuc.edu/des < fixradec.update.sql > fixradec.update.out\n");
	system(command);
}
