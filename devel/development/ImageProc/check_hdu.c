/*

This program check the number of HDU for given input fits file.

*/

#include "imageproc.h"

main(int argc, char *argv [])
{
  char image[1000];
  int  status=0, hdunum=0;

  fitsfile *fptr;

  if (argc<2) 
    {
      printf("Usage: %s <image.fits> \n", argv[0]);
      exit(0);
    }

  printf(" -- Checking number of extension (HDU) for the input FITS file\n");

  sprintf(image, "%s", argv[1]);
  fits_open_file(&fptr,image,READONLY,&status);
  
  if(fits_get_num_hdus(fptr,&hdunum,&status)) 
    {
      printf("  **** FITS Error ****\n");
      fits_report_error(stderr,status);
      fits_close_file(fptr,&status);
      exit(0);
    }
  else
    printf(" -- Image %s has %d HDU\n", image, hdunum);

  fits_close_file(fptr,&status);
}



