#include "imageproc.h" 

#define GBAND 1
#define RBAND 2
#define IBAND 3
#define ZBAND 4

/* possible truth tables */
#define NUM_CATALOGS 4
#define STANDARDS 1
#define CATSIM1  2
#define USNOB   3
#define CATSIM3  4
#define TOLERANCE (1.0/3600.0) /* arcsec */

main(argc,argv)
	int argc;
	char *argv[];
{
	char	objecttable[100],sqlcall[200],tmp[10],filename[500],
		band[5][10]={"","g","r","i","z"},oband[100],nite[100],
		truthquery[10000],objquy[10000],command[100],runid[100],
		truthtable[NUM_CATALOGS+1][50]={"","standard_stars",
		"imsim2_truth","usno-b2","catsim3_truth"},tilename[100],outputroot[500],
		inputline[5000],**objline,dblogin[500];
	float	ramin,ramax,decmin,decmax,temp,scale,
		omag_g,omagerr_g,omag_r,omagerr_r,omag_i,
		omagerr_i,omag_z,omagerr_z,oclass,calcdistance(),
	        truemag,mindist,dist,fwhm,
		*sra,*tg,*tr,*ti,*tz,*tclass;
	double	tra,tdec,*ora,*odec;
	int	i,j,flag_quiet=0,flag_ccd=0,flag_band=0,*omatch,
		nomatches,matches,nobjects,locmatch,loclow,lochi,
		ntruth,flag_nite=0,flag_noquery=0,flag_runid=0,
		flag_truth=CATSIM1;
	FILE	*pip,*out,*outno,*inp;
	void	select_dblogin();

	if (argc<2) {
	  printf("%s <TileID> \n",argv[0]);
	  printf("  -runid <runid>\n");
	  printf("  -truthtable <Standards, ImSim2_Truth, USNO-B2, CatSim3_Truth>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	sprintf(tilename,"%s",argv[1]);
	sprintf(outputroot,"coadd_grabmatches_%s",tilename);

	/* set default truth table */
	flag_truth=CATSIM1;

	/*  process the rest of the command line */
	for (i=2;i<argc;i++) {
	  if (!strcmp("-quiet",argv[i])) flag_quiet=1;
	  if (!strcmp("-runid",argv[i])) {
	    sscanf(argv[i+1],"%s",runid);
	    flag_runid=1;
	  }
	  if (!strcmp("-truthtable",argv[i])) {
	    if (!strcmp(argv[i+1],"Standards")) flag_truth=STANDARDS;
	    else if (!strcmp(argv[i+1],"ImSim2_Truth")) flag_truth=CATSIM1;
	    else if (!strcmp(argv[i+1],"USNO-B2")) flag_truth=USNOB;
	    else if (!strcmp(argv[i+1],"CatSim3_Truth")) flag_truth=CATSIM3;
	    else {
	      printf("  Truthtable name %s not known\n",argv[i+1]);
	      exit(0);
	    }
	    printf("  Using truthtable %s\n",truthtable[flag_truth]);
	  }
	}

	/* set name of objects table */
	sprintf(objecttable,"coadd_objects");

	/* load in db access information from environment */
	select_dblogin(dblogin);
        /* set up generic db call */
        sprintf(sqlcall,"sqlplus -S %s < %s.sql > /dev/null",
	  dblogin,outputroot,outputroot);




	/* *************************************** */
	/* ***** Query Coadd_objects Table ******* */
	/* *************************************** */

	/* set up query for coadd objects */
	sprintf(objquy,"SELECT coadd_objects.alpha_j2000,coadd_objects.delta_j2000,");
	sprintf(objquy,"%scoadd_objects.mag_auto_g,coadd_objects.magerr_auto_g,coadd_objects.CLASS_STAR_g,coadd_objects.mag_auto_r,coadd_objects.magerr_auto_r,coadd_objects.CLASS_STAR_r,coadd_objects.mag_auto_i,coadd_objects.magerr_auto_i,coadd_objects.CLASS_STAR_i,coadd_objects.mag_auto_z,coadd_objects.magerr_auto_z,coadd_objects.CLASS_STAR_z\n",objquy);
	sprintf(objquy,"%sFROM %s,files\n",objquy,objecttable);
	sprintf(objquy,"%sWHERE files.imageid=coadd_objects.imageid_g\n",
	  objquy);
	sprintf(objquy,"%sAND coadd_OBJECTS.flags_g<1 AND coadd_OBJECTS.flags_r<1 AND coadd_OBJECTS.flags_i<1 AND coadd_OBJECTS.flags_z<1 \n",objquy);
	sprintf(objquy,"%sAND files.imagetype='coadd' AND files.tilename='%s'\n",
	objquy,tilename);
	if (flag_runid) sprintf(objquy,"%s AND files.runiddesc like '%s%%'\n",
	  objquy,runid);
	sprintf(objquy,"%sORDER by coadd_OBJECTS.ALPHA_J2000;\n",objquy);


	/* construct a query */
	if (!flag_quiet && !flag_noquery) 
	  printf("  Constructing a query to return objects\n");
	sprintf(filename,"%s.sql",outputroot);
	out=fopen(filename,"w");
	fprintf(out,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        fprintf(out,"SET TERMOUT OFF;\n");

	/* OBJECT table query */
	fprintf(out,"SPOOL %s.objects;\n",outputroot);
	fprintf(out,"%s",objquy);
	fprintf(out,"SPOOL OFF;\nEXIT;\n");
	fclose(out);

	if (!flag_quiet && !flag_noquery) printf("  Executing query of table %s\n",
	  objecttable);
	if (!flag_noquery) system(sqlcall);

	/* count the table */
	sprintf(command,"wc %s.objects",outputroot);
	pip=popen(command,"r");
	fscanf(pip,"%d",&nobjects);
	pclose(pip);
	if (!nobjects) {
	  printf("  ** There are no sources found in tile %s\n",tilename);
	  exit(0);
	}
	else if (!flag_quiet) printf("  Reading %d objects from %s\n",
	  nobjects,objecttable);
	
	/* allocate space for this information */
	ora=(double *)calloc(nobjects,sizeof(double));
	odec=(double *)calloc(nobjects,sizeof(double));
	omatch=(int *)calloc(nobjects,sizeof(int));
	objline=(char **)calloc(nobjects,sizeof(char *));
	for (i=0;i<nobjects;i++) 
	  objline[i]=(char *)calloc(5000,sizeof(char));

	/* read tables and start matching */
	sprintf(filename,"%s.objects",outputroot);
	inp=fopen(filename,"r");
	i=0; 
	while (fgets(objline[i],5000,inp)!=NULL) {
	  sscanf(objline[i],"%lf %lf\n",ora+i,odec+i);
	  objline[i][strlen(objline[i])-1]=0;
	  /* track the range in both RA and DEC */
	  if (!i) {
	    ramin=ramax=ora[i];
	    decmin=decmax=odec[i];
	  }
	  if (ora[i]>ramax) ramax=ora[i];
	  if (ora[i]<ramin) ramin=ora[i];
	  if (odec[i]>decmax) decmax=odec[i];
	  if (odec[i]<decmin) decmin=odec[i];
	  i++;
	  if (i>nobjects) {
	    printf("  ** More objects than expected (%d) in %s\n",
	      nobjects,objecttable);
	  }
	}
	fclose(inp);
	
	if (!flag_quiet) {
	  printf("  Range of RA:  %.10f %.10f  DEC:  %.10f %.10f\n",
	    ramin,ramax,decmin,decmax); 
	}

	/* *************************************** */
	/* ***** set up truth table query ******** */
	/* *************************************** */

	if (flag_truth==CATSIM1) {
	  sprintf(truthquery,"SELECT ra,dec,g_mag,r_mag,i_mag,z_mag,class FROM %s\n",
	    truthtable[flag_truth]);
	  sprintf(truthquery,"%sWHERE (RA between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DEC between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  sprintf(truthquery,"%sORDER by ra;\n",truthquery);
	}

	if (flag_truth==USNOB) {
	  sprintf(truthquery,"SELECT ra,dec,sra,sdec FROM %s\n",
	    truthtable[flag_truth]);
	  sprintf(truthquery,"%sWHERE (RA between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DEC between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  sprintf(truthquery,"%sORDER by ra;\n",truthquery);
	}

	if (flag_truth==STANDARDS) {
	  sprintf(truthquery,"SELECT radeg,decdeg,stdmag_g,stdmagerr_g,stdmag_r,stdmagerr_r,stdmag_i,stdmagerr_i,stdmag_z,stdmagerr_z FROM %s\n",
	    truthtable[flag_truth]);
	  sprintf(truthquery,"%sWHERE (RAdeg between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DECdeg between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  sprintf(truthquery,"%sORDER by radeg;\n",truthquery);
	}

	if (flag_truth==CATSIM3) {
	  sprintf(truthquery,"SELECT ra,dec,g_mag,r_mag,i_mag,z_mag,class FROM %s\n",
	    truthtable[flag_truth]);
	  sprintf(truthquery,"%sWHERE (RA between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DEC between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  sprintf(truthquery,"%sORDER by ra;\n",truthquery);
	}

	

	/* *************************************** */
	/* ***** submit truth table query ******** */
	/* *************************************** */

	if (!flag_quiet && !flag_noquery) printf("  Constructing a query to return truth table objects\n");
	sprintf(filename,"%s.sql",outputroot);
	out=fopen(filename,"w");
	fprintf(out,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        fprintf(out,"SET TERMOUT OFF;\n");

	/* OBJECT table query */
	fprintf(out,"SPOOL %s.truth;\n",outputroot);
	fprintf(out,"%s",truthquery);
	fprintf(out,"SPOOL OFF;\nEXIT;\n");
	fclose(out);

	if (!flag_quiet && !flag_noquery) printf("  Executing query of table %s\n",
	  truthtable);
	if (!flag_noquery) system(sqlcall);

	/* now read through truth table finding matches */
	if (!flag_quiet) {
	  printf("  Reading truth table %s\n",truthtable);
	  fflush(stdout);
	}
	sprintf(filename,"%s.truth",outputroot);
	inp=fopen(filename,"r");
	sprintf(filename,"%s.matches",outputroot);
	out=fopen(filename,"w");
	sprintf(filename,"%s.nomatches",outputroot);
	outno=fopen(filename,"w");
	
	if (inp==NULL) {
	  printf("  '%s.truth' empty or not found\n",outputroot);
	  exit(0);
	}
	ntruth=matches=nomatches=0;
	loclow=1;
	while (fgets(inputline,5000,inp)!=NULL) {
	  sscanf(inputline,"%lf %lf",&tra,&tdec);
	  ntruth++;
	  if (ntruth%5000==0 && !flag_quiet)
	    printf("  %d matches / %d objects\n",matches,nobjects);
	  /* find nearest neighbor */
	  scale=cos(tdec*M_PI/180.0);
	  ramin=tra-TOLERANCE/scale;
	  i=loclow;lochi=-1;
	  if (loclow==0) i=1;
	  while (lochi==-1) {
	    if (ora[i]>ramin && ora[i-1]<=ramin) {
	      loclow=lochi=i-1;
	      if (loclow<0) loclow=lochi=0;
	    }
	    else { /* adjust the location */
	      if (ora[i]<=ramin) i++;
	      else i--;
	    }
	    if (i==0) loclow=lochi=0;
	    if (i>=nobjects-1) loclow=lochi=nobjects-1;
	  }
	  ramax=tra+TOLERANCE/scale;
	  for (i=loclow;i<nobjects;i++) if (ora[i]>ramax) break;
	  lochi=i; if (lochi>=nobjects) lochi=nobjects-1;
	  mindist=TOLERANCE;
	  locmatch=-1;
	  for (i=loclow;i<=lochi;i++) {
	    /* if dec offset is acceptable then calculate distance */
	    if (fabs(odec[i]-tdec)<TOLERANCE) {
	      dist=calcdistance(tra,tdec,ora[i],odec[i]);
	      if (dist<mindist) {
		mindist=dist;
		locmatch=i;
	      }
	    }
	  }
	  if (locmatch==-1) {
	    nomatches++;
	    fprintf(outno,"%s",inputline);
	  }
	  else {
	    //omatch[i]=1;
	    omatch[locmatch]=1;
	    matches++;
	    fprintf(out,"%s %s",objline[locmatch],inputline);
	  }
 	}
	fclose(inp);fclose(out);
	printf("  Found %d matches out of %d objects (%d objects in truth table)\n",matches,nobjects,ntruth);
}



float calcdistance(ra1,dec1,ra2,dec2)
	double ra1,dec1,ra2,dec2;
{
	double	distance,scale;
	float	outdist;
	
	scale=cos(0.5*(dec1+dec2)*M_PI/180.0);
	distance=Squ(scale*(ra1-ra2))+Squ((dec1-dec2));
	if (distance>1.0e-20) outdist=sqrt(distance);
	else outdist=distance;
	return(distance);
}

#undef GBAND
#undef RBAND
#undef IBAND
#undef ZBAND
#undef NUM_CATALOGS
#undef STANDARDS
#undef CATSIM1
#undef USNOB
#undef CATSIM3 
#undef TOLERANCE
