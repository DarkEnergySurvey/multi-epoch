/* run at this level:
/*
/*	runCatalog_ingest.c  -- wrapper program for running
/*	catalog_ingest, a program that reads FITS catalogs and prints
/*	formatted output in ASCII.
/*	into the Archive
/*
/*   runid/data/nite/band/imagename/imagename_cat.fits
*/
#include "imageproc.h"
#include <unistd.h>

#define MAXLISTS 500
#define MAXIMAGE 100
#define SQLLDR_LIMIT 100000
#define LEN_FIELDS 100
#define NUM_FIELDS 9
#define STRING_FIELDS (LEN_FIELDS*NUM_FIELDS)

main(argc,argv)
	int argc;
	char *argv[];
{
	char	imagename[800],command[15000],sqlldr_ctrl[800],
	    	sqlldr_ctrlout[800],temp[800],dbtable[800],
		imagenamesave[800],cleanup[20],sqlldr_data[800],
		sqlldr_root[800],nite[100],oldnite[100]="",runid[100],band[100],
		filelist[MAXLISTS][800],line[1000],newimagename[800],
		tilename[100],imagetype[100],filename[800],sqlcall[800],
		inputimage[MAXIMAGE][800],binpath[800],runids[1000][100],
	        templist[STRING_FIELDS],tmp[800],dblogin[500];
	int	k,j,i,len,imnum=0,flag_quiet=0,num_data,nrunids=0,
		nlists=0,flag_cat=0,flag_cleanup=0,flag_bin=0,
		flag_sqlldrctrl=0,flag_sqlldrdata=0,fileread,
		flag_sqlldr_multiingest=0,ingest_count=0,
		sqlldr_multiingest_limit=0,sleepcounter,
		imageid,ccdnum,flag,nfiles,match,
		num_tot=0,num_current=0,num_line,flag_ccd=0,
		select_ccdnum,table_exists(),create_table(),
		numdbjobs();
	FILE	*fin,*pip,*fin2,*out;
	void	filename_resolve(),splitstring(),select_dblogin();
	struct  imageinfo {
		int	imageid,ccdnum;
		float	equinox;
		char	nite[100],band[100],runid[100],tilename[100],
			imagename[100],imagetype[100];
		} *im;

  	if ( argc < 2 ) {
    	  printf ("Usage: %s <list1> <list2> ... <listN>\n", 
	    argv [0]);
	  printf("  Options:\n");
	  printf("  -bin <dir>\n");
	  printf("  -sqlldr_ctrl <file>\n");
	  printf("  -sqlldr_data <file>\n");
	  printf("  -sqlldr_multiingest <#>\n");
	  printf("  -ccd <#>\n");
	  printf("  -quiet\n");
    	  exit(0);
  	}
	
	/* set up defaults */
	binpath[0]=0;
	sprintf(sqlldr_data,"runCatalog_ingest.data");
	sprintf(sqlldr_root,"runCatalog_ingest");


	/* ************************************************************* */
	/* *************  process the command line ********************* */
	/* ************************************************************* */
	for (i=1;i<argc;i++) {
	  flag=0;
	  if (!strcmp(argv[i],"-quiet")) flag=flag_quiet=1;
	  if (!strcmp(argv[i],"-sqlldr_data")) {
	    flag=flag_sqlldrdata=1;
	    i++;
	    /* read bin */
	    sscanf(argv[i],"%s",sqlldr_root);
	    sprintf(sqlldr_data,"%s.sqldata",sqlldr_root);
	  }
	  if (!strcmp(argv[i],"-sqlldr_ctrl")) {
	    flag=flag_sqlldrctrl=1;
	    i++;
	    /* read bin */
	    sscanf(argv[i],"%s",sqlldr_ctrl);
	  }
	  if (!strcmp(argv[i],"-sqlldr_multiingest")) {
	    flag=flag_sqlldr_multiingest=1;
	    i++;
	    /* read bin */
	    sscanf(argv[i],"%d",&sqlldr_multiingest_limit);
	  }
	  if (!strcmp(argv[i],"-bin")) {
	    flag=flag_bin=1;
	    i++;
	    /* read bin */
	    sscanf(argv[i],"%s",binpath);
	  }
	  if (!strcmp(argv[i],"-ccd")) {
	    flag=flag_ccd=1;
	    i++;
	    /* read bin */
	    sscanf(argv[i],"%d",&select_ccdnum);
	  }
	  if (!flag) { /* assume it is a file list */
	      sprintf(filelist[nlists],"%s",argv[i]);
	      nlists++;
	      if (nlists>=MAXLISTS) {
	 	printf("  **runCatalog_ingest:  MAXLISTS exceeded!\n");
	 	exit(0);
	      }
	  }
	} 


	/* ************************************************************* */
	/* now cycle through lists to confirm cat files and get runids * */
	/* ************************************************************* */
	for (j=0;j<nlists;j++) {
	  fin=fopen(filelist[j],"r");
	  /* check list file to confirm it exists */
	  if (fin == NULL) {
	    printf("  ** runCatalog_ingest:  List file \"%s\" not found.\n", 
	    filelist[j]);
	    exit (0);
	  }  
	  /* confirm that it contains FITS catalog files */
	  while (fscanf(fin,"%s",imagename)!=EOF) {
	    /* resolve filename into file information */
	    filename_resolve(imagename,runid,nite,band,tilename,imagetype,
	      newimagename,&ccdnum);
	    if (!strlen(oldnite)) sprintf(oldnite,"%s",nite);
	    else if (strcmp(oldnite,nite)) {
	      printf(" ** Expect catalogs to be from a single nite\n");
	      printf("    Found %s and %s\n",oldnite,nite);
	    }
	    if (!flag_quiet) printf("%s\n runid=%s nite=%s band=%s tilename=%s imagetype=%s ccdnum=%d newimagename=%s\n",imagename,runid,nite,band,tilename,imagetype,ccdnum,newimagename);
	    /* confirm it is a catalog file */
	    if (strncmp(imagetype,"catalog", 7)) {
	      printf("  **runCatalog_ingest: File must contain list of FITS catalogs **\n");
	      exit(0);
	    }
	    /* add runid to current list */
	    /* but only if correct ccdnum if select_ccdnum filter set */
	    if (!flag_ccd || (flag_ccd && select_ccdnum==ccdnum)) {
	      flag=0;
	      for (i=0;i<nrunids;i++) if (!strcmp(runid,runids[i])) flag=1;
	      if (!flag) {
	        sprintf(runids[nrunids],"%s",runid);
	        nrunids++;
	      }
	    }
	  }
	  fclose(fin);
	}

	if (!flag_quiet) {
	  printf("  Catlist contains %d runid",nrunids);
	  if (nrunids>1) printf("s\n");
	  else printf("\n");
	}



	/* ************************************************************* */
	/* ****** now query for list of images with these runids ******* */
	/* ************************************************************* */

	sprintf(filename,"%s.sqlquery",sqlldr_root);
	out=fopen(filename,"w");
        fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF COLSEP '|'");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        /* first find out how many solutions to expect */
        fprintf(out,"SELECT count(*)\n");
        fprintf(out,"FROM FILES\n");
        fprintf(out,"WHERE RUNIDDESC='%s' ",runids[0]);
	for (i=1;i<nrunids;i++) fprintf(out,"\nOR RUNIDDESC='%s'",runids[i]);
	fprintf(out,";\n");
        fprintf(out,"SELECT imageid||'|'||nite||'|'||band||'|'||tilename||'|'||runiddesc||'|'||imagename||'|'||imagetype||'|'||ccd_number||'|'||equinox \n");
        fprintf(out,"FROM FILES\n");
        fprintf(out,"WHERE RUNIDDESC='%s' ",runids[0]);
	for (i=1;i<nrunids;i++) fprintf(out,"\nOR RUNIDDESC='%s'",runids[i]);
	fprintf(out,";\n");
        fprintf(out,"exit;\n");
        fclose(out); 

	/* access dblogin */
	select_dblogin(dblogin);

	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s.sqlquery",dblogin,sqlldr_root);

	/* now make the call and read in image information */
	i=-1;
	pip=popen(sqlcall,"r");
	while (fgets(line,1000,pip)!=NULL) {
	  if (i==-1) {
	    sscanf(line,"%d",&nfiles);
	    if (!flag_quiet) printf("  Selected %d files from db\n",nfiles);
	    if (nfiles==0) {
	      printf("  ** runCatalog_ingest:  Query must select at least one file\n");
	      exit(0);
	    }
	    im=(struct imageinfo *)calloc(nfiles,sizeof(struct imageinfo));
	  }
	  else {
	    /* split the string */
	    //printf(" ** %s",line);
	    splitstring(line,"|",NUM_FIELDS,LEN_FIELDS,templist);
	    /* copy substrings into proper variables */
	    sscanf(&(templist[0*LEN_FIELDS]),"%d",&(im[i].imageid));
	    sscanf(&(templist[1*LEN_FIELDS]),"%s",im[i].nite);
	    sscanf(&(templist[2*LEN_FIELDS]),"%s",im[i].band);
	    sscanf(&(templist[3*LEN_FIELDS]),"%s",im[i].tilename);
	    sscanf(&(templist[4*LEN_FIELDS]),"%s",im[i].runid);
	    sscanf(&(templist[5*LEN_FIELDS]),"%s",im[i].imagename);
	    sscanf(&(templist[6*LEN_FIELDS]),"%s",im[i].imagetype);
	    sscanf(&(templist[7*LEN_FIELDS]),"%d",&(im[i].ccdnum));
	    sscanf(&(templist[8*LEN_FIELDS]),"%f",&(im[i].equinox));
	    if (!flag_quiet) printf("  imageid: %d nite: %s band: %s tilename: %s runid: %s imagename: %s imagetype: %s ccdnum: %d\n",
	      im[i].imageid,im[i].nite,im[i].band,im[i].tilename,
	      im[i].runid,im[i].imagename,im[i].imagetype,im[i].ccdnum);
	  }
	  i++;
	  if (i>nfiles) {
	    printf("  **runCatalog_ingest:  too many files returned from db query \n");
	    exit(0);
	  }
	}
	pclose(pip);

	/* ************************************************************* */
	/* ************     Deal with temporary table     ************** */
	/* ************************************************************* */
	
	sprintf(dbtable,"tmp_objects_%s",nite);	
	if (!table_exists(dblogin,dbtable,sqlldr_root)) {
	  create_table(dblogin,dbtable,"OBJECTS",sqlldr_root);
	if (!flag_quiet) printf("  Created temporary table %s\n",dbtable);
	}
	else if (!flag_quiet) 
	  printf("  Temporary table %s already exists\n",dbtable);
	
	/* prepare control file */
	sprintf(temp,"%s",sqlldr_ctrl);
	temp[strlen(temp)-4]=0;
	sprintf(sqlldr_ctrlout,"%s_%s.ctr",temp,nite);
	sprintf(command,"sed s/table\\ objects/table\\ %s/ %s > %s",
	  dbtable,sqlldr_ctrl,sqlldr_ctrlout);
	system(command);
	if (!flag_quiet) printf("  Created new control file %s\n",sqlldr_ctrlout);
	
	/* ************************************************************* */
	/* now cycle through lists to catalog_ingest the files contained */
	/* ************************************************************* */
	num_tot=num_current=0;
	for (j=0;j<nlists;j++) {
	  fin=fopen(filelist[j],"r");
	  if(!flag_quiet) printf(" ** Processing list %s\n", 
	    filelist[j]);
	  /* read through image list */
	  do {
	    fileread=fscanf(fin,"%s",imagename);
	    /* prepare output data file */
	    if (fileread==EOF || num_current==0 || num_current>SQLLDR_LIMIT) {
	      if (num_current && flag_sqlldrctrl) { /* ingest current data */
	        if (flag_sqlldr_multiingest) {
		  /* make sure we don't exceed multiingest limits*/
		  printf("\n");
		  sleepcounter=0;
		  do {
		    if (sleepcounter>0) sleep(10);
		    sleepcounter++;
		    printf(".");fflush(stdout);
	          } while (numdbjobs("sqlldr",sqlldr_root,dblogin)>=sqlldr_multiingest_limit);
		  printf("\n");
	          sprintf(command,"${ORACLE_HOME}/bin/sqlldr %s control=\"%s\" data=\"%s_%02d\" log=\"%s.sqllog_%02d\" bad=\"%s.sqlbad_%02d\" rows=5000 bindsize=20000000 readsize=20000000 &",
		  dblogin,sqlldr_ctrlout,sqlldr_data,ingest_count,sqlldr_root,
		  ingest_count,sqlldr_root,ingest_count);
	   	  if (!flag_quiet) printf("%s\n",command);
		}
		else {
	          sprintf(command,"${ORACLE_HOME}/bin/sqlldr %s control=\"%s\" data=\"%s_%02d\" log=\"%s.sqllog_%02d\" bad=\"%s.sqlbad_%02d\" rows=5000 bindsize=20000000 readsize=20000000",
		  dblogin,sqlldr_ctrlout,sqlldr_data,ingest_count,sqlldr_root,
		  ingest_count,sqlldr_root,ingest_count);
	   	  if (!flag_quiet) printf("%s\n",command);
	        }
	        if (!flag_quiet) {
	          printf("  %s\n",command);
	          printf("  Ingesting %d objects\n\n",num_current);
		  fflush(stdout);
	        }
	        system(command);
	      }
	      /* now open new data file */
	      /*if (fopen(sqlldr_data,"w")!=NULL) {
		sprintf(command,"rm %s",sqlldr_data);
		system(command);
	      }*/
	      ingest_count++; /* increment file ingest counter */
	      /* reset counter */
	      num_tot+=num_current;
	      num_current=0;
	    } 

	    /* resolve catalog name */
	    if (fileread!=EOF) filename_resolve(imagename,runid,nite,band,tilename,imagetype,
	      newimagename,&ccdnum);

	    if (fileread!=EOF && (!flag_ccd || (flag_ccd && ccdnum==select_ccdnum))) {
	      if (!flag_quiet) printf("  Catalog %s: ",imagename);
	      //if (!flag_quiet) printf("  nite: %s band: %s tilename: %s runid: %s imagename: %s imagetype: %s ccdnum: %d\n",
	      // nite,band,tilename,runid,newimagename,imagetype,ccdnum);
	      /* find the corresponding image file and imageid */
	      match=-1;
	      for (i=0;i<nfiles;i++) {
	        if (!strcmp(newimagename,im[i].imagename) && !strcmp(runid,im[i].runid) 
		  &&  !strcmp(nite,im[i].nite) &&  !strcmp(tilename,im[i].tilename) 
		  &&  !strcmp(band,im[i].band) && ccdnum==im[i].ccdnum &&
		  strcmp(im[i].imagetype,"catalog")) {
		  if (match<0) match=i; 
	    	  else {
		    printf("  **runCatalog_ingest:  more than one image match found for catalog %s:  %d (imageid=%d) and %d (imageid=%d)\n",
		      imagename,match,im[match].imageid,i,im[i].imageid);
		    exit(0);
		  }
	        }
	      }
	      if (match<0) {
	        printf("\n  **runCatalog_ingest:  no matching image file found for %s\n",
		  imagename);
	        exit(0);
	      }
  
	      command[0]=0; /* clear the last command */
	      if (flag_bin) sprintf(command,"%s/catalog_ingest %s %d %s %.2f >> %s_%02d",
		  binpath,imagename,im[match].imageid,band,im[match].equinox,
		  sqlldr_data,ingest_count);
	      else sprintf(command,"catalog_ingest %s %d %s %.2f >> %s_%02d",
		  imagename,im[match].imageid,band,im[match].equinox,
		  sqlldr_data,ingest_count);
	      /* report and execute the command */
	      if (!flag_quiet) {printf("%s\n",command);fflush(stdout);}
	      system (command);
	      /* count the number of entries in the data file */
	      sprintf(command,"wc -l %s_%02d",sqlldr_data,ingest_count);
	      pip=popen(command,"r");
	      fscanf(pip,"%d %s",&num_line,tmp);
	      pclose(pip);
	      num_current=num_line;
	      if (!flag_quiet) printf(" %d objects extracted\n",num_line);
	    }
	  } while (fileread!=EOF);/* completed processing of single catalog */
	  fclose(fin);
	} /* completed processing of a single list */
	if (!flag_quiet) printf("  runCatalog_ingest complete:  %d objects ingested in %d batches\n",num_tot,ingest_count-1);

	return(0);
}

#undef MAXLISTS
#undef MAXIMAGE
#undef SQLLDR_LIMIT
#undef LEN_FIELDS
#undef NUM_FIELDS

