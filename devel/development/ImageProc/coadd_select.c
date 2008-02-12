#include "imageproc.h" 

#define GBAND 1
#define RBAND 2
#define IBAND 3
#define ZBAND 4

main(argc,argv)
	int argc;
	char *argv[];
{
	char	objecttable[100],sqlcall[200],tmp[10],filename[500],
		band[5][10]={"","g","r","i","z"},oband[100],nite[100],
		objquy[10000],command[100],runid[100],
		tilename[100],outputroot[500],
		inputline[5000],**objline,dblogin[500];
	float	ramin,ramax,decmin,decmax,temp,scale,
		omag_g,omagerr_g,omag_r,omagerr_r,omag_i,
		omagerr_i,omag_z,omagerr_z,oclass,calcdistance(),
	        truemag,mindist,dist,fwhm,
		*sra,*tg,*tr,*ti,*tz,*tclass;
	double	tra,tdec,*ora,*odec;
	int	i,j,flag_quiet=0,flag_ccd=0,flag_band=0,*omatch,
		nomatches,matches,nobjects,locmatch,loclow,lochi,
		flag_nite=0,flag_noquery=0,flag_runid=0;
	FILE	*pip,*out,*outno,*inp;
	void	select_dblogin();

	if (argc<2) {
	  printf("%s <TileID> \n",argv[0]);
	  printf("  -runid <runid>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	sprintf(tilename,"%s",argv[1]);
	sprintf(outputroot,"coadd_select_%s",tilename);

	/*  process the rest of the command line */
	for (i=2;i<argc;i++) {
	  if (!strcmp("-quiet",argv[i])) flag_quiet=1;
	  if (!strcmp("-runid",argv[i])) {
	    sscanf(argv[i+1],"%s",runid);
	    flag_runid=1;
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
	sprintf(objquy,"SELECT coadd_objects.alpha_j2000,coadd_objects.delta_j2000,\n");
	sprintf(objquy,"%scoadd_objects.mag_auto_g,coadd_objects.magerr_auto_g,coadd_objects.flags_g,coadd_objects.class_star_g,\n",objquy);
	sprintf(objquy,"%scoadd_objects.mag_auto_r,coadd_objects.magerr_auto_r,coadd_objects.flags_r,coadd_objects.class_star_r,\n",objquy);
	sprintf(objquy,"%scoadd_objects.mag_auto_i,coadd_objects.magerr_auto_i,coadd_objects.flags_i,coadd_objects.class_star_i,\n",objquy);
	sprintf(objquy,"%scoadd_objects.mag_auto_z,coadd_objects.magerr_auto_z,coadd_objects.flags_z,coadd_objects.class_star_z \n",objquy);
	sprintf(objquy,"%sFROM %s,files\n",objquy,objecttable);
	sprintf(objquy,"%sWHERE files.imageid=coadd_objects.imageid_g\n",objquy);
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
	else if (!flag_quiet) printf("  Selected %d objects from %s\n",
	  nobjects,objecttable);
	
}

#undef GBAND
#undef RBAND
#undef IBAND
#undef ZBAND
