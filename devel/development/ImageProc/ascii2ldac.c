#include "fitsio.h"

main(int argc, char *argv[])

{

  FILE *asciifile;
  int counter = 0;
  int c, i;
  double *txworld, *tyworld; 
  float *terraworld, *terrbworld, *tmag, *tmagerr;
 

  fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
  int status, hdutype;
  
  char filename[500];           /*  name for new FITS file */
  char extname[] = "LDAC_IMHEAD";           /* extension name */
  char extname2[] = "LDAC_OBJECTS";
  long firstrow, firstelem;
  int tfields   = 1;       /* table will have 1 columns */
  long nrows    = 1;       /* table will have 1 rows    */
  int tfields2  = 6;      /* 2nd table will have 6 columns */
  long nrows2;

  

  /* see if there are the correct number of command line arguments */

  if(argc!=3) {
    printf("Usage: asci2ldac <ASCII_Catalog> <LDAC_Catalog>\n");
    exit(1);
    } 

  sprintf(filename,"!%s",argv[2]); /* name of new FITS file */

  /* open file for determining the number of rows for binary table */
   if ((asciifile = fopen(argv[1], "r"))==NULL){
    printf("Error opening file\n");
    exit(1);
    }
   /* get the number of rows in the data file */
   while ((c = getc(asciifile)) != EOF) {
	if (c == '\n')
	    counter++;
    }
   fclose(asciifile);
   
   txworld = (double *) calloc(counter,sizeof(double));
   tyworld = (double *) calloc(counter,sizeof(double));
   terraworld = (float *) calloc(counter,sizeof(float));
   terrbworld = (float *) calloc(counter,sizeof(float));
   tmag = (float *) calloc(counter,sizeof(float));
   tmagerr = (float *) calloc(counter,sizeof(float));
   nrows2 = counter;



  /* define the name, datatype and unit for the first table */
  char *ttype[] = { "Field Header Card"};
  char *tform[] = { "1680A"};
  char *tunit[] = {""};

  /* define the name, datatype, and units for the second table */
  char *ttype2[] = {"X_WORLD", "Y_WORLD", "ERRA_WORLD", "ERRB_WORLD", "MAG", "MAGERR" };
  char *tform2[] = { "1D", "1D", "1E", "1E", "1E", "1E" };
  char *tunit2[] = {"deg", "deg", "deg", "deg", "mag", "mag"};


  /* define the name and value of the first column in table 1 */
  char *fieldname[] = {"SIMPLE  =                    T / This is a FITS file                              BITPIX  =                    0 /"};
 
   status = 0;         /* initialize status before calling fitsio routines */

  if (fits_create_file(&fptr,filename, &status)) /* create new FITS file */
         printerror( status );  /* call printerror if error occurs */

  if (fits_create_tbl(fptr,BINARY_TBL, nrows, tfields, ttype, tform,tunit, extname, &status) )

         printerror( status );

    firstrow  = 1;  /* first row in table to write   */
    firstelem = 1;  /* first element in row  (ignored in ASCII tables) */

    /* write names to the first column (character strings) */

      fits_write_col(fptr, TSTRING, 1, firstrow, firstelem, nrows, fieldname,
	  &status);

    if (fits_create_tbl(fptr,BINARY_TBL,nrows2, tfields2, ttype2, tform2,tunit2, extname2, &status) )
         printerror( status );


      /* open ascii file for reading of USNO-B1 data */
   if ((asciifile = fopen(argv[1], "r"))==NULL){
    printf("Error opening file\n");
    exit(1);
    }
    
    firstrow  = 1;  /* first row in table to write   */
    firstelem = 1;  /* first element in row  (ignored in ASCII tables) */

    i = 0;
   while (i<counter) { 
     fscanf(asciifile,"%lf %lf %f %f %f ",&txworld[i],&tyworld[i],&terraworld[i],&terrbworld[i],&tmag[i]);
     
     tmagerr[i]=0.40;
     terraworld[i]*=0.001/3600.0;
     terrbworld[i]*=0.001/3600.0;
    
     i++;
   }
   
   fclose(asciifile);
   i = 0;

   while (i<counter){
       
     fits_write_col(fptr, TDOUBLE,1, firstrow, firstelem, 1, &txworld[i],&status);
     fits_write_col(fptr, TDOUBLE,2, firstrow, firstelem, 1, &tyworld[i],&status);
     fits_write_col(fptr, TFLOAT, 3, firstrow, firstelem, 1, &terraworld[i],&status);
     fits_write_col(fptr, TFLOAT, 4, firstrow, firstelem, 1, &terrbworld[i],&status);
     fits_write_col(fptr, TFLOAT, 5, firstrow, firstelem, 1, &tmag[i],&status);
     fits_write_col(fptr, TFLOAT, 6, firstrow, firstelem, 1, &tmagerr[i],&status); 
     i++; 
      firstrow++;
   }
   
   if ( fits_close_file(fptr, &status) )           /* close the file */
     printerror( status );           
   
   return 0;
}



/*--------------------------------------------------------------------------*/
printerror( int status)
{
    /*****************************************************/
    /* Print out cfitsio error messages and exit program */
    /*****************************************************/


    if (status)
    {
       fits_report_error(stderr, status); /* print error report */

       exit( status );    /* terminate the program, returning error status */
    }
    return;
}

