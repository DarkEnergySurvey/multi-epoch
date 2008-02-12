/*

This program check the values of given keyword from a given input fits file.

*/

#include "imageproc.h"

main(int argc, char *argv [])
{
  char image[1000],keyname[100],comment[1000],value[400];
  int  status=0, hdunum=0;

  fitsfile *fptr;

  if (argc<2) 
    {
      printf("Usage: %s <image.fits> <keyword_name>\n", argv[0]);
      exit(0);
    }

  sprintf(image, "%s", argv[1]);
  sprintf(keyname, "%s", argv[2]);

  fits_open_file(&fptr,image,READONLY,&status);
  
  if(fits_read_keyword(fptr,keyname,value,comment,&status)) 
    {
      printf("  **** FITS Error ****\n");
      fits_report_error(stderr,status);
      fits_close_file(fptr,&status);
      exit(0);
    }
  else
    printf("%s = %s\n", keyname, value);

  fits_close_file(fptr,&status);
}



