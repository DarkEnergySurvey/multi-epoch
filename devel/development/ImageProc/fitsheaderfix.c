/* Combine bias frames into a composite */
#include "imageproc.h"

desimage *data,datain,output;

main(argc,argv)
	int argc;
	char *argv[];
{

	int	status=0,j,len;
	void	printerror();
	char	value[100],param[15],incoming[200],oldvalue[100],
		comment[100],oldcomment[100];
	fitsfile *fptr;

	if (argc<3) {
	  printf("fitsheaderfix <image> <HEADERPARAM=VALUE> <COMMENT>\n");
	  exit(0);
	}
	
	/* confirm that we have a fits image or a gzipped fits image */
	len=strlen(argv[1])-1;
	for (j=len;j>=0;j--) 
 	  if (!strncmp(&(argv[1][j]),".fit",4)) break;
	if (j==-1) {
	  printf("  ** fitsheaderfix requires a FITS image on input\n");
	  exit(0);
	}

	sprintf(incoming,"%s",argv[2]);
	if (argc==4) sprintf(comment,"%s",argv[3]);
	else comment[0]=0;


	/* parse the param=value */
	/* assume it is string for now */
	len=strlen(incoming);
	for (j=0;j<len;j++) if (!strncmp(incoming+j,"=",1)) break;
	if (j==len) {
	  printf("  ** Require PARAM=VALUE form: %s\n",incoming);
	  exit(0);	
	}
	incoming[j]=32;
	sscanf(incoming,"%s %s",param,value);

	/* open the image */
	if (fits_open_file(&fptr,argv[1],READWRITE,&status)) 
	  printerror(status);
	/* first read parameter and comment */
	if (fits_read_key_str(fptr,param,oldvalue,oldcomment,&status)==KEY_NO_EXIST) {
	  status=0;
	  printf("  Param %s not found.  Adding it\n",param);
	  if (fits_write_key_str(fptr,param,value,comment,&status)) 
	    printerror(status);
	}
	else {
	  /* no new comment included */
	  if (argc==3) sprintf(comment,"%s",oldcomment);
	  /* update the keyword value */
	  if (fits_update_key_str(fptr,param,value,comment,&status)) 
	    printerror(status);
	}
	/* close the file */
	if (fits_close_file(fptr,&status)) printerror(status);
	
}
