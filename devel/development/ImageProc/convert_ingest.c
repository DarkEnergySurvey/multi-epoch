#include "imageproc.h"


main(argc, argv)
	int argc;
	char *argv[];
{

	char	fiximfile[500],crosstalkfile[500],
		sqlldrcontrol[500],inputnite[500],nite[100],
		command[5000],trash[10000],image[500],nimage[500],
                binpath[500],listname[1000],rootname[1000],
		file_mkdir[1000],file_convert[1000],file_sqldata[1000],
		filename[1000],line[10000],
		arnode[100],arroot[1000],arsites[100],dblogin[500];
	int	i,flag_quiet=0,flag_crosstalk=0,flag_fixim=0,count=0,
		flag_sqlldr=0,flag_gzip=0,ncount=0,flag_mosaic=0,flag,
                flag_decam=0,flag_binpath=0, flag_list=0,flag_phot=0;
	FILE	*out1,*out2,*pip,*out,*inp;
	void	select_dblogin(),select_archivenode();

	if (argc<3) {
	  printf("  convert_ingest <nite> <archive node>\n");
          printf("   -list <listname>\n");
          printf("   -crosstalk <crosstalk matrix- file>\n");
          printf("   -fixim <gain/rdnoise/saturate file>\n");
          printf("   -sqlldr_control <control file>\n");
	  printf("   -detector <Mosaic2 or DECam>\n");
	  printf("   -binpath <binpath>\n");
	  printf("   -photflag <0 or 1>\n");
	  printf("   -gzip\n");
	  printf("   -quiet\n");
	  exit(0);
	}
	sprintf(nite,"%s",argv[1]);
	sprintf(arnode,"%s",argv[2]);

	
        /* process command line */
        for (i=3;i<argc;i++) {
          if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
          if (!strcmp(argv[i],"-gzip")) flag_gzip=1;
          if (!strcmp(argv[i],"-list")) {
	    flag_list=1;
	    i++;
	    sprintf(listname,"%s",argv[i]);
	    sprintf(rootname,"%s.convert_ingest",listname);
	  }
          if (!strcmp(argv[i],"-crosstalk")) {
	    flag_crosstalk=1;
	    i++;
	    sprintf(crosstalkfile,"%s",argv[i]);
	  }
          if (!strcmp(argv[i],"-fixim")) {
	    flag_fixim=1;
	    i++;
	    sprintf(fiximfile,"%s",argv[i]);
	  }
          if (!strcmp(argv[i],"-sqlldr_control")) {
	    flag_sqlldr=1;
	    i++;
	    sprintf(sqlldrcontrol,"%s",argv[i]);
	  }
	  if (!strcmp(argv[i],"-binpath")) {
	    flag_binpath=1;
	    i++;
	    sprintf(binpath,"%s",argv[i]);
	  }
	  if (!strcmp(argv[i],"-detector")) {
	    i++;
	    if (!strcmp(argv[i],"Mosaic2")) flag_mosaic=1;
	    else if (!strcmp(argv[i],"DECam")) flag_decam=1;
	    else { printf(" ** convert_ingest error: wrong input for -camera option\n");
	    exit(0);}
	  }
	  if (!strcmp(argv[i],"-photflag")) 
	    sscanf(argv[++i],"%d",&flag_phot);
	}

	if (!flag_decam && !flag_mosaic) {
	  printf("  Must select a detector (Mosaic2 or DECam)!\n");
	  exit(0);
	}

	if (!flag_list) sprintf(rootname,"%s.convert_ingest",nite);

	if (!flag_quiet) printf("  Examining nite %s and creating archive space\n",
	  nite);

	/* get login information for db */
	select_dblogin(dblogin);

	/* grab the archive node information */
	select_archivenode(dblogin,arnode,arroot,arsites);

	/* first create directory structure needed within the archive */
	if(flag_list) 
	  sprintf(command,"/bin/cat %s",listname);
	else
	  sprintf(command,"/bin/ls %s/raw/%s/src/*.fits*",arroot,nite);
	if (!flag_quiet) printf("  %s\n",command);
	pip=popen(command,"r");
	sprintf(file_mkdir,"%s.mkdir",rootname);
	sprintf(file_convert,"%s.convert",rootname);
	out1=fopen(file_mkdir,"w");
	out2=fopen(file_convert,"w");
	fprintf(out1,"/bin/mkdir -p %s/raw/%s/raw\n",arroot,nite);
	fprintf(out1,"/bin/mkdir -p %s/raw/%s/log\n",arroot,nite);
	while (fscanf(pip,"%s",image)!=EOF) {
	  
	  /* strip the .fits off the filename */
	  sprintf(nimage,"%s",image);
	  for (i=strlen(image);i>=0;i--) {
	    if (!strncmp(&(image[i]),".fits",5)) {
	      image[i]=0;
	      break;
	     }
	  }
	 
	  /* strip off only the image name */
	  for (i=strlen(image);i>=0;i--)
	    if (!strncmp(&(image[i]),"/",1)) {
	      sprintf(nimage,"%s",image+i+1);
	      break;
	    }
	  
	  fprintf(out1,"/bin/mkdir -p %s/raw/%s/raw/%s\n",arroot,nite,nimage);
	  if(flag_mosaic) {
	    if(flag_binpath) {
	      fprintf(out2,"%s/Mosaic2_convert %s.fits %s/raw/%s/raw/%s/%s -quiet",
		      binpath,image,arroot,nite,nimage,nimage);
	    }
	    else {
	      fprintf(out2,"Mosaic2_convert %s.fits %s/raw/%s/raw/%s/%s -quiet",
		      image,arroot,nite,nimage,nimage);
	    }
	  }
	  if(flag_decam) {
	    if(flag_binpath) {
	      fprintf(out2,"%s/DECam_convert %s.fits %s/raw/%s/raw/%s/%s -quiet",
		      binpath,image,arroot,nite,nimage,nimage);
	    }
	    else {
	       fprintf(out2,"DECam_convert %s.fits %s/raw/%s/raw/%s/%s -quiet",
		       image,arroot,nite,nimage,nimage);
	    }
	  }
	  if (flag_crosstalk) fprintf(out2," -crosstalk %s",crosstalkfile);
	  if (flag_fixim) fprintf(out2," -fixim %s",fiximfile);
	  if (flag_gzip) fprintf(out2," -gzip");
	  fprintf(out2," -photflag %d",flag_phot);
	  fprintf(out2,"\n");
	  count++;
	}
	fclose(out1);
	fclose(out2);
	pclose(pip);
	sprintf(command,"/bin/csh %s",file_mkdir);
	if (!flag_quiet) printf("  %s\n",command);
	system(command);
	if (!flag_quiet) printf("  Discovered %d images in the input nite %s\n",
	  count,nite);

	/* now cycle through images splitting them */
	sprintf(command,"/bin/csh %s",file_convert);
	if (!flag_quiet) printf("  %s\n",command);
	pip=popen(command,"r");
	sprintf(file_sqldata,"%s.convert_out",rootname);
	out1=fopen(file_sqldata,"w");
	if (!flag_quiet) {
	  printf("  Converting images: ");
	  fflush(stdout);
	}
	while (fgets(trash,10000,pip)!=NULL) {
	  /*if (!flag_quiet) printf("%s",trash);*/
	  fprintf(out1,"%s",trash);
	  ncount++;
	  if (!flag_quiet) {
	    if (ncount%50==0) printf("\n                     ");
	    else printf(".");
	    fflush(stdout);
	  }
	}
	if (!flag_quiet) {
	  printf("\n");
	  printf("  Conversion produced %d images\n",ncount);
	}
	fclose(out1);
	pclose(pip);

	/* filter the image list to prepare for ingest */
	sprintf(filename,"%s.convert_out",rootname);
	inp=fopen(filename,"r");
	sprintf(filename,"%s.sqldata",rootname);
	out=fopen(filename,"w");
	while (fgets(line,10000,inp)!=NULL) {
	  flag=0;
	  for (i=0;i<strlen(line);i++) if (!strncmp(line+i,"Warning",7)) flag=1;
	  if (!flag && strncmp(line,"  **** FITS Error ****",22)
	    && strncmp(line,"Mosaic2_convert",14)) {
	    fprintf(out,"%s|%s",arsites,line);
	  }
	  else if (strncmp(line,"Mosaic2_convert",14)) printf("ATTN=> %s",line);
	}
	fclose(inp);fclose(out);

	/* ingest image descriptions into the database */
	if (flag_sqlldr) {
	  if (!flag_quiet) printf("  Ingesting %d images into DES database",
	    ncount);
	  sprintf(command,"${ORACLE_HOME}/bin/sqlldr %s control=\"%s\" data=\"%s.sqldata\" log=\"%s.sqllog\" bad=\"%s.sqlbad\" rows=1000 bindsize=20000000 readsize=20000000",
	    dblogin,sqlldrcontrol,rootname,rootname,rootname);
	  system(command);
	}
	if (!flag_quiet) printf("\n");

	/* return flag to make OGRE happy */
	exit(0);
}
