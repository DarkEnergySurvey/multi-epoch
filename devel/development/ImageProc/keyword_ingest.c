/* This program will read specified keyword value from FITS file headers 
/* print to stdout and update the database
*/

#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
	FILE	*inp,*out,*flpt;
	char	comment[FLEN_COMMENT],imageclass[10],
	        imagename[200],imagetype[FLEN_VALUE],
		band[80],listname[500],keywordval[100],filename[1000],
	     	keyword[10],dbtag[100],outputname[500],outputlog[500],nite[500],
		command[500],runid[200],tilename[800];   
	int	i,status=0,ccdnum=0,flag_quiet=0,
		flag_remap=0,flag_reduced=0,flag_cat=0,flag_raw=0,flag_coadd=0;
	void	printerror(),filename_resolve();
	int	flag_db_update=0;

	fitsfile *fptr;

	if (argc<4) {
	  printf("keyword_ingest <fits-file list> <keyword> <dbtag>\n");
	  printf("  -db_update\n");
	  printf("  -quiet\n");
	  exit(0);
	}

	sprintf(listname,"%s",argv[1]);
	sprintf(keyword,"%s",argv[2]);
	sprintf(dbtag,"%s",argv[3]);
	
	/* process the command line */
	for (i=4;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-db_update")) flag_db_update=1;
	}

	sprintf(outputname,"keyword_ingest_%s.sql",keyword);
	sprintf(outputlog,"keyword_ingest_%s.log",keyword);

	/* open input list */
	inp=fopen(listname,"r");
	if (inp==NULL) {
	  printf("  **FITS file list %s not found\n",listname);
	  exit(0);
	}

	/* open sql update file if needed */
	if (flag_db_update) {
	  out=fopen(outputname,"w");
	  if (out==NULL) {
	    printf("  ** Could not open sql update file %s\n",outputname);
	    exit(0);
	  }	
	}

	/* cycle through the imagelist grabbing information and outputting it */
	while (fscanf(inp,"%s",filename)!=EOF) {

	  flag_cat=flag_reduced=flag_remap=flag_coadd=flag_raw=0;
	  imageclass[0]=imagetype[0]=runid[0]=tilename[0]=nite[0]=band[0]=0;

	  /* **************************************************** */
	  /* ************** Extract db location tags ************ */
	  /* **************************************************** */

	  filename_resolve(filename,imageclass,runid,nite,tilename,
	    imagetype,imagename,band,&ccdnum);

	  /* find out if it is a catalog fits file */
	  if (!strcmp( band,"raw")) {
	    flag_raw=1;
	    runid[0]=0;
	  }
	  else { 
	    if(!strcmp(imagetype,"catalog")) flag_cat=1; 
	    if(!strcmp(imagetype,"reduced")) flag_reduced=1; 
	    if(!strcmp(imagetype,"remap")) flag_remap=1; 
	    if(!strcmp(imagetype,"coadd")) flag_coadd=1; 
	  }

	  if (!flag_quiet) 
	    printf("  keyword_ingest:  runid: %s nite: %s band: %s tilename: %s imagetype: %s ccdnum: %d\n",runid,nite,band,tilename,imagetype,ccdnum);

	  /* clear some fields if they are not needed */
	  if (flag_raw) runid[0]=0;
	  if (flag_remap) sprintf(imagetype,"remap");
	  if (flag_reduced) sprintf(imagetype,"reduced");
	  if (flag_cat) sprintf(imagetype,"catalog");
	  if (flag_coadd) sprintf(imagetype,"coadd");


	  /* **************************************************** */
	  /* ***** Read FITS file to extract keyword value  ***** */
	  /* **************************************************** */

	  /* open the FITS file */
	  if (fits_open_file(&fptr,filename,READONLY,&status)) 
	    printerror(status);
	  if (!flag_quiet) printf("  Opened %s\n",filename);	
	  
	  /* get parameters we need for the FILES database table. */
	  if (fits_read_key_str(fptr,keyword,keywordval,comment,&status)==
	    KEY_NO_EXIST) {
	    if (!flag_quiet) 
	      printf("\n  **keyword_ingest:  %s not found in %s\n",
		keyword,filename);
	    status=0;
	  }
	  else if (!flag_quiet) printf("  %s:  %s\n",keyword,keywordval);
	  
          /* close the new image */
	  if (fits_close_file(fptr,&status)) printerror(status);
	  if (!flag_quiet) printf("  Closed image %s\n",filename);

	  /* **************************************************** */
	  /* ******* Write db update call to output file  ******* */
	  /* **************************************************** */

	  if (flag_db_update) {
	    fprintf(out,"update files set %s=%s\n",dbtag,keywordval);
	    fprintf(out,"  where imageclass='%s' and nite='%s' and imagename='%s' and imagetype='%s'\n  ",
	      imageclass,nite,imagename,imagetype);
	    if (runid[0]!=0) fprintf(out,"and runiddesc='%s' ",runid);
	    if (band[0]!=0) fprintf(out,"and band='%s' ",band);
	    if (tilename[0]!=0) fprintf(out,"and tilename='%s' ",tilename);
	    if (ccdnum!=0) fprintf(out,"and ccd_number=%d ",ccdnum);
	    fprintf(out,";\n");
	  }
	} 
	  
	fclose(inp);
	fclose(out);

	/* update the db */
	if (flag_db_update) { 
	  sprintf(command,"sqlplus pipeline/dc01user@charon/des < %s > %s",
	    outputname,outputlog);
	  system(command);
	}

	return(0);
}
