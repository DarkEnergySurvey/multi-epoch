#include <stdio.h>

#define SCIENCE 1
#define BIAS 2
#define FLAT 3
#define GBAND 0
#define RBAND 1
#define IBAND 2
#define ZBAND 3


main(argc,argv)
	int argc;
	char *argv[];
{
	char	imagename[500],imagetype[500],band[500],outname[500],
		nametag[50],bandtag[4][4]={"g","r","i","z"},dir[500],
		outfile[500],**line;
	int	i,num,ccdnum,flag_band,flag_type=0,bandcount[4]={0,0,0,0};
	FILE	*inp,*out[4],*out2[4];

	if (argc<2) {
	  printf("makelist <file> <listdir>\n");
	  exit(0);
	}
	sprintf(dir,"%s",argv[2]);

	/* set file pointers so we can tell which ones have been opened */
	for (i=0;i<4;i++) out[i]=out2[i]=NULL;

	/* read file into memory */
	inp=fopen(argv[1],"r");
	if (inp==NULL)  {
	  printf("  makelist:  File not found %s\n",argv[1]);
	  exit(0);
	}
	fscanf(inp,"%d",&num);
	printf("  makelist:  %d images in file %s\n",num,argv[1]);
	line=(char **)calloc(num,sizeof(char *));
	for (i=0;i<num;i++) {
          line[i]=(char *)calloc(1000,sizeof(char));
	  fgets(line[i],1000,inp);
	  /* first time around read twice to get past first line */
	  if (i==0) fgets(line[i],1000,inp);
	  sscanf(line[i],"%s %s %d %s",imagename,band,&ccdnum,imagetype);
	  /*printf("%d %s",i,line[i]);*/
	  if (!strncmp(band,"g",1)) bandcount[GBAND]++;
	  if (!strncmp(band,"r",1)) bandcount[RBAND]++;
	  if (!strncmp(band,"i",1)) bandcount[IBAND]++;
	  if (!strncmp(band,"z",1)) bandcount[ZBAND]++;
	}
	fclose(inp);

	printf("--making list for CCD%02d\n", ccdnum);

	/* choose tag for name */
	if (!strncmp(imagetype,"zero",4) || !strncmp(imagetype,"ZERO",4) ||
	  !strncmp(imagetype,"BIAS",4) || !strncmp(imagetype,"bias",4)) {
	  sprintf(nametag,"bias");
	  flag_type=BIAS;
	  printf("  makelist:  imagetype bias\n");
	}
	if (!strncmp(imagetype,"object",6) || !strncmp(imagetype,"OBJECT",6)) {
	  sprintf(nametag,"science"); 
	  flag_type=SCIENCE;
	  printf("  makelist:  imagetype object g(%d) r(%d) i(%d) z(%d)\n",
	    bandcount[0],bandcount[1],bandcount[2],bandcount[3]);
	}
	if (!strncmp(imagetype,"dome",4) || !strncmp(imagetype,"DOME",4) || 
	  !strncmp(imagetype,"flat",4) || !strncmp(imagetype,"FLAT",4)) {
	  sprintf(nametag,"flat");
	  flag_type=FLAT;
	  printf("  makelist:  imagetype flat g(%d) r(%d) i(%d) z(%d)\n",
	    bandcount[0],bandcount[1],bandcount[2],bandcount[3]);
	}

	/* open appropriate files */
	if (flag_type==BIAS) {
	  sprintf(outfile,"%s/%s_%02d.list",dir,nametag,ccdnum);
	  out[0]=fopen(outfile,"w");
	}
	else { /* either flats or science images */
	  for (i=0;i<4;i++) {
	    if (bandcount[i]>0) {
	      sprintf(outfile,"%s/%s_%s_%02d.list",
	        dir,nametag,bandtag[i],ccdnum);
	      out[i]=fopen(outfile,"w");
	      if (flag_type==SCIENCE) {
	        sprintf(outfile,"%s/%sout_%s_%02d.list",
	          dir,nametag,bandtag[i],ccdnum);
	        out2[i]=fopen(outfile,"w");
	      }
	    }
	  }
	}

	/* now write the files */
	for (i=0;i<num;i++) {
	  sscanf(line[i],"%s %s %d %s",imagename,band,&ccdnum,imagetype);
	  /*printf("%s",line[i]);*/
	  if (flag_type==BIAS) 
	    fprintf(out[0],"raw/%s/%s_%02d.fits\n",imagename,imagename,ccdnum);
	  else {
	    if (!strncmp(band,"g",1)) flag_band=GBAND;
	    if (!strncmp(band,"r",1)) flag_band=RBAND;
	    if (!strncmp(band,"i",1)) flag_band=IBAND;
	    if (!strncmp(band,"z",1)) flag_band=ZBAND;
	    fprintf(out[flag_band],"raw/%s/%s_%02d.fits\n",imagename,
	      imagename,ccdnum);
	    if (flag_type==SCIENCE) 
	      fprintf(out2[flag_band],"%s/%s/%s_%02d.fits\n",
	        bandtag[flag_band],imagename,imagename,ccdnum);
	  }
	}

	/* close the files */
	for (i=0;i<4;i++) {
	  if (out[i]!=NULL) fclose(out[i]);
	  if (out2[i]!=NULL) fclose(out2[i]);
	}

	return(0);
}

#undef BIAS
#undef FLAT
#undef SCIENCE
#undef GBAND
#undef RBAND
#undef IBAND
#undef ZBAND

