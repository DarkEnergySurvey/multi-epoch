#include "imageproc.h"

main(argc,argv)
     int argc;
     char *argv[];
{
  char filename_in[700], filename_out[700];
  char rakeyword[40],deckeyword[40];
  char comment[73];
  float ra_in, dec_in;
  int	status=0;

  fitsfile *fptr;
  
  sprintf(filename_in, "%s", argv[2]);
  
  fits_open_file(&fptr,filename_in,READONLY,&status);
  
  if (fits_read_key_str(fptr,"RA",rakeyword,comment,&status)==KEY_NO_EXIST){
    printf("\n  **Warning:  RA not found  %s\n",rakeyword);    
  }
  else printf("%s\n", rakeyword);

  fits_close_file(fptr,&status);

}
