#include "imageproc.h"

#define convert M_PI/180.0
#define DECAM 62
#define MOSAIC 8
#define BUFFERSIZE 6.0/60.0 // in 6 arcmin 
#define TELESCOPE "Blanco 4m"

main(argc,argv)
	int argc;
	char *argv[];
{
  	char 	imagename[1500],fullname[1500],line[1000],usnob_cat[500],
		nite[100],
                telescopename[500],detector[500],sqlcall[1000],temp[1000],
		imag_prev[1500],dblogin[500];
  	int 	ccdtotal=DECAM, *ccd, ccdnum, i, j, flag_quiet=0, 
		flag_max,flag_min,ct;
  	float 	*raoff, *decoff, *rahw, *dechw;
  	double 	ra, dec, ralow, rahigh, declow, dechigh;
	double 	ra_ori, dec_ori, ra_max, ra_min, dec_max, dec_min;

  	FILE 	*fin, *fout, *pip;
	void	select_dblogin();

  	if(argc < 2) { 
	  printf("%s <image list> -detector <detector (DECam or Mosaic2)> -catalog <output usnob.cat> -nite <nite> -quiet\n",argv[0]); 
	  exit(0); 
	}

	for(i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-catalog")) sprintf(usnob_cat,"%s",argv[i+1]);
	  if (!strcmp(argv[i],"-nite")) sprintf(nite,"%s",argv[i+1]);
	  if (!strcmp(argv[i],"-detector")) {
	    sprintf(detector,"%s",argv[i+1]);
	    if (!strcmp(detector,"DECam")) ccdtotal=DECAM;
	    else if(!strcmp(detector,"Mosaic2")) ccdtotal=MOSAIC;
	    else {
	      printf(" ** %s error: wrong input for -detector, abort\n",argv[0]);
	      exit(0);
	    }
	  }
	}

	/* hardwired at the moment */
	sprintf(telescopename, "%s", TELESCOPE);

	/* memory allocatio for the wcsoffset info */
	ccd=(int *)calloc(ccdtotal+1,sizeof(int));
	raoff=(float *)calloc(ccdtotal+1,sizeof(float));
	decoff=(float *)calloc(ccdtotal+1,sizeof(float));
	rahw=(float *)calloc(ccdtotal+1,sizeof(float));
	dechw=(float *)calloc(ccdtotal+1,sizeof(float));

	/* get db login information */
	select_dblogin(dblogin);

	/* input wcsoffset info */
	fout=fopen("wcsoffset.sql", "w");
	fprintf(fout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(fout,"SELECT chipid,raoffset,decoffset,rahwidth,dechwidth ");
	fprintf(fout,"FROM wcsoffset WHERE ");
	fprintf(fout,"TELESCOPE='%s' and DETECTOR='%s' ORDER BY chipid;\n",
		telescopename,detector);
	fprintf(fout,"exit;\n");
	fclose(fout);

	sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < wcsoffset.sql",
	  dblogin);

	pip=popen(sqlcall,"r"); i=1;
	while (fgets(line,1000,pip)!=NULL) {
	  sscanf(line,"%d %f %f %f %f",&ccd[i],&raoff[i],&decoff[i],&rahw[i],&dechw[i]);
	  i++;
	}
	pclose(pip);

	/* get the imagename and ccd from the list */
	fin=fopen(argv[1],"r");
	fout=fopen("imageradec.sql", "w");
	fprintf(fout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(fout,"SELECT distinct ra,dec ");
	//fprintf(fout,"FROM files WHERE upper(imagetype)='OBJECT' AND NITE='%s'\n",nite);
        fprintf(fout,"FROM files WHERE (upper(imagetype)='OBJECT' OR imagetype='reduced') AND NITE='%s'\n",nite);
        j=0;
	while (fscanf(fin,"%s",fullname)!=EOF) {
	  
	  if (strncmp(&(fullname[strlen(fullname)-5]),".fits",5)) {
	    printf("  ** File must contain list of FITS images **\n");
	    exit(0);
	  }
	  
	  /* either ../imagename/imagename_ccd.fits or ../imagename/imagename_ccd_im.fits */
	  if(!strncmp(&(fullname[strlen(fullname)-8]),"_im.fits",8)) 
	    fullname[strlen(fullname)-8]=0;
	  else 
	    fullname[strlen(fullname)-5]=0;
	  
	  for (i=strlen(fullname);i--;) {
	    if (!strncmp(&(fullname[i]),"_",1)) { 
	      fullname[i]=32;
	      break;
	    }
	  }
	  for (i=strlen(fullname);i--;) {
	    if (!strncmp(&(fullname[i]),"/",1)) { 
	      fullname[i]=32;
	      break;
	    }
	  }	  
	  sscanf(fullname,"%s %s %d",temp,imagename,&ccdnum);

	  if(!j) {
	    fprintf(fout,"AND (imagename='%s'\n",imagename);
	    //fprintf(fout,"(imagename='%s' AND ccd_number='%d' AND upper(imagetype)='OBJECT' AND NITE='%s')\n",imagename,ccdnum,nite);
	    sprintf(imag_prev,"%s",imagename);
	  }
	  else {
	    if(strcmp(imag_prev,imagename))
	      fprintf(fout,"OR imagename='%s'\n",imagename);
	    
	    sprintf(imag_prev,"%s",imagename);
	    //fprintf(fout,"OR (imagename='%s' AND ccd_number='%d' AND upper(imagetype)='OBJECT' AND NITE='%s')\n",imagename,ccdnum,nite);
	  }
	  j++;
	}
	fprintf(fout,");\n");
	fprintf(fout,"exit;\n");
	fclose(fin); fclose(fout);

	sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < imageradec.sql",
	  dblogin);


	/* now construct the USNOB call */
	fout=fopen("usnob.sql", "w");

	/* find the max/min of ra & dec */	
	ct=0;
	pip=popen(sqlcall,"r"); 
	while (fgets(line,1000,pip)!=NULL) {
	  sscanf(line,"%lg %lg",&ra,&dec);
	  
	  if (!ct) fprintf(fout,"create table usnob_temp_%s as \n",nite);
	  else fprintf(fout,"insert into usnob_temp_%s\n",nite);

	  ra_ori=ra; dec_ori=dec;

	  ra_min=360; ra_max=0.0;
	  dec_min=90; dec_max=-90;
	  for(i=1;i<=ccdtotal;i++) {
	    flag_min=flag_max=0;

	    ra = ra_ori + raoff[i]/cos(dec*convert);
	    ralow = ra - rahw[i]/cos(dec*convert) - BUFFERSIZE;
	    rahigh = ra + rahw[i]/cos(dec*convert) + BUFFERSIZE;
	    

	    dec = dec_ori + decoff[i];
	    declow = dec - dechw[i] - BUFFERSIZE;
	    dechigh = dec + dechw[i] + BUFFERSIZE;
      	  
	    if(ralow < ra_min) ra_min=ralow;
	    if(rahigh > ra_max) ra_max=rahigh;
	    
	    if(declow < dec_min) dec_min=declow;
	    if(dechigh > dec_max) dec_max=dechigh;
	    
	  }

	  if(ra_max>360.0) { ra_max-=360.0; flag_max=1; }
	  if(ra_min<0.0) { ra_min+=360.0; flag_min=1; }

	  fprintf(fout,"SELECT distinct ra,dec,sra,sde,r2 ");
	  fprintf(fout,"FROM USNOB_CAT1 WHERE ");
	  fprintf(fout," (r2 BETWEEN 10.0 AND 20.5) AND ");
	  if(flag_max) {
	    fprintf(fout,"((ra BETWEEN %2.6f AND 360.0 AND dec BETWEEN %2.6f AND %2.6f) or ", 
		    ra_min, dec_min, dec_max);
	    fprintf(fout,"(ra BETWEEN 0.0 AND %2.6f AND dec BETWEEN %2.6f AND %2.6f));\n", 
		    ra_max, dec_min, dec_max);
	  }
	  else if(flag_min) {
	    fprintf(fout,"((ra BETWEEN 0.0 AND %2.6f AND dec BETWEEN %2.6f AND %2.6f) or ", 
		    ra_max, dec_min, dec_max);
	    fprintf(fout,"(ra BETWEEN %2.6f AND 360.0 AND dec BETWEEN %2.6f AND %2.6f));\n", 
		    ra_min, dec_min, dec_max);
	  }
	  else  
	    fprintf(fout,"(ra BETWEEN %2.6f AND %2.6f AND dec BETWEEN %2.6f AND %2.6f);\n", 
		    ra_min, ra_max, dec_min, dec_max);
          ct++;
	}
	pclose(pip); 

	fprintf(fout,"\n");
	fprintf(fout,"SET ECHO OFF  NEWP 0 SPA 1 PAGES 0 FEED OFF ");
	fprintf(fout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(fout,"SPOOL %s;\n",usnob_cat);
	fprintf(fout,"SELECT distinct ra,dec,sra,sde,r2 from usnob_temp_%s;\n",
	  nite);
	fprintf(fout,"SPOOL off;\n");
	fprintf(fout,"drop table usnob_temp_%s purge;\n",nite);
	fprintf(fout,"exit;\n");
	fclose(fout);

	sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < usnob.sql > tem.dat",dblogin);
	system(sqlcall);

	system ("/bin/rm wcsoffset.sql imageradec.sql usnob.sql tem.dat");
	
	/* free memory */
	free(ccd); free(raoff); free(decoff); free(rahw); free(dechw);
	
	return(0);
}

#undef convert
#undef DECAM
#undef MOSAIC
#undef BUFFERSIZE
#undef TELESCOPE
