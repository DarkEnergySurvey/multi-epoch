#include "imagegproc.h"

#define GBAND 1
#define RBAND 2
#define IBAND 3
#define ZBAND 4

/* possible truth tables */
#define STRIPE82 0
#define CATSIM1  1
#define USNOB2   2

#define TOLERANCE (2.0/3600.0) /* arcsec */

main(argc,argv)
	int argc;
	char *argv[];
{
	char	truthtable[100],objecttable[100],sqlcall[200],tmp[10],
		band[5][10]={"","g","r","i","z"},oband[100],nite[100],
		truthquery[1000],command[100];
	float	ramin,ramax,decmin,decmax,temp,scale,
		omag,omagerr,oclass,calcdistance(),
		truemag,mindist,dist,
		*sra,*tg,*tr,*ti,*tz,*tclass,
	        x2err_world,y2err_world;
	double	*tra,*tdec,ora,odec;
	int	i,j,flag_quiet=0,flag_ccd=0,flag_band=0,*tmatch,
		nomatches,matches,nobjects,locmatch,loclow,lochi,
		ntruth,flag_nite=0,flag_noquery=0,flag_truth=STRIPE82;
	FILE	*pip,*out,*inp,*outnm;
	unsigned long *tindex;
	void	indexx();

	if (argc<5) {
	  printf("  grabmatches <RAmin> <RAmax> <DECmin> <DECmax>\n");
	  printf("  -band <g,r,i,z> (default is all)\n");
	  printf("  -ccd <# or 0(default) for all>>\n");
	  printf("  -nite <nite>\n");
	  printf("  -truth <\"stripe82\",\"catsim1\">\n");
	  printf("  -noquery\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	sscanf(argv[1],"%f",&ramin);
	sscanf(argv[2],"%f",&ramax);
	sscanf(argv[3],"%f",&decmin);
	sscanf(argv[4],"%f",&decmax);

	/* make sure values are in proper order */
	if (ramax<ramin) {temp=ramin;ramin=ramax;ramax=temp;}
	if (decmax<decmin) {temp=decmin;decmin=decmax;decmax=temp;}
	
	/*  process the rest of the command line */
	for (i=5;i<argc;i++) {
	  if (!strcmp("-quiet",argv[i])) flag_quiet=1;
	  if (!strcmp("-band",argv[i])) {
	    if (!strcmp("g",argv[i+1])) flag_band=GBAND;
	    if (!strcmp("r",argv[i+1])) flag_band=RBAND;
	    if (!strcmp("i",argv[i+1])) flag_band=IBAND;
	    if (!strcmp("z",argv[i+1])) flag_band=ZBAND;
	  }
	  if (!strcmp("-ccd",argv[i])) {
	    sscanf(argv[i+1],"%d",&flag_ccd);
	  }
	  if (!strcmp("-nite",argv[i])) {
	    sscanf(argv[i+1],"%s",nite);
	    flag_nite=1;
	  }
	  if (!strcmp("-truth",argv[i])) {
	    if (!strcmp(argv[i+1],"stripe82")) flag_truth=STRIPE82;
	    else {
	      if (!strcmp(argv[i+1],"catsim1")) flag_truth=CATSIM1;
	      else {
		printf("  ** Currently only set up to match against Stripe82 or CatSim1 truth tables\n");
		exit(0);
	      }
	    }
	    sscanf(argv[i+1],"%d",&flag_ccd);
	  }
	  if (!strcmp("-noquery",argv[i])) flag_noquery=1;
	}

	/*  set tables for comparison */
	if (flag_truth==CATSIM1) {
	  sprintf(truthtable,"USNOB_CAT1");
	  sprintf(truthquery,"SELECT ra,dec,R1,R1,R1,R1 FROM %s\n",
	    truthtable);
	  sprintf(truthquery,"%sWHERE (RA between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DEC between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  /* add a cut on magnitude */
	  if (flag_band==GBAND) sprintf(truthquery,"%sand R1<22.5 ;\n",truthquery);
	  if (flag_band==RBAND) sprintf(truthquery,"%sand R1<22.5 ;\n",truthquery);
	  if (flag_band==0 || flag_band==IBAND) sprintf(truthquery,"%sand R1<22.5 ;\n",truthquery);
	  if (flag_band==ZBAND) sprintf(truthquery,"%sand R1<22.5 ;\n",truthquery);
	}
	if (flag_truth==STRIPE82) {
	  sprintf(truthtable,"des_stripe82_stds_v1");
	  sprintf(truthquery,"SELECT radeg,decdeg,stdmag_g,stdmag_r,stdmag_i,stdmag_z FROM %s\n",
	    truthtable);
	  sprintf(truthquery,"%sWHERE (RADEG between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DECDEG between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  /* add a cut on magnitude */
	  if (flag_band==GBAND) sprintf(truthquery,"%sand stdmag_g<22.5 ;\n",truthquery);
	  if (flag_band==RBAND) sprintf(truthquery,"%sand stdmag_r<22.5 ;\n",truthquery);
	  if (flag_band==0 || flag_band==IBAND) sprintf(truthquery,"%sand stdmag_i<22.5 ;\n",truthquery);
	  if (flag_band==ZBAND) sprintf(truthquery,"%sand stdmag_z<22.5 ;\n",truthquery);
	}
	sprintf(objecttable,"OBJECTS");
        /* set up generic db call */
        sprintf(sqlcall,"sqlplus -S pipeline/dc01user@charon.ncsa.uiuc.edu/DES < grabmatches.sql > grabmatches.tmp");


	/* construct a query */
	if (!flag_quiet && !flag_noquery) printf("  Constructing a query to return objects\n");
	out=fopen("grabmatches.sql","w");
	fprintf(out,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
        fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
        fprintf(out,"SET TERMOUT OFF;\n");

	/* TRUTH table query */
	fprintf(out,"SPOOL grabmatches.truth;\n");
	fprintf(out,"%s",truthquery);
	fprintf(out,"SPOOL OFF;\n");

	/* OBJECT table query */
	fprintf(out,"SPOOL grabmatches.objects;\n");
	fprintf(out,"SELECT objects.alpha_j2000,objects.delta_j2000,objects.MAG_AUTO,objects.MAGERR_AUTO,objects.class_star,objects.band,objects.ERRX2_IMAGE,objects.ERRY2_IMAGE FROM %s",
	  objecttable);
	if (flag_ccd || flag_nite) fprintf(out,",FILES\n");
	else fprintf(out,"\n");
	fprintf(out,"WHERE (OBJECTS.ALPHA_J2000 between %.7f and %.7f)\n",
	  ramin,ramax);
	fprintf(out,"and (OBJECTS.DELTA_J2000 between %.7f and %.7f)\n",
	  decmin,decmax);
	fprintf(out," and OBJECTS.flags<3\n");
	if (flag_band) fprintf(out," and OBJECTS.band='%s' ",band[flag_band]);
	if (flag_ccd) fprintf(out," and FILES.ccd_number=%d ",flag_ccd);
	if (flag_nite) fprintf(out," and FILES.nite='%s' ",nite);
	if (flag_nite || flag_ccd) fprintf(out," and FILES.IMAGEID=OBJECTS.IMAGEID ");
	fprintf(out,"\norder by OBJECTS.ALPHA_J2000;\nSPOOL OFF;\nexit;\n");
	fclose(out);

	if (!flag_quiet && !flag_noquery) printf("  Executing query of tables %s and %s\n",
	  truthtable,objecttable);
	if (!flag_noquery) system(sqlcall);

	/* count the table */
	sprintf(command,"wc grabmatches.truth");
	pip=popen(command,"r");
	fscanf(pip,"%d",&ntruth);
	pclose(pip);
	if (!ntruth) {
	  printf("  ** There are no sources found in this region in %s\n",
	    truthtable);
	  exit(0);
	}
	else if (!flag_quiet) printf("  Reading %d objects in truth tables\n",ntruth);
	
	/* allocate space for this information */
	tra=(double *)calloc(ntruth,sizeof(double));
	sra=(float *)calloc(ntruth,sizeof(float));
	tdec=(double *)calloc(ntruth,sizeof(double));
	tg=(float *)calloc(ntruth,sizeof(float));
	tr=(float *)calloc(ntruth,sizeof(float));
	ti=(float *)calloc(ntruth,sizeof(float));
	tz=(float *)calloc(ntruth,sizeof(float));
	tclass=(float *)calloc(ntruth,sizeof(float));
	tmatch=(int *)calloc(ntruth,sizeof(int));
	tindex=(unsigned long *)calloc(ntruth,sizeof(unsigned long));
	/* read tables and start matching */
	inp=fopen("grabmatches.truth","r");
	i=0; 
	while (fscanf(inp,"%lf %lf %f %f %f %f\n",tra+i,tdec+i,tg+i,
	  tr+i,ti+i,tz+i)!=EOF) {
	  tindex[i]=i;sra[i]=tra[i];
	  if (!strcmp(tmp,"G")) tclass[i]=0;
	  else tclass[i]=1;
	  tmatch[i]=0;
	  i++;
	}
	fclose(inp);
	
	if (!flag_quiet) {
	  printf("  Indexing truth table\n");
	  fflush(stdout);
	}
	/* now sort the truth table by ra to make searches more efficient */
	indexx(ntruth,sra-1,tindex-1);
	for (i=0;i<ntruth;i++) tindex[i]-=1;
	
	/* test indexing */
/*
	for (i=0;i<10;i++) {
	  printf("(%11.7f,%11.7f) (%11.7f,%11.7f) (%11.7f,%11.7f)\n",
	    tra[tindex[i]],tdec[tindex[i]],
	    tra[tindex[i+1000]],tdec[tindex[i+1000]],
	    tra[tindex[i+20000]],tdec[tindex[i+20000]]);
	}
*/
	

	/* now read through object list finding matches */
	if (!flag_quiet) {
	  printf("  Reading object table\n");
	  fflush(stdout);
	}
	inp=fopen("grabmatches.objects","r");
	out=fopen("grabmatches.matches","w");
	
	if (inp==NULL) {
	  printf("  'grabmatches.objects' empty or not found\n");
	  exit(0);
	}
	nobjects=matches=nomatches=0;
	loclow=1;
	while (fscanf(inp,"%lf %lf %f %f %f %s %f %f",&ora,&odec,
	  &omag,&omagerr,&oclass,oband,&x2err_world,&y2err_world)!=EOF) {
	  nobjects++;
	  /*if (!flag_quiet) printf(" %d  %d matches  %.7f %.7f\n",nobjects,matches,ora,odec);*/
	  if (nobjects%5000==0) if (!flag_quiet) printf("  %d matches / %d objects\n",
	    matches,nobjects);
	  /* find nearest neighbor */
	  scale=cos(odec*M_PI/180.0);
	  ramin=ora-TOLERANCE/scale;
	  i=loclow;lochi=-1;
	  if (loclow==0) i=1;
	  while (lochi==-1) {
	    if (tra[tindex[i]]>ramin && tra[tindex[i-1]]<=ramin) {
	      loclow=lochi=i-1;
	      if (loclow<0) loclow=lochi=0;
	    }
	    else { /* adjust the location */
	      if (tra[tindex[i]]<=ramin) i++;
	      else i--;
	    }
	    if (i==0) loclow=lochi=0;
	    if (i>=ntruth-1) loclow=lochi=ntruth-1;
	  }
	  ramax=ora+TOLERANCE/scale;
	  for (i=loclow;i<ntruth;i++) if (tra[tindex[i]]>ramax) break;
	  lochi=i; if (lochi>=ntruth) lochi=ntruth-1;
	  mindist=TOLERANCE;
	  locmatch=-1;
	  for (i=loclow;i<=lochi;i++) {
	    /* if dec offset is acceptable then calculate distance */
	    if (fabs(tdec[tindex[i]]-odec)<TOLERANCE) {
	      dist=calcdistance(tra[tindex[i]],tdec[tindex[i]],ora,odec);
	      if (dist<mindist) {
		mindist=dist;
		locmatch=i;
	      }
	    }
	  }
	  if (locmatch==-1) {
	    nomatches++;
	    /*if (nomatches%10==0) printf("-");*/

	  }
	  else {
	    tmatch[tindex[locmatch]]=1;
	    matches++;/*if (matches%10==0) printf("+");*/
	    if (!strcmp(oband,"g")) truemag=tg[tindex[locmatch]];
	    if (!strcmp(oband,"r")) truemag=tr[tindex[locmatch]];
	    if (!strcmp(oband,"i")) truemag=ti[tindex[locmatch]];
	    if (!strcmp(oband,"z")) truemag=tz[tindex[locmatch]];
	    fprintf(out,"%.7f %.7f  %.4f  %.7f %.7f %.4f %.4f %s %.2f %2.7f %2.7f\n",
	      tra[tindex[locmatch]],tdec[tindex[locmatch]],
	      truemag,scale*(ora-tra[tindex[locmatch]]),
	      odec-tdec[tindex[locmatch]],omag-truemag,omagerr,oband,oclass,x2err_world,y2err_world);
	  }
 	}
	fclose(inp);fclose(out);
	/* output all the unmatched truth table objects */
	outnm=fopen("grabmatches.truth_nomatches","w");
	for (i=0;i<ntruth;i++) {
	  if (!tmatch[i])
	    fprintf(outnm,"%.7f %.7f   %.4f %.4f %.4f %.4f   %.2f\n",
		    tra[i],tdec[i],tg[i],tr[i],ti[i],tz[i],tclass[i]);
	}

	fclose(outnm);
	printf("  Found %d matches out of %d objects (%d objects in truth table)\n",matches,nobjects,ntruth);
}



float calcdistance(ra1,dec1,ra2,dec2)
	double ra1,dec1,ra2,dec2;
{
	double	distance,scale;
	float	outdist;
	
	scale=cos(0.5*(dec1+dec2)*M_PI/180.0);
	distance=Squ(scale*(ra1-ra2))+Squ(dec1-dec2);
	if (distance>1.0e-20) outdist=sqrt(distance);
	else outdist=distance;
	return(distance);
}

