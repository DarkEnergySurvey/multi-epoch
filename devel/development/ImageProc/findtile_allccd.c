
/* this program meant to create a mapping between all selected coadd tiles
/* and the images that overlap these tiles */

#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
	char	command[200],sqlcall[500],nitestring[100],filter[5][10],
		line[1000],sgn[10],project[100],tilestring[100],band[100],
		inputfile[1000],outputfile[1000],dblogin[500];
	int	i,j,k,flag,flag_quiet=0,ntiles,nimages,noffsets, ccdnummax,
	        nobjects,imageid,objectid,maxzp_n,allobjects=0, ccdnum, match[200],
		flag_ccd=0,flag_nite=0,flag_list=0,flag_out=0,rightfit,count=0, len;
	float	zeropoint,maxtilesize_ra=0.0,maxtilesize_dec=0.0,
	        deltadec,deltara,scale,minra,maxra,mindec,maxdec,ra,dec;
	float   ra_img, dec_img, ra_offset, dec_offset, ra_width, dec_width;
	float   ra_tmax, ra_tmin, dec_tmax, dec_tmin;
        float   factor=0.75;
	char    imagename[800], imagename_in[800], rootname[800], detector[300];
	char    tile_in[200][200],tile_match[200];
        int     npixra_in, npixdec_in;
        double  tra_in, tdec_in;
	float   pixscale_in;
	void	select_dblogin();

	db_tiles *tile;
	db_files *im;
	db_wcsoffset *offset;
	FILE	*inp,*outp,*out,*pip;

	if (argc<4) {
	  printf("Usage: %s <imagename> <nite> <band> <project>\n", argv[0]);
          	  printf("  -quiet\n");
	  exit(0);
	}

	sprintf(rootname,"%s",argv[1]);
	sprintf(nitestring,"%s",argv[2]);
	sprintf(band,"%s",argv[3]);
	sprintf(project,"%s",argv[4]);

	if(!strcmp(argv[4],"BCS")) ccdnummax = 8;
	else if(!strcmp(argv[4],"DES")) ccdnummax = 64;
	else { printf(" ** <project> has to be either BCS or DES\n"); exit(0);}
	
	for (i=5;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}

	for (i=0;i<=ccdnummax;i++) match[i]=0;

	/* access dblogin */
	select_dblogin(dblogin);

	/* set up generic db call */
	sprintf(sqlcall,"sqlplus -S %s @ images.sql",dblogin);

	/* now cycle through ccdnumber for given image_name */
	for (ccdnum=1;ccdnum<=ccdnummax;ccdnum++) {	  

	  /* find out if the image exist in Files table */
	  out=fopen("images.sql","w");
	  fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF HEAD OFF TRIMS ON LINESIZE 1000;\n");
	  fprintf(out,"SELECT count(*)\n");
	  fprintf(out,"FROM files ");
	  fprintf(out,"WHERE BAND='%s' AND ",band);
	  fprintf(out," NITE like '%%%s%%' AND ",nitestring);
	  fprintf(out," CCD_NUMBER = %d AND", ccdnum);
	  fprintf(out," IMAGETYPE = 'object' AND ");
	  fprintf(out," IMAGENAME = '%s';", rootname);
	  fprintf(out," \n");
	  fprintf(out,"exit\n");
	  fclose(out);
	  
	  pip=popen(sqlcall,"r");
	  fgets(line,1000,pip);
	  sscanf(line,"%d", &nimages);
	  pclose(pip);
	  
	  if(nimages == 0)
	    {
	      printf(" ** Imagename %s for nite %s not found\n", rootname, nitestring);
	      exit(0);
	    }
	  
	  /* get the RA,DEC for this image from the Files table */
	  out=fopen("images.sql","w");
	  fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF HEAD OFF TRIMS ON LINESIZE 1000;\n");
	  fprintf(out,"SELECT ra,dec,detector\n");
	  fprintf(out,"FROM files ");
	  fprintf(out,"WHERE BAND='%s' AND ",band);
	  fprintf(out," NITE like '%%%s%%' AND ",nitestring);
	  fprintf(out," CCD_NUMBER = %d AND", ccdnum);
	  fprintf(out," IMAGETYPE = 'object' AND ");
	  fprintf(out," IMAGENAME = '%s';", rootname);
	  fprintf(out," \n");
	  fprintf(out,"exit\n");
	  fclose(out);
	  
	  pip=popen(sqlcall,"r");
	  fgets(line,1000,pip);
	  sscanf(line,"%f %f %s", &ra_img,&dec_img, detector);
	  pclose(pip);
	
	  /* query wcsoffset table to get the offsets */
	  out=fopen("images.sql","w");
	  fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF HEAD OFF TRIMS ON LINESIZE 1000;\n");
	  fprintf(out,"SELECT raoffset,decoffset,rahwidth,dechwidth\n");
	  fprintf(out,"FROM wcsoffset ");
	  fprintf(out,"WHERE CHIPID='%d' AND ",ccdnum);
	  fprintf(out," DETECTOR like '%%%s%%';\n ",detector);
	  fprintf(out,"exit\n");
	  fclose(out);

	  pip=popen(sqlcall,"r");
	  fgets(line,1000,pip);
	  sscanf(line,"%f %f %f %f", &ra_offset,&dec_offset, &ra_width,&dec_width);
	  pclose(pip);

	  /* get the image ra,dec and the four corners of the image */
	  scale = cos(dec_img*M_PI/180.0);
	  ra = ra_img + ra_offset/scale;
	  dec = dec_img + dec_offset;

	  maxra = ra + ra_width/scale; if (maxra>360.0) maxra-=360.0;
	  minra = ra - ra_width/scale; if (minra<0.0)   minra+=360.0;
	  maxdec = dec + dec_width;
	  mindec = dec - dec_width;

	  /* output info */ 
	  if(!flag_quiet)
	  {
	    printf("\n ** For image %s_%02d.fits at RA=%2.6f DEC=%2.6f with Detector %s\n", rootname, ccdnum, ra_img, dec_img, detector);
	    printf("    RA_OFFSET=%2.5f DEC_OFFSET=%2.5f RA_HWIDTH=%2.5f DEC_HWIDTH=%2.5f\n", ra_offset,dec_offset, ra_width,dec_width);
	    printf("    Corners of the images: %2.5f %2.5f %2.5f %2.5f\n", maxra,minra,maxdec,mindec);
	  }
	
	  /* first f ind the nearby tiles within 2.0degree */
	  out=fopen("images.sql","w");
	  fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF HEAD OFF TRIMS ON LINESIZE 1000;\n");
	  fprintf(out,"SELECT count(*)\n");
	  fprintf(out,"FROM coaddtile ");
	  fprintf(out,"WHERE PROJECT like '%%%s%%' ",project);
	  fprintf(out,"AND (RA between %2.6f AND %2.6f) ",ra-factor/scale,ra+factor/scale);
	  fprintf(out,"AND (DEC between %2.6f AND %2.6f);\n",dec-factor,dec+factor);
	  fprintf(out,"SELECT TILENAME,RA,DEC,PIXELSIZE,NPIX_RA,NPIX_DEC\n");
	  fprintf(out,"FROM coaddtile ");
	  fprintf(out,"WHERE PROJECT like '%%%s%%' ",project);
	  fprintf(out,"AND (RA between %2.6f AND %2.6f) ",ra-factor/scale,ra+factor/scale);
	  fprintf(out,"AND (DEC between %2.6f AND %2.6f);\n",dec-factor,dec+factor);
	  fprintf(out,"exit\n");
	  fclose(out);
	  
	  if(!flag_quiet) printf("\n");
	  i=-1;
	  pip=popen(sqlcall,"r");
	  while (fgets(line,1000,pip)!=NULL) {
	    if (i==-1) {
	      sscanf(line,"%d",&ntiles);
	      if (!flag_quiet) printf("  Found %d tiles from db\n",ntiles);
	      if (ntiles==0) {
	    	printf("  ** Query must select at least one tile\n");
	    	exit(0);
	      }
	      tile=(db_tiles *)calloc(ntiles,sizeof(db_tiles));
	    }
	    else {
	      if (!flag_quiet) printf("  ** %s",line);
	      /* read information into local variables */
	      sscanf(line,"%s %lf %lf %f %d %d",
		     tile[i].tilename,
		     &(tile[i].ra), &(tile[i].dec),
		     &(tile[i].pixelsize), &(tile[i].npix_ra), &(tile[i].npix_dec));
	      if (tile[i].pixelsize*tile[i].npix_ra>maxtilesize_ra) 
		maxtilesize_ra=tile[i].pixelsize*tile[i].npix_ra;
	      if (tile[i].pixelsize*tile[i].npix_dec>maxtilesize_dec) 
		maxtilesize_dec=tile[i].pixelsize*tile[i].npix_dec;
	    }
	    i++;
	  }
	  pclose(pip);
	  
	  system("rm images.sql");

	  /* cycle through tiles to find the tile within the image */
	  if(!flag_quiet) printf("\n");
	
	  for (j=0;j<ntiles;j++) { 
	    
	    flag = 0;

	    scale   =cos(tile[j].dec*M_PI/180.0);
	    deltara =tile[j].npix_ra*tile[j].pixelsize/2.0/3600.0;
	    deltadec=tile[j].npix_dec*tile[j].pixelsize/2.0/3600.0;
	        
	    ra_tmax = tile[j].ra + deltara/scale;
	    dec_tmax = tile[j].dec + deltadec;
	    ra_tmin = tile[j].ra - deltara/scale;
	    dec_tmin = tile[j].dec - deltadec;

	    //printf("For tile %s\t%2.5f %2.5f %2.5f %2.5f\n", tile[j].tilename, ra_tmin,ra_tmax, dec_tmin,dec_tmax);

	    /* check upper left corner of image */
	    if (minra>ra_tmin && minra<ra_tmax && maxdec>dec_tmin && maxdec<dec_tmax) flag=1;

	    /* check lower left corner of image */
	    if (minra>ra_tmin && minra<ra_tmax && mindec>dec_tmin && mindec<dec_tmax) flag=1;
	  
	    /* check lower right corner of image */
	    if (maxra>ra_tmin && maxra<ra_tmax && maxdec>dec_tmin && maxdec<dec_tmax) flag=1;

	    /* check upper right corner of image */
	    if (maxra>ra_tmin && maxra<ra_tmax && mindec>dec_tmin && mindec<dec_tmax) flag=1;

	    /* print results to information */
	    if (flag) {
	      
	      if (!flag_quiet)
		printf(" ** Found %s %12.7f %12.7f %10.7f %5d %5d \n", 
		       tile[j].tilename,tile[j].ra,tile[j].dec,
		       tile[j].pixelsize,tile[j].npix_ra,tile[j].npix_dec);

	      /* find the tile that is common to all ccd */
	      if(ccdnum == 1) {sprintf(tile_in[j], "%s", tile[j].tilename); count++;}
	      else
		{
		  for(k=0;k<count;k++) 
		    if(!strcmp(tile[j].tilename,tile_in[k])) 
		      {
			sprintf(tile_match,"%s",tile[j].tilename);
			tra_in = tile[j].ra;
			tdec_in = tile[j].dec;
			pixscale_in = tile[j].pixelsize;
			npixra_in = tile[j].npix_ra;
			npixdec_in = tile[j].npix_dec;
			
			match[k]++;
		      }
		}	        
				
	    } /* for if(flag) */
	  } /* for j loop */
	  free(tile);
	}
	
	if (!flag_quiet) printf(" \n ** Tile found common for all %d ccds:\n", ccdnummax);
	/* printout the matched tile info */
	for(k=0;k<count;k++) 
	  if(match[k] == (ccdnummax-1)) 
	      printf("%s %s %12.7f %12.7f %10.7f %5d %5d \n", rootname,
		     tile_match,tra_in,tdec_in,pixscale_in,npixra_in,npixdec_in);
	      
	return (0);
}
