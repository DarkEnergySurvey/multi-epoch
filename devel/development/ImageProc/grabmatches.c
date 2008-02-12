#include "imageproc.h" 

/* possible truth tables */
#define STRIPE82 0
#define CATSIM1  1
#define IMSIM2T  2
#define USNOB2   3
#define STANDARD 4
#define CATSIM3  5

#define TOLERANCE (2.0/3600.0) /* arcsec */

main(argc,argv)
	int argc;
	char *argv[];
{
	char	truthtable[100],objecttable[100],sqlcall[200],tmp[10],
		band[5],oband[100],nite[100],
		truthquery[1000],command[100],dblogin[500],runid[200],
		magin[25],magerrin[25];
	float	ramin,ramax,decmin,decmax,temp,scale,
		omag,omagerr,oclass,calcdistance(),
	        truemag,mindist,dist,inclass,
		*sra,*tg,*tr,*ti,*tz,*tclass;
	double	*tra,*tdec,ora,odec;
	int	i,j,k,flag_quiet=0,flag_ccd=0,flag_band=2,*tmatch,
	        nomatches,matches,nobjects,locmatch,loclow,lochi,random,
	        ntruth,flag_nite=0,flag_noquery=0,flag_runid=0,flag_truth,
	        flag_mag=0,flag_random=0,flag_star=0,flag_galaxy=0,flag_getband=0,flag;
	FILE	*pip,*out,*inp,*outnm;
	unsigned long *tindex;
	void	indexx(),select_dblogin();

	if (argc<5) {
	  printf("  grabmatches <RAmin> <RAmax> <DECmin> <DECmax>\n");
	  printf("  -band <g,r,i,z> (default is all)\n");
	  printf("  -ccd <# or 0(default) for all>>\n");
	  printf("  -nite <nite>\n");
	  printf("  -runid <runid>\n");
	  printf("  -truth <stripe82,catsim1,imsim2_truth,usnob2,catsim3_truth,standard_stars>\n");
	  printf("  -staronly <#> \n");
	  printf("  -galaxyonly <#> \n");
	  printf("  -magtype <#> (default is 0)\n");
	  printf("           (0 = mag_auto)\n");
	  printf("           (1 = mag_aper1)\n");
	  printf("           (2 = mag_aper2)\n");
	  printf("           (3 = mag_aper3)\n");
	  printf("           (4 = mag_aper4)\n");
	  printf("           (5 = mag_aper5)\n");
	  printf("           (6 = mag_aper6)\n");
	  printf("  -random <#>\n");
	  printf("  -noquery\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	sscanf(argv[1],"%f",&ramin);
	sscanf(argv[2],"%f",&ramax);
	sscanf(argv[3],"%f",&decmin);
	sscanf(argv[4],"%f",&decmax);

	/* set default truth table and SExtractor flag */
	flag_truth=IMSIM2T;
	flag=3;
       

	/* grab dblogin */
	select_dblogin(dblogin);

	/* make sure values are in proper order */
	if (ramax<ramin) {temp=ramin;ramin=ramax;ramax=temp;}
	if (decmax<decmin) {temp=decmin;decmin=decmax;decmax=temp;}
	
	/*  process the rest of the command line */
	for (i=5;i<argc;i++) {
	  if (!strcmp("-quiet",argv[i])) flag_quiet=1;
	  if (!strcmp("-band",argv[i])) { 
	    sprintf(band,"%s",argv[i+1]);
	    flag_getband=1;
	    if (!strcmp("g",argv[i+1])) flag_band=GBAND;
	    if (!strcmp("r",argv[i+1])) flag_band=RBAND;
	    if (!strcmp("i",argv[i+1])) flag_band=IBAND;
	    if (!strcmp("z",argv[i+1])) flag_band=ZBAND;
	  }
	  if (!strcmp("-ccd",argv[i])) {
	    flag_ccd=1;
	    sscanf(argv[i+1],"%d",&flag_ccd);
	  }
	  if (!strcmp("-staronly",argv[i])) {
	    flag_star=1;
	    sscanf(argv[i+1],"%f",&inclass);
	  }
	  if (!strcmp("-galaxyonly",argv[i])) {
	    flag_galaxy=1;
	    sscanf(argv[i+1],"%f",&inclass);
	  }
	  if (!strcmp("-nite",argv[i])) {
	    sscanf(argv[i+1],"%s",nite);
	    flag_nite=1;
	  }
	  if (!strcmp("-magtype",argv[i])) {
	    sscanf(argv[i+1],"%d",&flag_mag);
	  }
	  if (!strcmp("-runid",argv[i])) {
	    sscanf(argv[i+1],"%s",runid);
	    flag_runid=1;
	  }
	  if (!strcmp("-truth",argv[i])) {
	    if (!strcmp(argv[i+1],"stripe82")) flag_truth=STRIPE82;
	    else if (!strcmp(argv[i+1],"catsim1")) flag_truth=CATSIM1;
	    else if (!strcmp(argv[i+1],"imsim2_truth")) flag_truth=IMSIM2T;
	    else if (!strcmp(argv[i+1],"usnob2")) flag_truth=USNOB2;
	    else if (!strcmp(argv[i+1],"standard_stars")) flag_truth=STANDARD;
	    else if (!strcmp(argv[i+1],"catsim3_truth")) flag_truth=CATSIM3;
	    else {
	      printf("  ** Wrong input of truth table, abort!\n");
	      exit(0);
	    }
	  }
	  if (!strcmp("-random",argv[i])) {
	    flag_random=1;
	    sscanf(argv[i+1],"%d",&random);
	  }
	  if (!strcmp("-noquery",argv[i])) flag_noquery=1;
	}

	/* set input magtype */
	switch(flag_mag) {
	case 0: sprintf(magin,"objects.mag_auto"); sprintf(magerrin,"%s","objects.magerr_auto"); 
	  break; 
	case 1: sprintf(magin,"objects.mag_aper_1"); sprintf(magerrin,"%s","objects.magerr_aper_1"); 
	  break; 
	case 2: sprintf(magin,"objects.mag_aper_2"); sprintf(magerrin,"%s","objects.magerr_aper_2"); 
	  break; 
	case 3: sprintf(magin,"objects.mag_aper_3"); sprintf(magerrin,"%s","objects.magerr_aper_3"); 
	  break; 
	case 4: sprintf(magin,"objects.mag_aper_4"); sprintf(magerrin,"%s","objects.magerr_aper_4"); 
	  break; 
	case 5: sprintf(magin,"objects.mag_aper_5"); sprintf(magerrin,"%s","objects.magerr_aper_5"); 
	  break; 
	case 6: sprintf(magin,"objects.mag_aper_6"); sprintf(magerrin,"%s","objects.magerr_aper_6"); 
	  break; 
	default: printf(" ** %s error: wrong input of <magtype> for -magtype\n", argv[0]); exit(0);
	}


	/*  set tables for comparison */
	if (flag_truth==CATSIM1) {
	  sprintf(truthtable,"CatSim1_truth");
	  sprintf(truthquery,"SELECT ra,dec,g_mag,r_mag,i_mag,z_mag FROM %s\n",
	    truthtable);
	  sprintf(truthquery,"%sWHERE (RA between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DEC between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  /* class_star separation */
	  //if(flag_star) sprintf(truthquery,"%sand class='S'\n",truthquery);
	  //if(flag_galaxy) sprintf(truthquery,"%sand class='G'\n",truthquery);
	  /* add a cut on magnitude */
	  if (flag_band==GBAND) sprintf(truthquery,"%sand g_mag<22.5 ;\n",truthquery);
	  if (flag_band==RBAND) sprintf(truthquery,"%sand r_mag<22.5 ;\n",truthquery);
	  if (flag_band==IBAND) sprintf(truthquery,"%sand i_mag<22.5 ;\n",truthquery);
	  if (flag_band==ZBAND) sprintf(truthquery,"%sand z_mag<22.5 ;\n",truthquery);
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
	  if (flag_band==IBAND) sprintf(truthquery,"%sand stdmag_i<22.5 ;\n",truthquery);
	  if (flag_band==ZBAND) sprintf(truthquery,"%sand stdmag_z<22.5 ;\n",truthquery);
	}

	if (flag_truth==IMSIM2T) {
	  sprintf(truthtable,"Imsim2_Truth");
	  sprintf(truthquery,"SELECT ra,dec,g_mag,r_mag,i_mag,z_mag FROM %s\n",
	    truthtable);
	  sprintf(truthquery,"%sWHERE (RA between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DEC between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  /* class_star separation */
	  //if(flag_star) sprintf(truthquery,"%sand class='S'\n",truthquery);
	  //if(flag_galaxy) sprintf(truthquery,"%sand class='G'\n",truthquery);
	  /* add a cut on magnitude */
	  if (flag_band==GBAND) sprintf(truthquery,"%sand g_mag<22.5 ;\n",truthquery);
	  if (flag_band==RBAND) sprintf(truthquery,"%sand r_mag<22.5 ;\n",truthquery);
	  if (flag_band==IBAND) sprintf(truthquery,"%sand i_mag<22.5 ;\n",truthquery);
	  if (flag_band==ZBAND) sprintf(truthquery,"%sand z_mag<22.5 ;\n",truthquery);
	}

	if (flag_truth==USNOB2) {
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
	  if (flag_band==IBAND) sprintf(truthquery,"%sand R1<22.5 ;\n",truthquery);
	  if (flag_band==ZBAND) sprintf(truthquery,"%sand R1<22.5 ;\n",truthquery);
	}

	if (flag_truth==STANDARD) {
	  sprintf(truthtable,"standard_stars");
	  sprintf(truthquery,"SELECT radeg,decdeg,stdmag_g,stdmag_r,stdmag_i,stdmag_z FROM %s\n",
	    truthtable);
	  sprintf(truthquery,"%sWHERE (RADEG between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DECDEG between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  /* add a cut on magnitude */
	  if (flag_band==GBAND) sprintf(truthquery,"%sand stdmag_g<22.5 ;\n",truthquery);
	  if (flag_band==RBAND) sprintf(truthquery,"%sand stdmag_r<22.5 ;\n",truthquery);
	  if (flag_band==IBAND) sprintf(truthquery,"%sand stdmag_i<22.5 ;\n",truthquery);
	  if (flag_band==ZBAND) sprintf(truthquery,"%sand stdmag_z<22.5 ;\n",truthquery);
	}

	if (flag_truth==CATSIM3) {
	  sprintf(truthtable,"catsim3_truth");
	  sprintf(truthquery,"SELECT ra,dec,g_mag,r_mag,i_mag,z_mag FROM %s\n",
	    truthtable);
	  sprintf(truthquery,"%sWHERE (RA between %.7f and %.7f)\n",
	    truthquery,ramin,ramax);
	  sprintf(truthquery,"%sand (DEC between %.7f and %.7f)\n",
	    truthquery,decmin,decmax);
	  /* class_star separation */
	  //if(flag_star) sprintf(truthquery,"%sand class='S'\n",truthquery);
	  //if(flag_galaxy) sprintf(truthquery,"%sand class='G'\n",truthquery);
	  /* add a cut on magnitude */
	  if (flag_band==GBAND) sprintf(truthquery,"%sand g_mag<22.5 ;\n",truthquery);
	  if (flag_band==RBAND) sprintf(truthquery,"%sand r_mag<22.5 ;\n",truthquery);
	  if (flag_band==IBAND) sprintf(truthquery,"%sand i_mag<22.5 ;\n",truthquery);
	  if (flag_band==ZBAND) sprintf(truthquery,"%sand z_mag<22.5 ;\n",truthquery);
	}

	/* set the object table */
	sprintf(objecttable,"OBJECTS");

        /* set up generic db call */
        sprintf(sqlcall,"sqlplus -S %s < grabmatches.sql > grabmatches.tmp",dblogin);


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
	fprintf(out,"SELECT objects.alpha_j2000,objects.delta_j2000,%s,%s,objects.class_star,objects.band FROM %s",
	  magin,magerrin,objecttable);
	if (flag_ccd || flag_nite) fprintf(out,",FILES\n");
	else fprintf(out,"\n");
	fprintf(out,"WHERE (OBJECTS.ALPHA_J2000 between %.7f and %.7f)\n",
	  ramin,ramax);
	fprintf(out,"and (OBJECTS.DELTA_J2000 between %.7f and %.7f)\n",
	  decmin,decmax);
	fprintf(out," and OBJECTS.flags<%d\n",flag);
	if(flag_getband) {
	  if(flag_band==GBAND) fprintf(out," and OBJECTS.band='g' ");
	  if(flag_band==RBAND) fprintf(out," and OBJECTS.band='r' ");
	  if(flag_band==IBAND) fprintf(out," and OBJECTS.band='i' ");
	  if(flag_band==ZBAND) fprintf(out," and OBJECTS.band='z' ");
	}
	if(flag_ccd) fprintf(out," and FILES.ccd_number=%d ",flag_ccd);
	if(flag_nite) fprintf(out," and FILES.nite='%s' ",nite);
	if(flag_runid) fprintf(out," and FILES.runiddesc like '%s%%' ",runid);
	if(flag_star) fprintf(out," and OBJECTS.CLASS_STAR > %2.2f ",inclass);
	if(flag_galaxy) fprintf(out," and OBJECTS.CLASS_STAR < %2.2f ",inclass);
	fprintf(out," and FILES.IMAGEID=OBJECTS.IMAGEID ");
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
	  printf("  Reading %s table\n",objecttable);
	  fflush(stdout);
	}
	inp=fopen("grabmatches.objects","r");
	out=fopen("grabmatches.matches","w");
	
	if (inp==NULL) {
	  printf("  'grabmatches.objects' empty or not found\n");
	  exit(0);
	}
	nobjects=matches=nomatches=0;
	loclow=1; k=0;
	while (fscanf(inp,"%lf %lf %f %f %f %s",&ora,&odec,
	  &omag,&omagerr,&oclass,oband)!=EOF) {
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
	    if (!strcmp(oband,"g")) { truemag=tg[tindex[locmatch]]; flag_band=GBAND; }
	    if (!strcmp(oband,"r")) { truemag=tr[tindex[locmatch]]; flag_band=RBAND; }
	    if (!strcmp(oband,"i")) { truemag=ti[tindex[locmatch]]; flag_band=IBAND; }
	    if (!strcmp(oband,"z")) { truemag=tz[tindex[locmatch]]; flag_band=ZBAND; }

	    if(flag_random) {
	      if(k%random==0)
		fprintf(out,"%.7f %.7f  %.4f  %.7f %.7f %.4f %.4f %s %.2f %d\n",
			tra[tindex[locmatch]],tdec[tindex[locmatch]],
			truemag,scale*(ora-tra[tindex[locmatch]]),
			odec-tdec[tindex[locmatch]],omag-truemag,omagerr,oband,oclass,flag_band);
	    }
	    else 
	      fprintf(out,"%.7f %.7f  %.4f  %.7f %.7f %.4f %.4f %s %.2f %d\n",
		      tra[tindex[locmatch]],tdec[tindex[locmatch]],
		      truemag,scale*(ora-tra[tindex[locmatch]]),
		      odec-tdec[tindex[locmatch]],omag-truemag,omagerr,oband,oclass,flag_band);
	  }
	  k++;
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
	distance=Squ(scale*(ra1-ra2))+Squ((dec1-dec2));
	if (distance>1.0e-20) outdist=sqrt(distance);
	else outdist=distance;
	return(distance);
}


#undef STRIPE82 
#undef CATSIM1  
#undef IMSIM2T  
#undef USNOB2   
#undef STANDARD 
#undef CATSIM3  

#undef TOLERANCE
