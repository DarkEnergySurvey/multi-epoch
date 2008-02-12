#include "imageproc.h"

main(argc,argv)
	int argc;
	char *argv[];
{
	int	flag_quiet=0,flag_output=0,i,hdutype,status=0;
	static 	desimage data,output;
	void	rd_desimage();
	char	imagename[500],root[100],comment[100],type[100];

	if (argc<2) {
	  printf("splitimage <image>\n");
	  printf("  -output <root>\n");
	  printf("  -quiet\n");
	  exit(0);
	}
		
	/* copy input image name if FITS file*/
	if (!strncmp(&(argv[1][strlen(argv[1])-5]),".fits",5)) {
	  sprintf(data.name,"%s",argv[1]);
	}
	else {
	  printf("  ** File must be FITS image %s\n",data.name);
	  exit(0);
	}

	/* process command line */
	for (i=2;i<argc;i++) {
	  if (!strcmp(argv[i],"-output")) {
	    flag_output=1;
	    sprintf(root,"%s",argv[i+1]);
	  }
	  if (!strcmp(argv[i],"-quiet")) flag_quiet=1;
	}
	
	/* read image */
	rd_desimage(&data,READONLY,flag_quiet);
	
	/* prepare the image name */
	for (i=0;i<strlen(argv[1]);i++) 
	  if (strncmp(".fits",argv[1]+i,5)) imagename[i]=argv[1][i];
	  else {
	    imagename[i]=0;
	    break;
	  }
	  
	/* write the components of the image */
	for (i=0;i<data.hdunum;i++) {
	  if (fits_movabs_hdu(data.fptr,i+1,&hdutype,&status)) 
	    printerror(status);
	  /* determine what kind of image extension */
	  if (fits_read_key_str(data.fptr,"DES_EXT",type,comment,&status)==KEY_NO_EXIST) {
	    status=0; /* reset status flag */
	    if (!flag_quiet) printf("  **Warning:  No DES_EXT keyword found in %s\n",data.name);
	    exit(0);
	  }
	  if (!flag_quiet) printf("  Extension type is %s:",type);
	  if (!strncmp(type,"IMAGE",5)) { /* write the image */
	    if (flag_output) sprintf(output.name,"!%s_im.fits",root);
	    else sprintf(output.name,"!%s_im.fits",imagename);
	    printf("  writing %s\n",output.name+1);
	    if (fits_create_file(&output.fptr,output.name,&status))
	    	printerror(status);
	    if (fits_copy_header(data.fptr,output.fptr,&status))
	    	printerror(status);
	    if (fits_write_img(output.fptr,TFLOAT,1,data.npixels,data.image,&status))
	      printerror(status);
	    if (fits_close_file(output.fptr,&status)) printerror(status);
	  }
	  if (!strncmp(type,"MASK",4) || !strncmp(type,"BPM",3)) { /* write the mask */
	    if (flag_output) sprintf(output.name,"!%s_bpm.fits",root);
	    else sprintf(output.name,"!%s_bpm.fits",imagename);	
	    printf("  writing %s\n",output.name+1);  
	    if (fits_create_file(&output.fptr,output.name,&status))
	    	printerror(status);
	    if (fits_copy_header(data.fptr,output.fptr,&status))
	    	printerror(status);
	    if (fits_write_img(output.fptr,TUSHORT,1,data.npixels,data.mask,&status))
	      printerror(status);
	    if (fits_close_file(output.fptr,&status)) printerror(status);
	  }
	  if (!strncmp(type,"VARIANCE",5)) { /* write the variance */
	    if (flag_output) sprintf(output.name,"!%s_var.fits",root);
	    else sprintf(output.name,"!%s_var.fits",imagename);	
	    printf("  writing %s\n",output.name+1);  
	    if (fits_create_file(&output.fptr,output.name,&status))
	    	printerror(status);
	    if (fits_copy_header(data.fptr,output.fptr,&status))
	    	printerror(status);
	    if (fits_write_img(output.fptr,TFLOAT,1,data.npixels,data.varim,&status))
	      printerror(status);
	    if (fits_close_file(output.fptr,&status)) printerror(status);
	  }
	}
	if (fits_close_file(data.fptr,&status)) printerror(status);
}
