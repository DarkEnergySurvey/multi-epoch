/* This program takes an existing bpm image and an input list of bpm regions
/* in ds9 region file format and masks out those regions and then saves the 
/* the output bpm image.
*/
#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{

	int	status=0,hdunum,quiet;	
	void	printerror();
	int	i,j,hdutype,xmin,xmax,ymin,ymax;
	char	comment[200],imagename[500],longcomment[10000],
		regionline[1000],goodregionfile[500],
		badregionfile[500];
	float	min=0.1,max=3.0,biasmax=1000.0;
	desimage bpm,newbpm;
	int	flag_quiet=0,flag_goodpix=0,flag_badpix=0;
	void	rd_desimage();
	FILE	*inpgood,*inpbad,*out,*pip;

	if (argc<3) {
	  printf("editbpm <bpm input> <bpm output> <options>\n");
	  printf("  -badpix <region file>\n");
	  printf("  -goodpix <region file>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
	
	/* get input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5) ||
	  !strncmp(&(argv[1][strlen(argv[1])-8]),".fits.gz",8))  {
	  sprintf(bpm.name,"%s",argv[1]);
	}
	else {
	  printf("Input image must be FITS or gzipped FITS file\n");
	  exit(0);
	}
	
	/* prepare output image */
	if (!strncmp(&(argv[2][strlen(argv[2])-5]),".fits",5)) {
	 sprintf(newbpm.name,"!%s",argv[2]);
	}
	else {
	  printf("  ** FITS output image required **\n");
	  exit(0);
	}

	/* process command line */
	for (i=3;i<argc;i++) {
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	  if (!strcmp(argv[i],"-badpix")) {
	    flag_badpix=1;
	    i++;
	    sprintf(badregionfile,"%s",argv[i]);
	    inpbad=fopen(badregionfile,"r");
	    if (inpbad==NULL) {
	      printf("  ** Region file %s not found.\n",badregionfile);
	      exit(0);
	    }
	  }
	  if (!strcmp(argv[i],"-goodpix")) {
	    flag_goodpix=1;
	    i++;
	    sprintf(goodregionfile,"%s",argv[i]);
	    inpgood=fopen(goodregionfile,"r");
	    if (inpgood==NULL) {
	      printf("  ** Region file %s not found.\n",goodregionfile);
	      exit(0);
	    }
	  }
	}

	/* ************************* */	
	/* **** READ BPM FILES ***** */
	/* ************************* */

	/* read the input bpm */
	rd_desimage(&bpm,READONLY,flag_quiet);
	/* prepare output image */
	newbpm.npixels=bpm.npixels;
	newbpm.nfound=bpm.nfound;
	for (i=0;i<bpm.nfound;i++) newbpm.axes[i]=bpm.axes[i];
	newbpm.bitpix=USHORT_IMG;
	/* only one mask array is needed */
	newbpm.mask=bpm.mask;

	/* *********************** */	
	/* **** PROCESS  BPM ***** */
	/* *********************** */

	/* read the bad region file */
	if (flag_badpix) {
	  if (!flag_quiet) printf("\n  Processing badpix regionfile %s\n",
	    badregionfile);
	  while (fgets(regionline,1000,inpbad)!=NULL) {
	    if (!strncmp(regionline,"rectangle",9) ||
	      !strncmp(regionline,"RECTANGLE",9)) {
	      if (!flag_quiet) printf("    %s",regionline);
	      sscanf(regionline+10,"%d %d %d %d",&xmin,&xmax,&ymin,&ymax);
	      /* assume these range 1... max rather than 0... max-1 */
	      xmin--;xmax--;ymin--;ymax--;
	      if (xmin<0 || xmin>=newbpm.axes[0] || 
	        xmax<0 || xmax>=newbpm.axes[0]) {
	        printf("  ** xmin/xmax outside range %d:%d\n",1,newbpm.axes[0]);
	        exit(0);
	      }
	      if (ymin<0 || ymin>=newbpm.axes[1] || 
	        ymax<0 || ymax>=newbpm.axes[1]) {
	        printf("  ** ymin/ymax outside range %d:%d\n",1,newbpm.axes[1]);
	        exit(0);
	      }
	      /* for this region set all pixels */
	        for (i=xmin;i<=xmax;i++) for (j=ymin;j<=ymax;j++) 
	        newbpm.mask[i+j*newbpm.axes[0]]=1;
	    }
	    else {
	      printf(" accepted shapes:\n");
	      printf("          rectangle <xmin> <xmax> <ymin> <ymax>\n");
	      exit(0);
	    }
	  }
	  fclose(inpbad);
	}

	/* read the good region file */
	if (flag_goodpix) {
	  if (!flag_quiet) printf("\n  Processing goodpix regionfile %s\n",
	    goodregionfile);
	  while (fgets(regionline,1000,inpgood)!=NULL) {
	    if (!strncmp(regionline,"rectangle",9) ||
	      !strncmp(regionline,"RECTANGLE",9)) {
	      if (!flag_quiet) printf("    %s",regionline);
	      sscanf(regionline+10,"%d %d %d %d",&xmin,&xmax,&ymin,&ymax);
	      /* assume these range 1... max rather than 0... max-1 */
	      xmin--;xmax--;ymin--;ymax--;
	      if (xmin<0 || xmin>=newbpm.axes[0] || 
	        xmax<0 || xmax>=newbpm.axes[0]) {
	        printf("  ** xmin/xmax outside range %d:%d\n",1,newbpm.axes[0]);
	        exit(0);
	      }
	      if (ymin<0 || ymin>=newbpm.axes[1] || 
	        ymax<0 || ymax>=newbpm.axes[1]) {
	        printf("  ** ymin/ymax outside range %d:%d\n",1,newbpm.axes[1]);
	        exit(0);
	      }
	      /* for this region set all pixels */
	      for (i=xmin;i<=xmax;i++) for (j=ymin;j<=ymax;j++) 
	        newbpm.mask[i+j*newbpm.axes[0]]=0;
	    }
	    else {
	      printf(" accepted shapes:\n");
	      printf("          rectangle <xmin> <xmax> <ymin> <ymax>\n");
	      exit(0);
	    }
	  }
	  fclose(inpgood);
	}

	/* *********************** */	
	/* **** SAVE RESULTS ***** */
	/* *********************** */

	/* create the new file */
        if (fits_create_file(&newbpm.fptr,newbpm.name,&status)) 
	  printerror(status);

	/* copy header from bpm to newbpm */
	if (fits_copy_header(bpm.fptr,newbpm.fptr,&status))
	  printerror(status);

	/* write the corrected image*/
	if (fits_write_img(newbpm.fptr,TUSHORT,1,newbpm.npixels,newbpm.mask,
	  &status))  printerror(status);
	    
	/* store information in the header */
	pip=popen("date","r");
	fgets(comment,200,pip);
	comment[strlen(comment)-1]=0;
	pclose(pip);
	if (fits_write_key_str(newbpm.fptr,"DESBPM",comment,"bad pixel map edited",&status)) printerror(status);
	sprintf(longcomment,"DESDM:");
	for (i=0;i<argc;i++) sprintf(longcomment,"%s %s",longcomment,argv[i]);
	if (!flag_quiet) printf("\n  %s\n", longcomment);
	if (fits_write_comment(newbpm.fptr,longcomment,&status)) printerror(status);
        /* close the original and corrected images */
        if (fits_close_file(newbpm.fptr,&status)) printerror(status);
        if (fits_close_file(bpm.fptr,&status)) printerror(status);
	if (!flag_quiet) printf("  Closed %s: 2D ( %d X %d )\n",
	  &(newbpm.name[1]),newbpm.axes[0],newbpm.axes[1]);	  
	if (!flag_quiet) printf("\n\n");
}

