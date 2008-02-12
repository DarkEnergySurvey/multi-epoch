#include "imageproc.h"

main(int argc, char *argv [])
{
  char image[1000],head[1000],comments[100],temp[100];
  char variable[10],value[20],*instr;
  double tempvar;
  float equx;
  int  status=0,len,i,j,k,flag,hdunum;
  void printerror();

  FILE *fhead;
  fitsfile *fptr;

  if (argc<2) 
    {
      printf("Usage: %s <image.fits> <image.head>\n", argv[0]);
      exit(0);
    }

  sprintf(image, "%s", argv[1]);
  sprintf(head, "%s", argv[2]);
  
  /* open both files */
  fhead=fopen(head,"r");
  if(fits_open_file(&fptr,image,READWRITE,&status))
    printerror(status);
    
  if (fits_get_num_hdus(fptr,&hdunum,&status)) 
    printerror(status);

  while(fgets(temp,1000,fhead) != NULL) {
    /* initialize */
    flag=0;
    variable[0]=0;
    value[0]=0;
    tempvar=0.0;

    /* format the input string */
    len=strlen(temp);
    for (j=len;j>0;j--) {
      if(!strncmp(&(temp[j]),"=",1)) { 
	temp[j]=32;
	flag=1;
      }
      if(!strncmp(&(temp[j]),"/",1))
	temp[j]=32;
    }

    /* Update keywords here */
    if(flag) {
      sscanf(temp,"%s %s",variable,value);
      
      if (!strcmp(variable,"EQUINOX")) {
	sscanf(value,"%f",&equx);
	if(fits_update_key(fptr,TFLOAT,"EQUINOX",&equx,"Equinox",&status))
	  printerror(status);
      }

      if (!strcmp(variable,"RADECSYS")) {
	instr=strtok(value,"'");
	if(fits_update_key(fptr,TSTRING,"RADECSYS",instr,"Astrometric system",&status))
	  printerror(status);
      }

      if (!strcmp(variable,"CTYPE1")) {
	instr=strtok(value,"'");
	if(fits_update_key(fptr,TSTRING,"CTYPE1",instr,"WCS projection type for this axis",&status))
	  printerror(status);
      }

      if (!strcmp(variable,"CTYPE2")) {
	instr=strtok(value,"'");
	if(fits_update_key(fptr,TSTRING,"CTYPE2",instr,"WCS projection type for this axis",&status))
	  printerror(status);
      }

      if (!strcmp(variable,"CUNIT1")) {
	instr=strtok(value,"'");
	if(fits_update_key(fptr,TSTRING,"CUNIT1",instr,"Axis unit",&status))
	  printerror(status);
      }

      if (!strcmp(variable,"CUNIT2")) {
	instr=strtok(value,"'");
	if(fits_update_key(fptr,TSTRING,"CUNIT2",instr,"Axis unit",&status))
	  printerror(status);
      }

      if (!strcmp(variable,"CRVAL1")) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,"CRVAL1",tempvar,12,"World coordinate on this axis",&status))
	  printerror(status);
      }
      
      if (!strcmp(variable,"CRVAL2")) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,"CRVAL2",tempvar,12,"World coordinate on this axis",&status))
	  printerror(status);
      }
     
      if (!strcmp(variable,"CRPIX1")) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,"CRPIX1",tempvar,12,"Reference pixel on this axis",&status))
	  printerror(status);
      }
      
      if (!strcmp(variable,"CRPIX2")) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,"CRPIX2",tempvar,12,"Reference pixel on this axis",&status))
	  printerror(status);
      }

      if (!strcmp(variable,"CD1_1")) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,"CD1_1",tempvar,12,"Linear projection matrix",&status))
	  printerror(status);
      }
      
      if (!strcmp(variable,"CD1_2")) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,"CD1_2",tempvar,12,"Linear projection matrix",&status))
	  printerror(status);
      }
     
      if (!strcmp(variable,"CD2_1")) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,"CD2_1",tempvar,12,"Linear projection matrix",&status))
	  printerror(status);
      }

      if (!strcmp(variable,"CD2_2")) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,"CD2_2",tempvar,12,"Linear projection matrix",&status))
	  printerror(status);
      }
      
      if (!strncmp(variable,"PV",2)) {
	sscanf(value,"%lg",&tempvar);
	if(fits_update_key_dbl(fptr,variable,tempvar,12,"Projection distortion parameter",&status))
	  printerror(status);
      }

    } /* if flag loop */
  } /* while loop */
 
  /* close both files */
  if(fits_close_file(fptr,&status)) printerror(status);
  fclose(fhead);

  return(0);
}


