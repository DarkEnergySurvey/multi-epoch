#include "fitsio.h"

#define MAXFILE 500

main(argc,argv)
	int argc;
	char *argv[];
{
  	char 	infile[MAXFILE][500],outfile[500],command[1000],listname[500];
  	int  	status=0, hdunum, hdutype,morekey=1,nfiles,i,j,hdustart,
		flag_quiet=0,flag_cleanup=0,flag_list=0,flag_no0exthead=0;
	FILE	*inp;
  	fitsfile *infptr,*outfptr;

  	if (argc<4) {
      	  printf("fitscombine <file1.fits> ... <fileN.fits> <output.fits>\n");
	  printf("  -list <name>\n");
	  printf("  -no0exthead\n");
	  printf("  -cleanup\n");
	  printf("  -quiet\n");
      	  exit(0);
    	}
	 
	if (argc>=MAXFILE) {
	  printf(" ** fitscombine:  MAXFILE exceeded\n");
	  exit(0);
	}

	/* process command line */
	nfiles=0;
	for (i=1;i<argc;i++) {
	  if (!strncmp(&(argv[i][strlen(argv[i])-5]),".fits",5)) {
	    /* create room for and store name */
	    sprintf(infile[nfiles],"%s",argv[i]);
	    nfiles++;
	  }
	  if (!strcmp(argv[i],"-list")) {
		flag_list=1;
	        i++;
		sscanf(argv[i],"%s",listname);
	  }
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-no0exthead")) flag_no0exthead=1;
	  if (!strcmp(argv[i],"-cleanup")) flag_cleanup=1;
	}


	sprintf(outfile,"!%s",infile[nfiles-1]);

	/* if a list then read in filenames */
	if (flag_list) {
	  inp=fopen(listname,"r");
	  nfiles=0;
	  while (fscanf(inp,"%s",infile+nfiles)!=EOF) {
	    nfiles++;
	    if (nfiles>=MAXFILE) {
	      printf(" ** fitscombine:  MAXFILE exceeded\n");
	      exit(0);
	    }
	  }
	  nfiles++;
	  fclose(inp);
	}

	if (!flag_quiet) printf("  Combining %d FITS files into %s\n",
	  nfiles-1,outfile+1);

	/* open output file */
  	if (fits_create_file(&outfptr,outfile,&status)) printerror(status);
	/* cycle through files */
	for (i=0;i<nfiles-1;i++) {
	  /* open fits files */
   	  if (fits_open_file(&infptr,infile[i],READONLY,&status)) 
	    printerror(status);
  	    /* Get the HDU number */
  	    if (fits_get_num_hdus(infptr,&hdunum,&status)) 
	      printerror(status);
  	    printf("  %s has %d HDU\n",infile[i],hdunum);
	    if (flag_no0exthead) hdustart=2;
	    else hdustart=1;
	    for (j=hdustart;j<=hdunum;j++) {
	      if (fits_movabs_hdu(infptr,j,&hdutype,&status))
		printerror(status);
  	      /* copy the HDU */
  	      if (fits_copy_hdu(infptr,outfptr,morekey,&status))
		printerror(status);  
	    }
	  /* close input file ptr */
	  if (fits_close_file(infptr,&status)) printerror(status);
	  /* cleanup if needed */
	  if (flag_cleanup) {
	    sprintf(command,"rm %s",infile[i]);
	    if (!flag_quiet) printf("  %s\n",command);
	    system(command);
	  }
	}
  	if (fits_get_num_hdus(outfptr,&hdunum,&status)) 
	  printerror(status);
  	if (!flag_quiet) printf("  Output file %s has %d HDU\n", 
	  outfile+1,hdunum);
  	/* close output file */
  	if (fits_close_file(outfptr,&status)) printerror(status);
	
}
  
#undef MAXFILE
