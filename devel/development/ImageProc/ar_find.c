#include "imageproc.h" 
#define IMTYPEMAX 7
#define REMAP 5 
#define REDUCED 4 

#define BANDMAX 4
#define CCDMAX 100
#define RUNIDMAX 200

main(argc,argv)
	 int argc;
	char *argv[];
{ 
	char	tagimtype[IMTYPEMAX][100]={"src","zero","dome flat","object", 
		"reduced","remap","catalog"},imname[100],
		tagband[BANDMAX][10]={"g","r","i","z"},
		tagrunid[RUNIDMAX][200],sqlfile[200],
		nite[100],command[500],list_name[500],
		query[25]="count(*)",dblogin[500],runid_in[200];
	int	i,files[RUNIDMAX][IMTYPEMAX][BANDMAX+1][CCDMAX],ccdnumber,
		runidnumber,runidmax,runid,imtypemin,imtypemax,flag,runidcount,
		objects[RUNIDMAX][IMTYPEMAX][BANDMAX+1][CCDMAX],
		remapobjects=0,reducedobjects=0,ccdcount,ccdmin,
		ccdmax,band,imtype,ccd,flag_list=0,flag_runid=0;
	FILE	*out,*pip;
	void	select_dblogin();

	if (argc<2) {
	  printf("ar_find <nite>\n");
	  printf("  -list <filename>\n");
	  printf("  -runid <runid>\n");
	  exit(0);
	}

	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-list")) {
	    flag_list=1;
	    sprintf(query,"*");
	    i++;
	    sscanf(argv[i],"%s",list_name);
	  }
	  if (!strcmp(argv[i],"-runid")) {
	    flag_runid=1;
	    sscanf(argv[i+1],"%s",runid_in);
	  }
	}

	sprintf(nite,"%s",argv[1]);

	/* retrieve db login information */
	select_dblogin(dblogin);

	/* create ar_find.sql file */
	sprintf(sqlfile,"%s.ar_find.sql",nite);


	/* ******************************************************* */
	/* ********* First See how many CCD's and RUNID's ******** */
	/* ******************************************************* */
	/* write sqlplus query to extract information from archive */
	/* we want:
	/*  	# of CCD's
	/*  	# of distinct RUNIDDESC
	/*  	List of those distinct RUNIDDESC's
	/* ******************************************************* */
	out=fopen(sqlfile,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' | '");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        fprintf(out,"select count(unique(ccd_number)) from files where\n");
	fprintf(out,"(nite='%s' and (lower(imagetype)='object'",nite);
 	fprintf(out," or lower(imagetype)='dome flat'));\n");
        fprintf(out,"select count(unique(runiddesc)) from files where\n");
	fprintf(out,"(nite='%s' ",nite);
	if(flag_runid) fprintf(out,"and runiddesc like '%s%%' ",runid_in);
	fprintf(out,"and (lower(imagetype)='reduced'",nite);
	fprintf(out," or lower(imagetype)='remap' ");
	fprintf(out," or lower(imagetype)='catalog'));\n");
        fprintf(out,"select unique(runiddesc) from files where\n");
	fprintf(out,"(nite='%s' ",nite);
	if(flag_runid) fprintf(out,"and runiddesc like '%s%%' ",runid_in);
	fprintf(out,"and (lower(imagetype)='reduced'",nite);
	fprintf(out," or lower(imagetype)='remap'");
	fprintf(out," or lower(imagetype)='catalog'));\n");
        fprintf(out,"exit;\n");
	fclose(out);

        sprintf(command,
	  "sqlplus -S %s",dblogin);
        if (flag_list) 
	  sprintf(command,"%s < %s > %s\n",command,sqlfile,list_name);
        else sprintf(command,"%s < %s\n",command,sqlfile);

	pip=popen(command,"r");
	fscanf(pip,"%d",&ccdnumber);
	fscanf(pip,"%d",&runidnumber);
	runidcount=flag=0;
	for (runid=0;runid<runidnumber;runid++) {
	  fscanf(pip,"%s",tagrunid[runidcount]);
	  /* strip off the _?? suffix if present */
	  if (!strncmp(tagrunid[runidcount]+strlen(tagrunid[runidcount])-3,
	    "_",1)) 
	    tagrunid[runidcount][strlen(tagrunid[runidcount])-3]=0;
	  /* check for duplicates */
	  flag=0;
	  for (i=0;i<runidcount;i++) 
	    if (!strcmp(tagrunid[i],tagrunid[runidcount])) flag=1;
	  if (!flag) runidcount++;
	}
	pclose(pip);
	runidnumber=runidcount;

	if (ccdnumber>CCDMAX || ccdnumber<0) {
	  printf("  ** CCD number if out of range 0<#<=%d\n",CCDMAX);
	  exit(0);
	}
	if (runidnumber>RUNIDMAX || ccdnumber<0) {
	  printf("  ** CCD number if out of range 0<#<=%d\n",CCDMAX);
	  exit(0);
	}
	printf("  %d CCDs ",ccdnumber);
	/* make 1 the minimum ccdnumber */
	if (ccdnumber==0) ccdnumber=1;
	if (runid>0) {
	  printf("and the following RUNID: \n    ");
	  for (runid=0;runid<runidnumber;runid++) 
	    printf("  %s",tagrunid[runid]);
	}
	printf("\n");


	/* ****************************************** */
	/* ********* Begin Large Scale Query ******** */
	/* ****************************************** */

	for (runid=0;runid<=runidnumber;runid++) 
	for (imtype=0;imtype<IMTYPEMAX;imtype++) 
	 for (band=0;band<BANDMAX+1;band++) 
	  for (ccd=0;ccd<ccdnumber;ccd++) files[runid][imtype][band][ccd]=
		objects[runid][imtype][band][ccd]=0;

	
	for (ccdcount=0;ccdcount<8;ccdcount++) {
	  ccdmin=ccdcount*8;ccdmax=(ccdcount+1)*8;
	  if (ccdmin>=ccdnumber) break;
	  if (ccdmax>ccdnumber) ccdmax=ccdnumber;

	/* write sqlplus query to extract information from archive */
	out=fopen(sqlfile,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP ' | '");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");

      	/* query for numbers of files */
	if (runidnumber==0) imtypemax=4;
	else imtypemax=IMTYPEMAX;
      	for (runid=0;runid<=runidnumber;runid++) {
	  if (runid==runidnumber && runid) break;
      	  if (runid==0) imtypemin=0;
       	  else imtypemin=4;
          /* select statements for images */
	  for (imtype=imtypemin;imtype<imtypemax;imtype++) {
	    for (band=0;band<BANDMAX;band++) 
	      for (ccd=ccdmin;ccd<ccdmax;ccd++) 
	        if (imtype>=4) {/* processed data */
	          fprintf(out,"select %s from files where nite='%s' ",
	            query,nite);
	          fprintf(out,"and lower(imagetype)='%s' and ccd_number=%d ",
		    tagimtype[imtype],ccd+1);
		  fprintf(out,"and band='%s' and runiddesc like '%s%%';\n",
		    tagband[band],tagrunid[runid]);
		}
	        else if (strcmp(tagimtype[imtype],"src") && 
		  strcmp(tagimtype[imtype],"zero")) {
	          fprintf(out,"select %s from files where nite='%s' and ",
	            query,nite);
	          fprintf(out,"lower(imagetype)='%s' and ccd_number=%d ",
	            tagimtype[imtype],ccd+1);
		  fprintf(out,"and band='%s';\n",
	            tagband[band]);
		}
	        else if (band==0 && !strcmp(tagimtype[imtype],"zero")) {
	          fprintf(out,"select %s from files where nite='%s' and ",
	            query,nite);
	          fprintf(out,"lower(imagetype)='%s' and ccd_number=%d;\n",
	            tagimtype[imtype],ccd+1);
		}
	        else if (band==0 && !strcmp(tagimtype[imtype],"src")) {
	          fprintf(out,"select %s from files where nite='%s' and ",
	            query,nite);
	          fprintf(out,"lower(imagetype)='%s';\n",
		    tagimtype[imtype]);
		}
	  } /* imtype loop */

	  if (runidnumber>0) {
	  /* query for numbers of objects */
	    /* query for objects from reduced images */
	    for (band=0;band<BANDMAX;band++) 
	      for (ccd=ccdmin;ccd<ccdmax;ccd++) {
	        fprintf(out,"select %s from objects o, files f where ",query);
		fprintf(out,"o.imageid=f.imageid and f.nite='%s' and ",nite);
		fprintf(out,"lower(f.imagetype)='%s' and f.ccd_number=%d ",
	          tagimtype[REDUCED],ccd+1);
		fprintf(out,"and f.band='%s' and f.runiddesc like '%s%%';\n",
	          tagband[band],tagrunid[runid]);
	      }
	    /* query for objects from remap images */
	    for (band=0;band<BANDMAX;band++) 
	      for (ccd=ccdmin;ccd<ccdmax;ccd++) {
	        fprintf(out,"select %s from objects o, files f where ",query);
		fprintf(out,"o.imageid=f.imageid and f.nite='%s' and ",nite);
		fprintf(out,"lower(f.imagetype)='%s' and f.ccd_number=%d ",
	          tagimtype[REMAP],ccd+1);
		fprintf(out,"and f.band='%s' and f.runiddesc like '%s%%';\n", 
		  tagband[band],tagrunid[runid]); } }
	}
	/* close query file */
        fprintf(out,"exit;\n");
        fclose(out); 




	/* ****************** */
	/* make database call */
	/* ****************** */

	if (flag_list) {
          sprintf(command,
	    "sqlplus -S %s ",dblogin);
	  sprintf(command,"%s  < %s > %s\n",command,sqlfile,list_name);
	  system(command);
	}
	else {
          sprintf(command,
	    "sqlplus -S %s ",dblogin);
	  sprintf(command,"%s  < %s\n",command,sqlfile);
	}
	

	/* *************************************** */
	/* read in the number of files and objects */
	/* *************************************** */

	pip=popen(command,"r");
	printf("%s   \n",nite);
	printf("=========================================================");
	printf("===========\n    ");
	for (ccd=ccdmin;ccd<ccdmax;ccd++) printf("      %2d",ccd+1);
	printf("\n");
	printf("                    **********  file counts   **********\n");
	printf("\nRaw and Source Data:  \n");
	printf("**********************************************************");
	printf("**********\n");
        for (runid=0;runid<=runidnumber;runid++) {
	  if (runid==runidnumber && runid) break;
	  if (runid==0) imtypemin=0;
	  else imtypemin=4;
	  for (imtype=imtypemin;imtype<imtypemax;imtype++) {
	    if (imtype==4) {
	      printf("\nRUNID:  %s\n",tagrunid[runid]);
	      printf("*******************************************************");
	      printf("*************\n");
	    }
	    for (ccd=ccdmin;ccd<ccdmax;ccd++) 
	      files[runid][imtype][BANDMAX][ccd]=0;
	    for (band=0;band<BANDMAX;band++) {
	      if (band==0) sprintf(imname,"%s",tagimtype[imtype]);
	      else imname[0]=0;
	      for (ccd=ccdmin;ccd<ccdmax;ccd++) {
	        if ((!strcmp(tagimtype[imtype],"zero") || 
	          !strcmp(tagimtype[imtype],"src")) && band==0) {
	          fscanf(pip,"%d",&(files[runid][imtype][band][ccd]));
	          if (ccd==ccdmin) {
	            printf("%-10s\n",imname);
	            printf("%4s     %3d","",files[runid][imtype][band][ccd]);
	          }
	          else printf("     %3d",files[runid][imtype][band][ccd]);
	        }
	        if (strcmp(tagimtype[imtype],"zero")
	          && strcmp(tagimtype[imtype],"src")) {
	          fscanf(pip,"%d",&(files[runid][imtype][band][ccd]));
	          if (ccd==ccdmin) {
	            printf("%-10s\n",imname);
	            printf("%4s     %3d",tagband[band],
	              files[runid][imtype][band][ccd]);
	          }
	          else printf("     %3d",files[runid][imtype][band][ccd]);
	          files[runid][imtype][BANDMAX][ccd]+=
	            files[runid][imtype][band][ccd];
	        }
	      }   /* end of  ccd loop */
	    }     /* end of band loop */
	
	    if (strcmp(tagimtype[imtype],"zero")
	      && strcmp(tagimtype[imtype],"src")) {
	      printf("\n%4s","tot");
	      for (ccd=ccdmin;ccd<ccdmax;ccd++)
	        printf("     %3d",files[runid][imtype][BANDMAX][ccd]);
	    }
	    printf("\n");
	  }


	  /* ***************************** */
	  /* read in the number of objects */
	  /* ***************************** */
	
	  if (runidnumber>0) {
	  printf("                    ********** object counts  **********\n");
	  remapobjects=reducedobjects=0;
	  for (ccd=ccdmin;ccd<ccdmax;ccd++) 
	    objects[runid][REDUCED][BANDMAX][ccd]=0;
	  for (band=0;band<BANDMAX;band++) {
	    if (band==0) sprintf(imname,"%s",tagimtype[REDUCED]);
	    else imname[0]=0;
	    for (ccd=ccdmin;ccd<ccdmax;ccd++) {
	      fscanf(pip,"%d",&(objects[runid][REDUCED][band][ccd]));
	      if (ccd==ccdmin) {
		printf("%-10s\n",imname);
		printf("%4s %7d",tagband[band],
		  objects[runid][REDUCED][band][ccd]);
	      }
	      else printf(" %7d",objects[runid][REDUCED][band][ccd]);
	      objects[runid][REDUCED][BANDMAX][ccd]+=
	        objects[runid][REDUCED][band][ccd];
	    }
	  }
	  printf("\n%4s %7d","tot",objects[runid][REDUCED][BANDMAX][0]);
	  for (ccd=ccdmin+1;ccd<ccdmax;ccd++) {
	    printf(" %7d",objects[runid][REDUCED][BANDMAX][ccd]);
	    reducedobjects+=objects[runid][REDUCED][BANDMAX][ccd];
	  }
	  printf("\n%4s %7d\n","All Reduced Image Catalogs:",reducedobjects);
	  /*for (ccd=ccdmin;ccd<ccdmax;ccd++) objects[REMAP][BANDMAX][ccd]=0;*/
	  for (band=0;band<BANDMAX;band++) {
	    if (band==0) sprintf(imname,"%s",tagimtype[REMAP]);
	    else imname[0]=0;
	    for (ccd=ccdmin;ccd<ccdmax;ccd++) {
	      fscanf(pip,"%d",&(objects[runid][REMAP][band][ccd]));
	      if (ccd==ccdmin) {
		printf("%-10s\n",imname);
		printf("%4s %7d",tagband[band],
		  objects[runid][REMAP][band][ccd]);
	      }
	      else printf(" %7d",objects[runid][REMAP][band][ccd]);
	      objects[runid][REMAP][BANDMAX][ccd]+=
		objects[runid][REMAP][band][ccd];
	    }
	  }
	  printf("\n%4s","tot");
	  for (ccd=ccdmin;ccd<ccdmax;ccd++) {
	    printf(" %7d",objects[runid][REMAP][BANDMAX][ccd]);
	    remapobjects+=objects[runid][REMAP][BANDMAX][ccd];
	  }
	  printf("\n%4s %7d\n","All Remap Image Catalogs:",remapobjects);
	  }
	  }
	  pclose(pip);
	}
	sprintf(command,"rm %s",sqlfile);
	system(command);

	printf("========================================================");
	printf("============\n");
	printf("\n\n");
}

#undef REMAP
#undef REDUCED
#undef BANDMAX
#undef IMTYPEMAX
#undef RUNIDMAX
#undef CCDMAX
