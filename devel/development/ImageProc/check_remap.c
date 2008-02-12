/* 

This program check the dimension of the remaps images and the number of objects in the remap catalogs, and output a new list for QC that excluding empty image/catalog 

NOTE: -  have to run in the "nite" directory for pipeline operation
      -  need to add DB query for general purpose usage (PENDING)

Version - 1.00 : initial version
          1.01 : add codes that check the HDU to get the correct row number
*/

#include "imageproc.h"

#define VERSION 1.01

main(int argc, char *argv [])
{
  char inlist[1000],imagename_in[1000],image[1000],catalog[1000],root[1000],comment[1000],outlist[1000];
  long naxes[2],nrow;
  int i,j,len,nfound,hdutype,status=0,hdunum=0;
  int flag_quiet=0;
  void printerror();

  FILE *flist,*fout;
  fitsfile *fptr_im,*fptr_cat;

  if (argc<2) 
    {
      printf("Usage: %s <remap.list>\n", argv[0]);
      printf("        Option:\n");
      printf("                -version\n");      
      printf("                -quiet\n");      
      exit(0);
    }
  
  /* process the command line */
  for (i=1;i<argc;i++) {
   
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;

    if (!strcmp(argv[i],"-version"))
      {
	printf("%s: Version %2.2f \n",argv[0],VERSION);
	exit(0);
      }
  }
  
  sprintf(inlist, "%s", argv[1]);
  sprintf(outlist,"%s_qc",inlist);

  /* open the list for process */
  flist=fopen(inlist,"r");
  fout=fopen(outlist,"w");
  
  while (fscanf(flist,"%s",imagename_in)!=EOF) {
    /* check the input list */
    if (strncmp(&(imagename_in[strlen(imagename_in)-5]),".fits",5)) {
      printf("  ** File must contain list of FITS images **\n");
      exit(0);
    }
    else /* cp the image to _im.fits and _cat.fits */ 
      {
	sprintf(root,"%s",imagename_in);
	len=strlen(imagename_in);
	for (i=len;i>0;i--) if (!strncmp(&(imagename_in[i]),".fits",5)) {
	  root[i]=0;
	  break;
	}

	sprintf(image,"%s_im.fits",root);
	sprintf(catalog,"%s_cat.fits",root);
      }

    /* read in the naxis keywords and print out the image size */
    status=0;
    if (fits_open_file(&fptr_im,image,READONLY,&status)) printerror(status);
    if (fits_read_keys_lng(fptr_im,"NAXIS",1,2,naxes,&nfound,&status)) printerror(status);
    if (fits_close_file(fptr_im,&status)) printerror(status);

    if (!flag_quiet) printf("%s:  %d X %d \n",image,naxes[0],naxes[1]);   

    /* read in the number of rows in catalog and print out the value */
    status=0;
    if (fits_open_file(&fptr_cat,catalog,READONLY,&status)) printerror(status);
    fits_get_hdu_num(fptr_cat, &hdunum);
   
    if(hdunum == 1) {
	/* This is the primary array;  try to move to the */
	/* first extension and see if it is a table */
	fits_movabs_hdu(fptr_cat, 2, &hdutype, &status);
	fits_get_hdu_type(fptr_cat, &hdutype, &status); 
    }
    else
	fits_get_hdu_type(fptr_cat, &hdutype, &status); /* Get the HDU type */
     
    if (hdutype == IMAGE_HDU)
	printf("Error: this program only displays tables, not images\n");
    else  
	if (fits_get_num_rows(fptr_cat,&nrow,&status)) printerror(status);

    if (fits_close_file(fptr_cat,&status)) printerror(status);

    if (!flag_quiet) printf("%s:  %d \n",catalog,nrow);   
   

    if (!flag_quiet) printf("\n");
    
    /* output to a new list if the catalog is not empty */
    if(nrow>0)
	fprintf(fout,"%s\n", imagename_in);
  }
  fclose(flist); fclose(fout);

}

#undef VERSION
