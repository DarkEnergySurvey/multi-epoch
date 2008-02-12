
/* this program meant to create a mapping between all selected coadd tiles
/* and the images that overlap these tiles */

#include "imageproc.h"

/* define a minimum overlap with TILE for us to both remapping */

#define MINIMUM_OVERLAP (0.5/60.0)  /* half an arcminute */

main(argc,argv)
	int argc;
	char *argv[];
{
	char	command[200],sqlcall[500],nitestring[100],filter[5][10],
		line[1000],sgn[10],project[100],tilestring[100],band[100],
		inputfile[1000],outputfile[1000];
	int	i,j,k,flag,flag_quiet=0,ntiles,nimages,noffsets,
	        nobjects,imageid,objectid,maxzp_n,allobjects=0, imm,
		flag_ccd=0,flag_nite=0,flag_list=0,flag_out=0,rightfit,count, len, ccdnum, imnum;
	float	zeropoint,maxtilesize_ra=0.0,maxtilesize_dec=0.0,
	        deltadec,deltara,scale,minra,maxra,mindec,maxdec,ra,dec;
	float   ra_img, dec_img, ra_offset, dec_offset, ra_width, dec_width;
	float   ra_tmax, ra_tmin, dec_tmax, dec_tmin;
        /*	This is Choong's variable that I don't understand 
	float   factor=0.75;
	*/
	float	pixelsize,oldpixelsize;
	int	npix_ra,npix_dec,oldnpix_ra,oldnpix_dec;
	char    imagename[800], imagename_in[800], 
		bandin[50],rootname[800], rootname1[800], detector[300],
		dblogin[500];
	void	select_dblogin();

	db_tiles *tile;
	db_files *im;
	db_wcsoffset *offset;
	FILE	*inp,*outp,*out,*pip;

	if (argc<4) {
	  printf("Usage: %s <image_name.fits or list> <nite> <band> <project>\n", argv[0]);
          printf("  -output <output.list>\n");
	  printf("  -quiet\n");
	  exit(0);
	}

	sprintf(inputfile,"%s",argv[1]);
	sprintf(nitestring,"%s",argv[2]);
	sprintf(band,"%s",argv[3]);
	sprintf(project,"%s",argv[4]);

	for (i=5;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-output")) 
	    {
	      flag_out=1;
	      sprintf(outputfile,"%s",argv[i+1]);
	      outp=fopen(outputfile,"w"); 
	    }
	}

	/* copy input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5))  {
	  sprintf(imagename_in,"%s",argv[1]);
	  imnum=1;
	}
	else { /* expect file containing list of images */
	  imnum=0;flag_list=1;
	  inp=fopen(argv[1],"r");
	  while (fscanf(inp,"%s",imagename)!=EOF) {
	    imnum++;
	    if (strncmp(&(imagename[strlen(imagename)-5]),".fits",5)) {
	      printf("  ** File must contain list of FITS images **\n");
	      exit(0);
	    }
	  }
	  fclose(inp);
	  /* reopen file for processing */
	  inp=fopen(argv[1],"r");
	}

	if(!flag_quiet && flag_list) 
	  printf("\n ** Input list %s contains %d images \n", argv[1], imnum);

	/* access dblogin */
	select_dblogin(dblogin);

	/* set up generic db call */
	sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s @ images.sql",dblogin);

	/* first get the RA,DEC Tilesize from the coaddtile table */
	out=fopen("images.sql","w");
	fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF HEAD OFF TRIMS ON LINESIZE 1000;\n");
	fprintf(out,"SELECT pixelsize,npix_ra,npix_dec\n");
	fprintf(out,"FROM coaddtile ");
	fprintf(out,"WHERE PROJECT='%s';\n",project);
	fprintf(out,"exit;\n");
	fclose(out);
	  
	pip=popen(sqlcall,"r");
	fgets(line,1000,pip);
	sscanf(line,"%f %d %d", &oldpixelsize,&oldnpix_ra,&oldnpix_dec);
	while (fgets(line,1000,pip)!=NULL) {
	  sscanf(line,"%f %d %d", &pixelsize,&npix_ra,&npix_dec);
	  if (pixelsize!=oldpixelsize) {
	    printf("  **Tiles have different pixel sizes\n");
	    exit(0);
	  }
	  if (npix_ra!=oldnpix_ra || npix_dec!=oldnpix_dec) {
	    if (npix_ra>oldnpix_ra) oldnpix_ra=npix_ra;
	    if (npix_dec>oldnpix_dec) oldnpix_dec=npix_dec;
	  }
	}
	pclose(pip);
	pixelsize=oldpixelsize;npix_ra=oldnpix_ra;npix_dec=oldnpix_dec;
	if (!flag_quiet) printf("  Using tilesize %dX%d with %.4f arcsec pixels\n",
	  npix_ra,npix_dec,pixelsize);
	/* convert pixelsize to degrees */
	pixelsize/=3600.0;
	
	/* now cycle through input images */
	for (imm=0;imm<imnum;imm++) {

	  /* get next image name */
	  if (flag_list) fscanf(inp,"%s",imagename_in);
	
	  /* get the ccd_number and the rootname from the input imagename */
	  /* input imagename: band/rootname/rootname_ccd_number.fits */
	  len=strlen(imagename_in);
	  for (i=len;i>0;i--) {
	    if (!strncmp(&(imagename_in[i]),".fits",5)) imagename_in[i]=0;
	    if (!strncmp(&(imagename_in[i]),"/",1)) imagename_in[i]=32;
	  }	
	  sscanf(imagename_in,"%s %s %s",bandin,rootname,imagename);
	  len=strlen(imagename);
	  for (i=len-1;i>=0;i--) 
	    if (!strncmp(imagename+i,"_",1)) break;
	    sscanf(imagename+i+1,"%2d",&ccdnum);

	  /* find out if the image exist in Files table */
	  out=fopen("images.sql","w");
	  fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF HEAD OFF TRIMS ON LINESIZE 1000;\n");
	  fprintf(out,"SELECT count(*)\n");
	  fprintf(out,"FROM files ");
	  fprintf(out,"WHERE BAND='%s' AND ",band);
	  fprintf(out," NITE like '%%%s%%' AND ",nitestring);
	  fprintf(out," CCD_NUMBER = %d AND", ccdnum);
	  fprintf(out," upper(IMAGETYPE) = 'OBJECT' AND ");
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
	  fprintf(out," upper(IMAGETYPE) = 'OBJECT' AND ");
	  fprintf(out," IMAGENAME = '%s';", rootname);
	  fprintf(out," \n");
	  fprintf(out,"exit\n");
	  fclose(out);
	  
	  pip=popen(sqlcall,"r");
	  fgets(line,1000,pip);
	  sscanf(line,"%f %f %s", &ra_img,&dec_img, detector);
	  if (!flag_quiet) printf("  Location:  %s",line);
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
	  if (!flag_quiet) printf("  Offsets:  %s",line);
	  pclose(pip);

	  /* get the image ra,dec and the four corners of the image */
	  scale = cos(dec_img*M_PI/180.0);
	  ra = ra_img + ra_offset/scale;
	  dec = dec_img + dec_offset;

	  maxra = ra + ra_width/scale + npix_ra/2.0*pixelsize/scale - 
	    MINIMUM_OVERLAP/scale; 
	  if (maxra>360.0) maxra-=360.0;
	  minra = ra - ra_width/scale - npix_ra/2.0*pixelsize/scale + 
	    MINIMUM_OVERLAP/scale; 
	  if (minra<0.0)   minra+=360.0;
	  maxdec = dec + dec_width + npix_dec/2.0*pixelsize - 
	    MINIMUM_OVERLAP;
	  mindec = dec - dec_width - npix_dec/2.0*pixelsize + 
	    MINIMUM_OVERLAP;

	  /* output info */ 
	  if(!flag_quiet)
	  {
	    printf("\n ** For image %s/%s_%02d.fits at RA=%2.6f DEC=%2.6f with Detector %s\n", rootname, rootname, ccdnum, ra_img, dec_img, detector);
	    printf("    RA_OFFSET=%2.5f DEC_OFFSET=%2.5f RA_HWIDTH=%2.5f DEC_HWIDTH=%2.5f\n", ra_offset,dec_offset, ra_width,dec_width);
	    printf("    Corners of search region for image: %2.5f %2.5f %2.5f %2.5f\n", minra,maxra,mindec,maxdec);
	  }
	
	  /* first find the nearby tiles within 2.0degree */
	  out=fopen("images.sql","w");
	  fprintf(out,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF HEAD OFF TRIMS ON LINESIZE 1000;\n");
	  fprintf(out,"SELECT count(*)\n");
	  fprintf(out,"FROM coaddtile ");
	  fprintf(out,"WHERE PROJECT like '%%%s%%' ",project);
	  /* following lines are Choong's code-- cannot understand this
	  fprintf(out,"AND (RA between %2.6f AND %2.6f) ",ra-factor/scale,ra+factor/scale);
	  fprintf(out,"AND (DEC between %2.6f AND %2.6f);\n",dec-factor,dec+factor);
	  */
	  if (minra>maxra) {
	    fprintf(out,"AND ((RA>%2.6f AND RA<=360.0) ",minra);
	    fprintf(out,"OR (RA<%2.6f AND RA>=0.0)) ",maxra);
	  }
	  else fprintf(out,"AND (RA between %2.6f AND %2.6f) ",minra,maxra);
	  fprintf(out,"AND (DEC between %2.6f AND %2.6f);\n",mindec,maxdec);
	  fprintf(out,"SELECT TILENAME,RA,DEC,PIXELSIZE,NPIX_RA,NPIX_DEC\n");
	  fprintf(out,"FROM coaddtile ");
	  fprintf(out,"WHERE PROJECT like '%%%s%%' ",project);
	  /* followings line are Choong's code--- cannot understand this 
	  fprintf(out,"AND (RA between %2.6f AND %2.6f) ",ra-factor/scale,ra+factor/scale);
	  fprintf(out,"AND (DEC between %2.6f AND %2.6f);\n",dec-factor,dec+factor);
	  */
	  if (minra>maxra) {
	    fprintf(out,"AND ((RA>%2.6f AND RA<=360.0) ",minra);
	    fprintf(out,"OR (RA<%2.6f AND RA>=0.0)) ",maxra);
	  }
	  else fprintf(out,"AND (RA between %2.6f AND %2.6f) ",minra,maxra);
	  fprintf(out,"AND (DEC between %2.6f AND %2.6f);\n",mindec,maxdec);
	  fprintf(out,"exit\n");
	  fclose(out);
	  
	  if(!flag_quiet) printf("\n");
	  i=-1;
	  pip=popen(sqlcall,"r");
	  while (fgets(line,1000,pip)!=NULL) {
	    if (!flag_quiet) printf("%s",line);
	    if (i==-1) {
	      sscanf(line,"%d",&ntiles);
	      if (!flag_quiet) printf("  Found %d tiles from db\n",ntiles);
	      if (ntiles==0) {
	    	printf("  ** No tile found for %s\n",rootname);
		break;
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
	      if (!flag_quiet)
		printf(" ** Found %s %12.7f %12.7f %10.7f %5d %5d \n", 
		       tile[i].tilename,tile[i].ra,tile[i].dec,
		       tile[i].pixelsize,tile[i].npix_ra,tile[i].npix_dec);
	      else
		printf("%s %s/%s_%02d.fits %s %12.7f %12.7f %10.7f %5d %5d \n", rootname, rootname, rootname, ccdnum,
		       tile[i].tilename,tile[i].ra,tile[i].dec,
		       tile[i].pixelsize,tile[i].npix_ra,tile[i].npix_dec);
	      
	      if(flag_out)
		fprintf(outp, "%s %s/%s_%02d.fits %s %12.7f %12.7f %10.7f %5d %5d \n", rootname, rootname, rootname, ccdnum,
		       tile[i].tilename,tile[i].ra,tile[i].dec,
		       tile[i].pixelsize,tile[i].npix_ra,tile[i].npix_dec);
	    }
	    i++;
	  }
	  pclose(pip);
	  
	}
	
	system("rm images.sql");
	if(flag_list) fclose(inp);
	if(flag_out) fclose(outp);

	return (0);
}

#undef MINIMUM_OVERLAP 

