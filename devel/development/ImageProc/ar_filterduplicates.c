/*  this program can be used to remove duplicate images in a particular night
/*  a common result of multiple ingests
*/

#include "imageproc.h" 

main(argc,argv)
	 int argc;
	char *argv[];
{ 
	char	**imagename,**tilename,**band,nite[100],line1[1000],
		**imageclass,**imagetype,
		sqlquery[500],input_imagetype[100],dblogin[500];
	int	i,*imageid,flag_delete=0,*flag_duplicate,j,offset,
		*ccd_number,count,numtotal=0,numduplicate=0,
		flag_quiet=0;
	FILE	*out,*pip;
	void	select_dblogin();

	if (argc<2) {
	  printf("ar_filterduplicates <nite> \n");
	  printf("  -delete (delete duplicates)\n");
	  printf("  -quiet\n");
	  exit(0);
	}

	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-delete")) {
	    flag_delete=1;
	  }
	  if (!strcmp(argv[i],"-quiet")) {
	    flag_quiet=1;
	  }
	}

	sprintf(nite,"%s",argv[1]);

	/* write sqlplus query to extract information from archive */
	out=fopen("ar_filterduplicates.sql","w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(out,"select count(*) from files ");
	fprintf(out,"where nite='%s';\n", nite);
	fprintf(out,"select imageid||' | '||imagename||' | '||tilename||' | '||ccd_number||' | '||imagetype from files ");
	fprintf(out,"where nite='%s' order by imageid;\n", nite);
	fprintf(out,"exit;\n");
	fclose(out);


	/* get db login */
	select_dblogin(dblogin);
	/* execute query */
        sprintf(sqlquery,"sqlplus -S %s @ ar_filterduplicates.sql\n",dblogin);


	pip=popen(sqlquery,"r");
	fscanf(pip,"%d",&count);
	if (!flag_quiet) printf("  %d files found for nite %s\n",count,nite);
	imageid=(int *)calloc(count,sizeof(int));
	ccd_number=(int *)calloc(count,sizeof(int));
	imagename=(char **)calloc(count,sizeof(char *));
	tilename=(char **)calloc(count,sizeof(char *));
	imagetype=(char **)calloc(count,sizeof(char *));
	imageclass=(char **)calloc(count,sizeof(char *));
	flag_duplicate=(int *)calloc(count,sizeof(int));
	for (i=0;i<count;i++) {
	  imagename[i]=(char *)calloc(100,sizeof(char));
	  tilename[i]=(char *)calloc(100,sizeof(char));
	  imagetype[i]=(char *)calloc(100,sizeof(char));
	  imageclass[i]=(char *)calloc(100,sizeof(char));
	  flag_duplicate[i]=0;
	}
	i=0;
	while(fgets(line1,1000,pip)!=NULL) {
	  if (strlen(line1)>1) {
	    /* read imagetype */
	    for (j=strlen(line1);j>0;j--) 
	      if (!strncmp(line1+j,"|",1)) {
		strcpy(imagetype[i],line1+j+1);
		imagetype[i][strlen(imagetype[i])-1]=0;
	        line1[j]=0;
	  	break;
	      }
	    /* read ccd_number */
	    for (j=strlen(line1);j>0;j--) 
	      if (!strncmp(line1+j,"|",1)) {
	  	sscanf(line1+j+1,"%d",ccd_number+i);
	        line1[j]=0;
	  	break;
	      }
	    /* read tilename */
	    for (j=strlen(line1);j>0;j--) 
	      if (!strncmp(line1+j,"|",1)) {
	  	sscanf(line1+j+1,"%s",tilename[i]);
	        line1[j]=0;
	  	break;
	      }
	    /* read imagename */
	    for (j=strlen(line1);j>0;j--) 
	      if (!strncmp(line1+j,"|",1)) {
	  	sscanf(line1+j+1,"%s",imagename[i]);
	        line1[j]=0;
	  	break;
	      }
	    /* read imageid */
	    sscanf(line1,"%d",imageid+i);
	    /*if (!flag_quiet) {
	      printf("    %6d/%6d: %6d %20s %12s %2d %12s\n",i+1,
		count,imageid[i],
	        imagename[i],tilename[i],ccd_number[i],imagetype[i]);
	    }*/
	    i++;
	  }
	}
	pclose(pip);


	if (!flag_quiet) {
	  printf("  Now cycling through looking for duplicates\n");
	  fflush(stdout);
	}

	/* now cycle through marking the duplicates */
	out=fopen("ar_filterduplicates.sql","w");
	for (j=0;j<count;j++) {
	  for (i=j+1;i<count;i++) {
	    if (ccd_number[j]==ccd_number[i]) 
	    if (!strcmp(imagename[i],imagename[j]))
	    if (!strcmp(tilename[i],tilename[j])
	      && !strcmp(imagetype[i],imagetype[j])) {
	      flag_duplicate[i]=1;
	      fprintf(out,"delete from files where imageid=%d;\n",imageid[i]);
	      if (!flag_quiet) {
	        printf("  Duplicate: %6d %20s %12s %2d %12s\n",imageid[i],
	          imagename[i],tilename[i],ccd_number[i],imagetype[i]);
	    }
	      numduplicate++;
	    }
	  }
	  /*if (j%10000==0 && !flag_quiet) {printf(".");fflush(stdout);}*/
	}	
	fprintf(out,"commit;\n");
	fclose(out);

	printf("\n  %d images found, %d duplicate images marked for deletion\n",
	  count,numduplicate);
	if (flag_delete) {
	  printf("  Now deleting duplicate images");
	  fflush(stdout);
          sprintf(sqlquery,"sqlplus -S %s < ar_filterduplicates.sql > /dev/null; rm ar_filterduplicates.sql",dblogin);
	  system(sqlquery);
	  printf("\n");
	}

}
