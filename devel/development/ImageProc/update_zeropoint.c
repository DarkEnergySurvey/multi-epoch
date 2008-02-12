#include "imageproc.h"

#define BAND_ALL	5
#define BAND_g		0
#define BAND_r		1
#define BAND_i		2
#define BAND_z		3

#define ORIGIN_PSMFIT	1
#define ORIGIN_USNOB	2
#define ORIGIN_COADD	3

main(argc,argv)
	int argc;
	char *argv[];
{
	char	command[200],sqlcall[500],nite[50],filter[5][10],
		line[1000],sgn[10];
	int	i,j,flag_band=BAND_ALL,flag_quiet=0,nfit,nimages,
		nobjects,imageid,objectid,maxzp_n,allobjects=0,
		flag_ccd=0,rightfit;
	float	zeropoint;
	db_psmfit 	*fit;
	db_files 	*im;
	db_zeropoint 	*zp;
	FILE	*out,*pip,*out2;

	if (argc<2) {
	  printf("update_zeropoint <nite>\n");
	  printf("  -band <g r i z or all(default)>\n");
	  printf("  -ccd <# (0=all is the default)>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	sprintf(nite,"%s",argv[1]);
	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-band")) { 
	    i++;
	    if (i>=argc) {
	      printf("  **Must include band choice with -band directive\n");
	      exit(0);
	    }
	    if (!strcmp(argv[i],"g")) flag_band=BAND_g;
	    if (!strcmp(argv[i],"r")) flag_band=BAND_r;
	    if (!strcmp(argv[i],"i")) flag_band=BAND_i;
	    if (!strcmp(argv[i],"z")) flag_band=BAND_z;
	    if (!strcmp(argv[i],"all")) flag_band=BAND_ALL;
	  }
	  if (!strcmp(argv[i],"-ccd")) sscanf(argv[i+1],"%d",&flag_ccd);
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}

	sprintf(filter[BAND_g],"g");
	sprintf(filter[BAND_r],"r");
	sprintf(filter[BAND_i],"i");
	sprintf(filter[BAND_z],"z");

	/* set up generic db call */
	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S pipeline/dc01user@charon/DES < update_zeropoint.sql");


	/* ************************************************** */
	/* ************* QUERY for Solutions **************** */
	/* ************************************************** */

	out=fopen("update_zeropoint.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	/* first find out how many solutions to expect */
	fprintf(out,"SELECT count(*)\n");
	fprintf(out,"FROM psmfit\n");
	/* NOTE:  CCDID=0 is when single solution for all CCD's is available */
	fprintf(out,"WHERE NITE='%s' ",nite); 
	if (flag_band!=BAND_ALL) fprintf(out,"AND FILTER='%s';\n",filter[flag_band]);
	else fprintf(out,";\n");
	/* now ask for line to be returned */
	fprintf(out,"SELECT PSMFIT_ID,NITE,FILTER,CCDID,A,AERR,B,BERR,K,");
	fprintf(out,"KERR,RMS,CHI2,DOF,PHOTOMETRICFLAG\n");
	fprintf(out,"FROM psmfit\n");
	/* NOTE:  CCDID=0 is when single solution for all CCD's is available */
	fprintf(out,"WHERE NITE='%s' ",nite);
	if (flag_band!=BAND_ALL) 
	  fprintf(out,"AND FILTER='%s';\n",filter[flag_band]);
	else fprintf(out,";\n");
	fprintf(out,"exit;\n");
	fclose(out);
	
	i=-1;
	pip=popen(sqlcall,"r");
	while (fgets(line,1000,pip)!=NULL) {
	  if (i==-1) {
	    sscanf(line,"%d",&nfit);
	    /*if (nfit>1 && flag_band!=BAND_ALL) {
	      printf("  ** Multiple solutions for a given night and band\n");
	      exit(0);
	    }*/
	    if (!flag_quiet) printf("  Reading %d solutions from db\n",nfit);
	    fit=(db_psmfit *)calloc(nfit,sizeof(db_psmfit));
	    if (!flag_quiet) 
	      printf("  **    ID Nite       FILTER          CCDID      A          AERR       B         BERR     K        KERR        RMS       CHI2         DOF           PHOTFLAG\n");
	  }
	  else {
	  if (!flag_quiet) printf("  ** %s",line);
	  /* read information into local variables */
	  sscanf(line,"%d %s %s %d %f %f %f %f %f %f %f %f %d %d %s %s %s",
	    &(fit[i].fitid),fit[i].nite,fit[i].band,&(fit[i].ccdid), 
	    &(fit[i].a), &(fit[i].aerr),
	    &(fit[i].b), &(fit[i].berr), &(fit[i].k), &(fit[i].kerr),
	    &(fit[i].rms), &(fit[i].chi2), &(fit[i].dof), &(fit[i].photflag));
	  }
	  i++;
	}
	pclose(pip);

	/* must filter fit array here unless we rely on db */

	/* ************************************************** */
	/* ************** QUERY for Images ****************** */
	/* ************************************************** */

	out=fopen("update_zeropoint.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	/* first find out how many solutions to expect */
	fprintf(out,"SELECT count(*)\n");
	fprintf(out,"FROM files\n");
	fprintf(out,"WHERE NITE='%s' AND (upper(IMAGETYPE)='REDUCED' or upper(IMAGETYPE)='REMAP') ",nite);
	if (flag_ccd) fprintf(out,"AND CCD_NUMBER=%d ",flag_ccd);
	if (flag_band!=BAND_ALL) 
	  fprintf(out,"AND BAND='%s';\n",filter[flag_band]);
	else fprintf(out,";\n");
	/* now ask for line to be returned */
	fprintf(out,"SELECT IMAGEID,CCD_NUMBER,BAND,AIRMASS,EXPTIME\n");
	fprintf(out,"FROM files \n");
	fprintf(out,"WHERE NITE='%s' AND (upper(IMAGETYPE)='REDUCED' or upper(IMAGETYPE)='REMAP') ",nite);
	if (flag_ccd) fprintf(out,"AND CCD_NUMBER=%d ",flag_ccd);
	if (flag_band!=BAND_ALL) 
	  fprintf(out,"AND BAND='%s';\n",filter[flag_band]);
	else fprintf(out,";\n");
	fprintf(out,"exit;\n");
	fclose(out);
	
	i=-1;
	pip=popen(sqlcall,"r");
	while (fgets(line,1000,pip)!=NULL) {
	  if (i==-1) {
	    sscanf(line,"%d",&nimages);
	    if (!flag_quiet) printf("  Reading %d Reduced Images from db\n",
	      nimages);
	    im=(db_files *)calloc(nimages,sizeof(db_files));
	    zp=(db_zeropoint *)calloc(nimages,sizeof(db_zeropoint));
	    if (!flag_quiet) printf("  ** ###      IMAGEID   CCDID BAND        AIRMASS    EXPTIME     ZP     SIGMA\n");
	  }
	  else {
	    sscanf(line,"%d %d %s %f %f",&(im[i].imageid),&(im[i].ccdid),
	      im[i].band, &(im[i].airmass), &(im[i].exptime));
	    /* find the corresponding solution */
	    rightfit=-1;
	    for (j=0;j<nfit;j++) {
	      if (!strcmp(im[i].band,fit[j].band)) { /* band matches */
	        if (im[i].ccdid==fit[j].ccdid) rightfit=j;
	        /* only use general fit if fit for ccd isn't available */
	        if (fit[j].ccdid==0 && rightfit==-1) rightfit=j;
	      }
	    }
	    if (rightfit==-1) {
	      printf("  ** Did not find photometric solution band %s ccd %d\n",
		im[i].band,flag_ccd);
	      exit(0);
	    }
	    zp[i].imageid=im[i].imageid;
	    zp[i].mag_zero=-1.0*fit[rightfit].a+2.5*log10(im[i].exptime)+
	      fit[rightfit].k*im[i].airmass;
	    zp[i].sigma_zp=sqrt(Squ(fit[rightfit].aerr)+Squ(fit[rightfit].kerr));
	    zp[i].b=fit[rightfit].b;
	    zp[i].berr=fit[rightfit].berr;
	    /*zp[i].fitid=im[i].fitid;*/
	    zp[i].sourceid=ORIGIN_PSMFIT;
	    line[strlen(line)-1]=0; 
	    if (!flag_quiet && i%20==0) printf("  ** %3d- %s   %.4f %.4f\n",
	      i,line,zp[i].mag_zero,zp[i].sigma_zp);
	  }
	  i++;
	}
	pclose(pip);

	/* now insert the records into the zeropoint table */
	/* first query to find the current value of zp_n */
	out=fopen("update_zeropoint.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(out,"select max(zp_n) from zeropoint;\n");
	fprintf(out,"exit;\n");
	fclose(out);
	pip=popen(sqlcall,"r");
	fscanf(pip,"%d",&maxzp_n);
	pclose(pip);
	if (!flag_quiet) printf("  Current max zeropoint id is %d\n",maxzp_n);
	/* now set up ingestion script */
	out=fopen("update_zeropoint.ingest","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	for (i=0;i<nimages;i++) {
	    fprintf(out,"INSERT INTO zeropoint (ZP_N,IMAGE_N,MAG_ZERO,");
	    fprintf(out,"SIGMA_ZP,B,BERR) \n");
	    fprintf(out,"VALUES ( zeropoint_seq.nextval, %d, %.6f, %.6f,",
	      zp[i].imageid,zp[i].mag_zero,zp[i].sigma_zp);
	    fprintf(out," %.6f, %.6f);\n",zp[i].b,zp[i].berr);
	}
	fprintf(out,"exit;");
	fclose(out);
	/* ingest zeropoints into zeropoint table */
	if (!flag_quiet) printf("  Ingesting %d new zeropoints\n",nimages);
	system("${ORACLE_HOME}/bin/sqlplus -S pipeline/dc01user@charon/DES < update_zeropoint.ingest");
	/* and read back the zeropoint ID's */
	if (!flag_quiet) printf("  Reading back zeropoint id's");
	out=fopen("update_zeropoint.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(out,"select zp_n,image_n from zeropoint\n");
	fprintf(out,"where zp_n>%d;\n",maxzp_n);
	fprintf(out,"exit;\n");
	fclose(out);
	pip=popen(sqlcall,"r");
	i=0;
	while (fscanf(pip,"%d %d",&(zp[i].zp_n),&imageid)!=EOF) {
	  if (imageid!=zp[i].imageid) {
	    printf("ImageID mismatch in zeropoint table\n");
	    exit(0);
	  }
	  i++;  
	  if (i>nimages) {printf("Too many zeropoints returned\n");exit(0);}
	}
	pclose(pip);
	if (!flag_quiet) printf(" for %d new zeropoints\n",i);
	if (i<nimages) {
	  printf("  Too few zeropoints returned!  %d vs %d\n",i,nimages);
	  exit(0);
	}

	/* ************************************************** */
	/* ************** QUERY for Objects ***************** */
	/* ************************************************** */

	nobjects=0;
	out=fopen("update_zeropoint.update","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	for (i=0;i<nimages;i++) {

	  /* add object select statement for current image */
	  out2=fopen("update_zeropoint.sql","w");
	  fprintf(out2,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	  fprintf(out2,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	  fprintf(out2,"SELECT object_id,imageid,zeropoint FROM objects\n");
	  fprintf(out2,"WHERE IMAGEID=%d;\n",zp[i].imageid);
	  fprintf(out2,"exit;\n");
	  fclose(out2);

	  /* trigger object queries here */
	  pip=popen(sqlcall,"r");
	  while (fgets(line,1000,pip)!=NULL) {
	    sscanf(line,"%d %d %f",&objectid,&imageid,&zeropoint);
	    zeropoint-=zp[i].mag_zero;
	    if (zeropoint<=0.0) sprintf(sgn,"+");
	    else sprintf(sgn,"-");
	    zeropoint=fabs(zeropoint);
	    /* write update statements for this object */
	    fprintf(out,"UPDATE objects\n SET\n");
	    fprintf(out,"  ZEROPOINTID=%d,\n",zp[i].zp_n);
	    fprintf(out,"  ZEROPOINT=ZEROPOINT%s%.4f,\n",sgn,zeropoint);
	    fprintf(out,"  ERRZEROPOINT=%.4f,\n",zp[i].sigma_zp);
	    fprintf(out,"  MAG_AUTO=MAG_AUTO%s%.4f,\n",sgn,zeropoint);
	    fprintf(out,"  MAG_APER_1=MAG_APER_1%s%.4f,\n",sgn,zeropoint);
	    fprintf(out,"  MAG_APER_2=MAG_APER_2%s%.4f,\n",sgn,zeropoint);
	    fprintf(out,"  MAG_APER_3=MAG_APER_3%s%.4f,\n",sgn,zeropoint);
	    fprintf(out,"  MAG_APER_4=MAG_APER_4%s%.4f,\n",sgn,zeropoint);
	    fprintf(out,"  MAG_APER_5=MAG_APER_5%s%.4f,\n",sgn,zeropoint);
	    fprintf(out,"  MAG_APER_6=MAG_APER_6%s%.4f\n",sgn,zeropoint);
	    fprintf(out,"WHERE OBJECT_ID=%d;\n",objectid);
	    nobjects++;allobjects++;
	  }

	  /* handle the object updates here */
	  if (nobjects>50000 || i==nimages-1) { 
	    /* close and update using current update file */
	    fprintf(out,"exit;\n");
	    fclose(out);
	    if (!flag_quiet) {
	      printf("  Updating %5d more rows in OBJECTS table (%d total)\n",
	        nobjects,allobjects);
	      fflush(stdout);
	    }
	    nobjects=0;
	    system("${ORACLE_HOME}/bin/sqlplus -S pipeline/dc01user@charon/DES < update_zeropoint.update");
	    if (i<nimages-1) { /* prepare for next one */
	      /* set up new OBJECTS update files */
	      out=fopen("update_zeropoint.update","w");
	      fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	      fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	    }
	  }
	}
	/* free vectors */
	free(im);free(zp);free(fit);
	if (!flag_quiet) 
	  printf("  Calculated %d image zeropoints and updated %d object records\n",
	    nimages,allobjects);
}

