#include "imageproc.h"

/* program like update_zeropoint but updates imageid by imageid rather than 
/* object by object */

#define BAND_ALL	5
#define BAND_g		0
#define BAND_r		1
#define BAND_i		2
#define BAND_z		3

#define ORIGIN_PSMFIT	1
#define ORIGIN_USNOB	2
#define ORIGIN_COADD	3
#define ORIGIN_RESET	4

#define RESET_ZEROPOINT 25.0


main(argc,argv)
	int argc;
	char *argv[];
{
	char	command[500],sqlcall[500],nite[50],filter[5][10],
		line[1000],sgn[10],timestamp[100],runid[100],dblogin[500];
	int	i,j,flag_band=BAND_ALL,flag_quiet=0,nimages,
		nobjects,imageid,objectid,maxzp_n,allobjects=0,
		flag_ccd=0,ccdnum=0,flag_timestamp=0,flag_runid=0;
	float	zeropoint;
	db_files 	*im;
	db_zeropoint 	*zp;
	FILE	*out,*pip,*out2;
	void	select_dblogin();

	if (argc<2) {
	  printf("reset_zeropoint <nite>\n");
	  printf("  -runid <runid>\n");
	  printf("  -band <g r i z or all(default)>\n");
	  printf("  -ccd <#> (for all ccd, either set -ccd 0 or ignore -ccd option)\n");
	  printf("  -timestamp <dd-MON-YY hh.mm.ss.s AM/PM> \n");
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
	  if (!strcmp(argv[i],"-runid")) {
	    sprintf(runid,"%s",argv[i+1]);
	    flag_runid=1;
	  }
	  if (!strcmp(argv[i],"-ccd")) {
	    sscanf(argv[i+1],"%d",&ccdnum);
	    flag_ccd=1;
	  }
	  if (!strcmp(argv[i],"-timestamp")) {
	    sscanf(argv[i+1],"%s",timestamp);
	    sprintf(timestamp, "%s %s", timestamp, argv[i+2]);
	    sprintf(timestamp, "%s %s", timestamp, argv[i+3]);
	    
	    flag_timestamp=1;
	  }
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}

	sprintf(filter[BAND_g],"g");
	sprintf(filter[BAND_r],"r");
	sprintf(filter[BAND_i],"i");
	sprintf(filter[BAND_z],"z");

	/* grab dblogin */
	select_dblogin(dblogin);

	/* set up generic db call */
	sprintf(sqlcall,"sqlplus -S %s < reset_zeropoint.sql",dblogin);


	/* ************************************************** */
	/* ************** QUERY for Images ****************** */
	/* ************************************************** */

	out=fopen("reset_zeropoint.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	/* first find out how many solutions to expect */
	fprintf(out,"SELECT count(*)\n");
	fprintf(out,"FROM files\n");
	fprintf(out,"WHERE NITE='%s' AND (upper(IMAGETYPE)='REDUCED' or upper(IMAGETYPE)='REMAP') ",nite);
	if (flag_runid) fprintf(out, " and RUNIDDESC like '%%%s%%' ",runid);
	if (flag_ccd && ccdnum>0) fprintf(out,"AND CCD_NUMBER=%d ",flag_ccd);
	if (flag_band!=BAND_ALL) 
	  fprintf(out,"AND BAND='%s';\n",filter[flag_band]);
	else fprintf(out,";\n");

	/* now ask for line to be returned */
	fprintf(out,"SELECT IMAGEID,CCD_NUMBER,BAND,AIRMASS,EXPTIME\n");
	fprintf(out,"FROM files \n");
	fprintf(out,"WHERE NITE='%s' AND (upper(IMAGETYPE)='REDUCED' or upper(IMAGETYPE)='REMAP') ",nite);
	if (flag_runid) fprintf(out, " and RUNIDDESC like '%%%s%%' ",runid);
	if (flag_ccd && ccdnum>0) fprintf(out,"AND CCD_NUMBER=%d ",ccdnum);
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
	    if (!flag_quiet) printf("  Reading %d Reduced Images from files\n",
	      nimages);
	    im=(db_files *)calloc(nimages,sizeof(db_files));
	    zp=(db_zeropoint *)calloc(nimages,sizeof(db_zeropoint));
	    if (!flag_quiet) printf("  ** ###      IMAGEID   CCDID BAND        AIRMASS    EXPTIME     ZP     SIGMA\n");
	  }
	  else {
	    sscanf(line,"%d %d %s %f %f",&(im[i].imageid),&(im[i].ccdid),
	      im[i].band, &(im[i].airmass), &(im[i].exptime));

	    /* reset the image zeropoint */
	    zp[i].imageid=im[i].imageid;
	    zp[i].mag_zero=RESET_ZEROPOINT;
	    zp[i].sigma_zp=0.0;
	    zp[i].b=0.0;
	    zp[i].berr=0.0;
	    zp[i].sourceid=ORIGIN_RESET;
	    line[strlen(line)-1]=0; 
	    if (!flag_quiet && i%1==0) printf("  ** %3d- %s   %.4f %.4f\n",
	      i,line,zp[i].mag_zero,zp[i].sigma_zp);
	  }
	  i++;
	}
	pclose(pip);

	/* now insert the records into the zeropoint table */
	/* first query to find the current value of zp_n */
	out=fopen("reset_zeropoint.sql","w");
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
	out=fopen("reset_zeropoint.ingest","w");
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
	
	sprintf(command,"sqlplus -S %s < reset_zeropoint.ingest",dblogin);
	system(command);

	/* and read back the zeropoint ID's */
	if (!flag_quiet) printf("  Reading back zeropoint id's");
	out=fopen("reset_zeropoint.sql","w");
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
	/* **************** UPDATE Objects ****************** */
	/* ************************************************** */

	nobjects=0;
	/* prepare for update query */
	out=fopen("reset_zeropoint.update","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	for (i=0;i<nimages;i++) {
	  if (i%10==0 && i>0) {  /* trigger queries */
	    fprintf(out,"commit;\n");
	  }
	  /* write update statements for current image */
	  fprintf(out,"UPDATE objects\n SET\n");
	  fprintf(out,"  ZEROPOINTID=%d,\n",zp[i].zp_n);
	  fprintf(out,"  ERRZEROPOINT=%.4f,\n",zp[i].sigma_zp);
	  fprintf(out,"  MAG_AUTO=(MAG_AUTO-(ZEROPOINT-%.4f)),\n",
	    zp[i].mag_zero);
	  fprintf(out,"  MAG_APER_1=(MAG_APER_1-(ZEROPOINT-%.4f)),\n",
	    zp[i].mag_zero);
	  fprintf(out,"  MAG_APER_2=(MAG_APER_2-(ZEROPOINT-%.4f)),\n",
	    zp[i].mag_zero);
	  fprintf(out,"  MAG_APER_3=(MAG_APER_3-(ZEROPOINT-%.4f)),\n",
	    zp[i].mag_zero);
	  fprintf(out,"  MAG_APER_4=(MAG_APER_4-(ZEROPOINT-%.4f)),\n",
	    zp[i].mag_zero);
	  fprintf(out,"  MAG_APER_5=(MAG_APER_5-(ZEROPOINT-%.4f)),\n",
	    zp[i].mag_zero);
	  fprintf(out,"  MAG_APER_6=(MAG_APER_6-(ZEROPOINT-%.4f)),\n",
	    zp[i].mag_zero);
	  fprintf(out,"  ZEROPOINT=%.4f\n",zp[i].mag_zero);
	  fprintf(out,"WHERE IMAGEID=%d;\n",zp[i].imageid);
	}
	/* now make the update call */
	fprintf(out,"exit;\n");
	fclose(out);
	if (!flag_quiet) {
 	  printf("  Updating zeropoints in %d images\n",nimages);
	  fflush(stdout);
	}

	sprintf(command,"sqlplus -S %s < reset_zeropoint.update",dblogin);
	system(command);

	/* free vectors */
	free(im);free(zp);
	if (!flag_quiet) 
	  printf("  Completed zeropoint reset to %.2f for objects in %d images\n",RESET_ZEROPOINT,nimages);

	exit(0);
}
