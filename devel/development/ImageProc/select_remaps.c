/* this program meant to create a mapping between all selected coadd tiles
/* and the images that overlap these tiles */
/* check the wcsoffset table... */

#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
        char	command[200],sqlcall[500],nitestring[100],filter[5][10],archroot[300],
		line[1000],sgn[10],project[100],tilestring[100],band[100],
		inputfile[1000],outputfile[1000],tsize[10],imagetype[500],
	        nitelist[1000],bandlist[64],tem[100],**nites,**bands,
	        **runid,runidlist[1024],dblogin[500];
	int	i,j,k,kin,flag,flag_quiet=0,flag_remap=0,flag_type=0,ntiles,nimages,noffsets,
	        nobjects,imageid,objectid,maxzp_n,allobjects=0,ncomma,s,nnite,nband,len,
     	        flag_ccd=0,flag_nite=0,flag_coadd=0,rightfit,count=0,nrunid,flag_runid=0;
	float	zeropoint,maxtilesize_ra=0.0,maxtilesize_dec=0.0;
	double 	deltadec,deltara,scale,sizera,sizedec;
	double  minra,maxra,mindec,maxdec,ra,dec;
	double  ra_hi,ra_lo,dec_hi,dec_lo;
	db_tiles *tile;
	db_files *im;
	db_wcsoffset *offset;
	FILE	*out,*pip;
	void	select_dblogin();

	if (argc<2) {
	  printf("select_remaps <project> <tile string> <band#1,band#2,...> <archive-root>\n");
	  printf("  -nite <nite#1,nite#2,...>\n");
	  printf("  -runid <runid#1,runid#2,...> \n");
          printf("  -type <type> (type = object, reduced or remap; default is object)\n");
	  printf("  -coadd\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	sprintf(project,"%s",argv[1]);
	sprintf(tilestring,"%s",argv[2]);
	sprintf(bandlist,"%s",argv[3]);
	sprintf(archroot,"%s",argv[4]);
	
	/* find out how many bands */
	ncomma=0;
	len=strlen(bandlist);
	for (j=len;j>0;j--) {
	  if (!strncmp(&(bandlist[j]),",",1)) { 
	    ncomma++;
	    bandlist[j]=32;
	  }
	}
	nband=ncomma+1;	
	bands=(char **)calloc(nband,sizeof(char *));
	for(j=0;j<nband;j++) bands[j]=(char *)calloc(8,sizeof(char ));
	s=0;
	for(k=0;k<nband;k++) {
	  sscanf(bandlist+s,"%s%[\0]",tem);
	  sprintf(bands[k],"%s",tem);
	  len=strlen(tem);
	  s+=len+1;
	}
	
	/* default */
	sprintf(imagetype,"%s","remap");
	flag_remap=1;
	
	for (i=5;i<argc;i++) {
	  if (!strcmp(argv[i],"-nite")) { 
	    flag_nite=1;
	    i++;
	    if (i>=argc) {
	      printf("  **Must include nite string with -nite directive\n");
	      exit(0);
	    }
	    sprintf(nitelist,"%s",argv[i]);
	    ncomma=0;
	    len=strlen(nitelist);
	    for (j=len;j>0;j--) {
	      if (!strncmp(&(nitelist[j]),",",1)) { 
		ncomma++;
		nitelist[j]=32;
	      }
	    }
	    nnite=ncomma+1;	
	    nites=(char **)calloc(nnite,sizeof(char *));
	    for(j=0;j<nnite;j++) nites[j]=(char *)calloc(64,sizeof(char ));
	    s=0;
	    for(k=0;k<nnite;k++) {
	      sscanf(nitelist+s,"%s%[\0]",tem);
	      sprintf(nites[k],"%s",tem);
	      len=strlen(tem);
	      s+=len+1;
	    }
	  }
	  if (!strcmp(argv[i],"-runid")) {
	    sprintf(runidlist,"%s",argv[i+1]);
	    flag_runid=1;
	    ncomma=0;
	    len=strlen(runidlist);
	    for (j=len;j>0;j--) {
	      if (!strncmp(&(runidlist[j]),",",1)) { 
		ncomma++;
		runidlist[j]=32;
	      }
	    }
	    nrunid=ncomma+1;
	    runid=(char **)calloc(nrunid,sizeof(char *));
	    for(j=0;j<nrunid;j++) runid[j]=(char *)calloc(1024,sizeof(char ));
	    s=0;
	    for(k=0;k<nrunid;k++) {
	      sscanf(runidlist+s,"%s%[\0]",tem);
	      sprintf(runid[k],"%s",tem);
	      len=strlen(tem);
	      s+=len+1;
	    }
	  }
	  if (!strcmp(argv[i],"-coadd")) flag_coadd=1;
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-type")) {
	    flag_type=1;
	    sprintf(imagetype,"%s",argv[i+1]);
	    /* check if input imagetype is acceptable or not */
	    if(!strcmp(imagetype,"REMAP")) sprintf(imagetype,"remap");
	    if(!strcmp(imagetype,"OBJECT")) sprintf(imagetype,"object");
	    if(!strcmp(imagetype,"REDUCED")) sprintf(imagetype,"reduced");
	    if(strcmp(imagetype,"object") && strcmp(imagetype,"reduced") && strcmp(imagetype,"remap")) {
	      printf("  **select_remaps error: wrong input of <type>\n");
	      exit(0);
	    }
	    
	    if(!strcmp(imagetype,"remap")) flag_remap=1;
	  }
	}

	if(!flag_quiet)
	  printf("  **selecting images with imagetype = %s from database\n", imagetype);
	if(flag_nite && !flag_quiet)
	  printf("  Restricting images to nites like '%s\%'\n",nitelist);
	if(flag_runid && !flag_quiet)
	  printf("  Restricting images to runid like '%s\%'\n",runidlist);
	

	/* access dblogin */
	select_dblogin(dblogin);

	/* set up generic db call */
	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < select_remaps.sql",dblogin);

	/* ************************************************** */
	/* ************* QUERY for Solutions **************** */
	/* ************************************************** */

	out=fopen("select_remaps.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	/* first find out how many solutions to expect */
	fprintf(out,"SELECT count(*)\n");
	fprintf(out,"FROM coaddtile\n");
	fprintf(out,"WHERE PROJECT='%s' and TILENAME like '%%%s%%';\n",
	  project,tilestring);
	fprintf(out,"SELECT *\n");
	fprintf(out,"FROM coaddtile\n");
	fprintf(out,"WHERE PROJECT='%s' and TILENAME like '%%%s%%';\n",
	  project,tilestring);
	fprintf(out,"exit;\n");
	fclose(out);
	
	i=-1;
	pip=popen(sqlcall,"r");
	while (fgets(line,1000,pip)!=NULL) {
	  if (i==-1) {
	    sscanf(line,"%d",&ntiles);
	    if (!flag_quiet) printf("  Selected %d tiles from db\n",ntiles);
	    if (ntiles==0) {
	    	printf("  ** Query must select at least one tile\n");
	    	exit(0);
	    }
	    tile=(db_tiles *)calloc(ntiles,sizeof(db_tiles));
	  }
	  else {
	  if (!flag_quiet) printf("  ** %s",line);
	  /* read information into local variables */
	  sscanf(line,"%d %s %s %lf %lf %f %f %d %d",
	    &(tile[i].coaddtile_id),tile[i].project,tile[i].tilename,
	    &(tile[i].ra), 
	    &(tile[i].dec), &(tile[i].equinox),
	    &(tile[i].pixelsize), &(tile[i].npix_ra), &(tile[i].npix_dec));

	  if (tile[i].pixelsize*tile[i].npix_ra>maxtilesize_ra) 
	    maxtilesize_ra=tile[i].pixelsize*tile[i].npix_ra;
	  if (tile[i].pixelsize*tile[i].npix_dec>maxtilesize_dec) 
	    maxtilesize_dec=tile[i].pixelsize*tile[i].npix_dec;
	  }
	  i++;
	}
	pclose(pip);
	
	/* Now must seek images from FILES table that lie close to */
	/* any tile in this list */

	/* ************************************************** */
	/* ************** QUERY for Images ****************** */
	/* ************************************************** */
	
	/* set up search region within 1 degree plus half the tilesize */
	deltara=1.0+maxtilesize_ra/2.0/3600.0;
	deltadec=1.0+maxtilesize_dec/2.0/3600.0;
	
	out=fopen("select_remaps.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	/* first find out how many solutions to expect */
	fprintf(out,"SELECT count(*)\n");
	fprintf(out,"FROM files\n");
	fprintf(out,"WHERE lower(IMAGETYPE)='%s' AND ",imagetype);
	fprintf(out,"(BAND='%s' ",bands[0]);
	if(nband>1) 
	  for(k=1;k<nband;k++) fprintf(out,"or BAND='%s' ",bands[k]);
	fprintf(out,") AND ");
	if (flag_nite) {
	  fprintf(out,"(NITE='%s' ",nites[0]);
	  if(nnite>1) 
	    for(k=1;k<nnite;k++) fprintf(out,"or NITE='%s' ",nites[k]);
	  fprintf(out,") AND ");
	}
	if (flag_runid) {
	  fprintf(out,"(RUNIDDESC like '%%%s%%' ",runid[0]);
	  if(nrunid>1) 
	    for(k=1;k<nrunid;k++) fprintf(out,"or RUNIDDESC like '%%%s%%' ",runid[k]);
	  fprintf(out,") AND ");
	}
	if (flag_remap) for (i=0;i<ntiles;i++) fprintf(out," TILENAME='%s' AND ",tile[i].tilename);
	fprintf(out," (\n");
	for (i=0;i<ntiles;i++) {
	  scale=cos(tile[i].dec*M_PI/180.0);
	  if (scale>1.0e-5) 
	  fprintf(out,"(CRVAL1>%11.7f and CRVAL1<%11.7f AND CRVAL2>%11.7f AND CRVAL2<%11.7f)",
	    tile[i].ra-deltara/scale,tile[i].ra+deltara/scale,
	    tile[i].dec-deltadec,tile[i].dec+deltadec);
	  else fprintf(out,"(CRVAL2>%11.7f AND CRVAL2<%11.7f)",
	    tile[i].dec-deltadec,tile[i].dec+deltadec);
	  if (i<ntiles-1) fprintf(out," OR\n");
	  else fprintf(out," );\n");
	}
	
	/* now ask for line to be returned */
	fprintf(out,"SELECT IMAGEID,CRVAL1,CRVAL2,RADECEQ,CCD_NUMBER,DETECTOR,BAND,IMAGENAME,NITE,RUNIDDESC\n");
	fprintf(out,"FROM files \n");
	fprintf(out,"WHERE lower(IMAGETYPE)='%s' AND ",imagetype);
	fprintf(out,"(BAND='%s' ",bands[0]);
	if(nband>1) 
	  for(k=1;k<nband;k++) fprintf(out,"or BAND='%s' ",bands[k]);
	fprintf(out,") AND ");
	if (flag_nite) {
	  fprintf(out,"(NITE='%s' ",nites[0]);
	  if(nnite>1) 
	    for(k=1;k<nnite;k++) fprintf(out,"or NITE='%s' ",nites[k]);
	  fprintf(out,") AND ");
	}
	if (flag_runid) {
	  fprintf(out,"(RUNIDDESC like '%%%s%%' ",runid[0]);
	  if(nrunid>1) 
	    for(k=1;k<nrunid;k++) fprintf(out,"or RUNIDDESC like '%%%s%%' ",runid[k]);
	  fprintf(out,") AND ");
	}
	if (flag_remap) for (i=0;i<ntiles;i++) fprintf(out," TILENAME='%s' AND ",tile[i].tilename);
	fprintf(out," ( \n");
	for (i=0;i<ntiles;i++) {
	  scale=cos(tile[i].dec*M_PI/180.0);
	  if (scale>1.0e-5) 
	  fprintf(out,"(CRVAL1>%11.7f and CRVAL1<%11.7f AND CRVAL2>%11.7f AND CRVAL2<%11.7f)",
	    tile[i].ra-deltara/scale,tile[i].ra+deltara/scale,
	    tile[i].dec-deltadec,tile[i].dec+deltadec);
	  else fprintf(out,"(CRVAL2>%11.7f AND CRVAL2<%11.7f)",
	    tile[i].dec-deltadec,tile[i].dec+deltadec);
	  if (i<ntiles-1) fprintf(out," OR\n");
	  else fprintf(out," );\n");
	}
	fprintf(out,"exit;\n");
	fclose(out);
	
	i=-1;
	pip=popen(sqlcall,"r");
	while (fgets(line,1000,pip)!=NULL) {
	  if (i==-1) {
	    sscanf(line,"%d",&nimages);
	    if (!flag_quiet) printf("  Initially selecting %d images from db\n",
	      nimages);
	    im=(db_files *)calloc(nimages,sizeof(db_files));
	  }
	  else {
	    sscanf(line,"%d %lf %lf %f %d %s %s %s %s %s",&(im[i].imageid),
	      &(im[i].ra),&(im[i].dec),&(im[i].radeceq),&(im[i].ccd_number),
	      im[i].detector,im[i].band,im[i].imagename,im[i].nite,
	      im[i].runiddesc);
	    if(strlen(im[i].runiddesc) == 0)
	      sprintf(im[i].runiddesc,"NA");
	  }
	  i++;
	}
	pclose(pip);

	/* now cycle through images to determine all the */
	/* telescope/instrument/chipid combinations */
	out=fopen("select_remaps.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(out,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	/* first find out how many solutions to expect */
	fprintf(out,"SELECT count(*) from WCSOFFSET WHERE \n");
	for (i=0;i<nimages;i++) {
	  fprintf(out,"( CHIPID=%d and DETECTOR='%s' )",
		im[i].ccd_number,im[i].detector);
	  if (i==nimages-1) fprintf(out,";\n");
	  else fprintf(out," OR\n");
	}

	fprintf(out,"SELECT * from WCSOFFSET WHERE \n");
	for (i=0;i<nimages;i++) {
	  fprintf(out,"( CHIPID=%d and DETECTOR='%s' )",
		im[i].ccd_number,im[i].detector);
	  if (i==nimages-1) fprintf(out,";\n");
	  else fprintf(out," OR\n");
	}
	fprintf(out,"exit;\n");	
	fclose(out);	
	/* read in offsets */
	i=-1;
	pip=popen(sqlcall,"r");
	while (fgets(line,1000,pip)!=NULL) {
	  if (i==-1) {
	    sscanf(line,"%d",&noffsets);
	    if (!flag_quiet) printf("  Selecting %d chip offsets from db\n",
	      noffsets);
	    offset=(db_wcsoffset *)calloc(noffsets,sizeof(db_wcsoffset));
	  }
	  else 
	    sscanf(line,"%d %f %f %f %f %s %s %s",&(offset[i].chipid),
		   &(offset[i].raoffset),&(offset[i].decoffset),
		   &(offset[i].rahwidth),&(offset[i].dechwidth),
		   offset[i].telescope,tsize,offset[i].detector);
	  i++;
	}
	pclose(pip);

	out=fopen("select_remaps.out","w");
	/* cycle through tiles */
	for (j=0;j<ntiles;j++) { 
	  scale   =cos(tile[j].dec*M_PI/180.0);
	  deltara =tile[j].npix_ra*tile[j].pixelsize/2.0/3600.0/scale;
	  deltadec=tile[j].npix_dec*tile[j].pixelsize/2.0/3600.0;
	  minra=tile[j].ra-deltara;if (minra<0.0)   minra+=360.0;
	  maxra=tile[j].ra+deltara;if (maxra>360.0) maxra-=360.0;
	  mindec=tile[j].dec-deltadec;
	  maxdec=tile[j].dec+deltadec;
	  sizera=20*tile[j].pixelsize/3600.0/scale;
	  sizedec=20*tile[j].pixelsize/3600.0;
	  
	  /* find matching images */
	  for (i=0;i<nimages;i++) {
	    flag=0;
	    scale=cos(im[i].dec*M_PI/180.0);

	    for (k=0;k<noffsets;k++) {
	      if (im[i].ccd_number==offset[k].chipid && 
		  !strcmp(im[i].detector,offset[k].detector)) break;
	    }
	    
	    /* check upper left corner of chip */
	    ra =im[i].ra +offset[k].raoffset/scale +offset[k].rahwidth/scale;
	    dec=im[i].dec+(offset[k].decoffset+offset[k].dechwidth);
	    if (ra>minra && ra<maxra && dec>mindec && dec<maxdec) flag=1;
	    /* check lower left corner of chip */
	    dec=im[i].dec+offset[k].decoffset-offset[k].dechwidth;
	    if (ra>minra && ra<maxra && dec>mindec && dec<maxdec) flag=1;
	    /* check lower right corner of chip */
	    ra =im[i].ra +offset[k].raoffset/scale -offset[k].rahwidth/scale;
	    if (ra>minra && ra<maxra && dec>mindec && dec<maxdec) flag=1;
	    /* check upper right corner of chip */
	    dec=im[i].dec+offset[k].decoffset+offset[k].dechwidth;
	    if (ra>minra && ra<maxra && dec>mindec && dec<maxdec) flag=1;
	      
	    /* make sure the image is more than 20 pixels from the edge of the tile */
	    //if(fabs(ra-minra)<sizera || fabs(ra-maxra)<sizera || fabs(dec-mindec)<sizedec || fabs(dec-maxdec)<sizedec)
	    //flag=0;

	    /* get ranges of ra and dec */
	    ra_hi =im[i].ra +offset[k].raoffset/scale +offset[k].rahwidth/scale;
	    ra_lo =im[i].ra +offset[k].raoffset/scale -offset[k].rahwidth/scale;
	    dec_hi=im[i].dec+offset[k].decoffset+offset[k].dechwidth;
	    dec_lo=im[i].dec+offset[k].decoffset-offset[k].dechwidth;
	    
	    
	    /* print results to information */
	    if (flag) {
	      sprintf(inputfile,"%s/%s/data/%s/%s/%s/%s_%02d.fits",
		      archroot,im[i].runiddesc,im[i].nite,im[i].band,im[i].imagename,
		      im[i].imagename,im[i].ccd_number);
	      sprintf(outputfile,"%s/%s/data/%s/%s/%s/%s_%02d.%s.fits",
		      archroot,im[i].runiddesc,im[i].nite,im[i].band,im[i].imagename,
		      im[i].imagename,im[i].ccd_number,tile[j].tilename);
	      fprintf(out,"%s %s %12.7f %12.7f %10.7f %5d %5d %s %s\n",
		      tile[j].project,tile[j].tilename,tile[j].ra,tile[j].dec,
		      tile[j].pixelsize,tile[j].npix_ra,tile[j].npix_dec,
		      inputfile,outputfile);
	      if (!flag_quiet)  
		printf("%s %s %12.7f %12.7f %10.7f %5d %5d %s %s\n",
		       tile[j].project,tile[j].tilename,tile[j].ra,tile[j].dec,
		       tile[j].pixelsize,tile[j].npix_ra,tile[j].npix_dec,
		       inputfile,outputfile);
	      
	      if(flag_coadd) {
		if(flag_remap)
		  printf("%d %s %s %s %s %s/%s_%02d.%s.fits %d %d %12.7f %12.7f %10.7f %5d %5d %2.7f %2.7f %2.7f %2.7f\n", 
			 count+1,tile[j].tilename,
			 im[i].runiddesc,im[i].nite,im[i].band,
			 im[i].imagename,im[i].imagename,im[i].ccd_number,tile[j].tilename,im[i].ccd_number,im[i].imageid,
			 tile[j].ra,tile[j].dec,tile[j].pixelsize,tile[j].npix_ra,tile[j].npix_dec,
			 ra_lo,ra_hi,dec_lo,dec_hi);
		else
		  printf("%d %s %s %s %s %s/%s_%02d.fits %d %d %12.7f %12.7f %10.7f %5d %5d %2.7f %2.7f %2.7f %2.7f\n", 
			 count+1,tile[j].tilename,
			 im[i].runiddesc,im[i].nite,im[i].band,
			 im[i].imagename,im[i].imagename,im[i].ccd_number,im[i].ccd_number,im[i].imageid,
			 tile[j].ra,tile[j].dec,tile[j].pixelsize,tile[j].npix_ra,tile[j].npix_dec,
			 ra_lo,ra_hi,dec_lo,dec_hi);
	      }
	      count++;
	    }
	    
	  }
	}
	  
	fclose(out);
	system ("rm select_remaps.out");
	system ("rm select_remaps.sql");
	
	/* free vectors */
	cfree(im);cfree(tile);cfree(bands);
	if(flag_nite) cfree(nites);
	if(flag_runid) cfree(runid);

	if (!flag_quiet) 
	  printf("  Selected %d images for remapping within %d coadd tiles\n",
		 count,ntiles);
}

