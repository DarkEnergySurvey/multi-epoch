#include "imageproc.h"

typedef unsigned long long uint64;

int main(int argc, char *argv[])
{
   int htmdepth=20;

   fitsfile *fptr;      /* FITS file pointer, defined in fitsio.h */
   char *val, value[1000], nullstr[]="*";
   char keyword[FLEN_KEYWORD], colname[FLEN_VALUE], comment[500];
   int status = 0;   /*  CFITSIO status value MUST be initialized to zero!  */
   int hdunum, hdutype, ncols, ii, anynul, dispwidth[1000];
   int i,firstcol= 1, lastcol, linewidth, imageid,flag_ascii=0,vector_length=10;
   long jj, nrows;
   float magzp,equinox=2000.0;
   float *magaper,*magerrap,*magauto,*magerrauto,*threshold,*ximage;
   float *yimage,*aimage,*aerrimage,*bimage,*berrimage,*thetaimage;
   float *thetaerrimage,*ellipticity,*class,floatnull=0;
   float *flux_radius,*fwhm_world,*theta_j2000,*background;
   float *mag_prof,*xprof_image,*yprof_image,*chi2_prof,*vector_prof;
   long *xminimage,*yminimage,*xmaximage,*ymaximage,*objnum;
   short *fflags,*niter_prof,shortnull=0;
   int **pointer,**nullpointer, j,k,*typecode,intnull=0;
   char **prstring;
   long *repeat,*width,longnull=0;
   double doublenull=0.0,*alphara,*deltadec,*alphapeak,*deltapeak,*x2world;
   double *x2errworld,*y2world,*y2errworld,*xyworld,*xyerrworld,*x2image;
   double *x2errimage,*y2image,*y2errimage,*xyimage,*xyerrimage;
   char band[5], command[1000],line[1000];
   double cx,cy,cz;
   void getxyz();
   uint64 cc_radec2ID();
   uint64 htmID;
   
   if(argc < 2)
   {
       printf("Usage: %s <#_cat.fits> <imageid (from Files table)> <band> <equinox>\n",argv[0]);
       exit(0);
   }
   
   imageid = atoi(argv[2]);
   sprintf(band, "%s", argv[3]);
   sscanf(argv[4],"%f",&equinox);

   magzp=25.0; /* set as default, make input from command line later? */ 

   /* begin conversion */

   status=0;
   if (!fits_open_file(&fptr, argv[1], READONLY, &status))  {
   
     /* OLD version */
     
     //fits_read_key_flt(fptr,"SEXMGZPT",&magzp,comment,&status);
     //fits_get_hdu_num(fptr, &hdunum);
     
     //if(hdunum == 1) {
     /* This is the primary array;  try to move to the */
     /* first extension and see if it is a table */
     //fits_movabs_hdu(fptr, 2, &hdutype, &status);
     //}
     //else
     //fits_get_hdu_type(fptr, &hdutype, &status); /* Get the HDU type */
       
     //if (hdutype == IMAGE_HDU)
     //printf("Error: this program only displays tables, not images\n");
     //else  
     //{
     //fits_get_num_rows(fptr, &nrows, &status);
     //fits_get_num_cols(fptr, &ncols, &status);
	 /* print each column, row by row (there are faster ways to do this) */
	 //val = value;
     //}	 
     /* END old version */
     
     status=0;
     fits_get_num_hdus(fptr,&hdunum,&status);
     if (status) fits_report_error(stderr, status);

     switch(hdunum) {
     case 2: fits_movabs_hdu(fptr, 2, &hdutype, &status); fits_get_hdu_type(fptr, &hdutype, &status); break;
     case 3: fits_movabs_hdu(fptr, 3, &hdutype, &status); fits_get_hdu_type(fptr, &hdutype, &status); break;
     default: printf("Input file not in FITS_1.0 or FITS_LDAC format, check file\n");
     }
     
     if(hdutype == IMAGE_HDU)
       printf("Error: this program only displays tables, not images\n");
     else {
       fits_get_num_rows(fptr, &nrows, &status);
       fits_get_num_cols(fptr, &ncols, &status);
       /* print each column, row by row (there are faster ways to do this) */
       val = value;
     }


     /* new lines from Joe's code */
     
     repeat=(long *)calloc(ncols,sizeof(long));
     width=(long *)calloc(ncols,sizeof(long));
     typecode=(int *)calloc(ncols,sizeof(int));
     pointer=(int *)calloc(ncols,sizeof(int *));
     nullpointer=(int *)calloc(ncols,sizeof(int *));
     prstring=(char **)calloc(ncols,sizeof(char *));
      
 
     for (i=0;i<ncols;i++) {
       if (fits_get_coltype(fptr,i+1,&(typecode[i]),
			    &(repeat[i]),&(width[i]),&status));
       nullpointer[i]=(float *)&floatnull;
     }
     
     nullpointer[5]=nullpointer[6]=(double *)&doublenull;
     nullpointer[7]=nullpointer[8]=(double *)&doublenull;
     nullpointer[9]=nullpointer[10]=(double *)&doublenull;
     nullpointer[11]=nullpointer[12]=(double *)&doublenull;
     nullpointer[13]=nullpointer[14]=(double *)&doublenull;
     nullpointer[22]=nullpointer[23]=(double *)&doublenull;
     nullpointer[24]=nullpointer[25]=(double *)&doublenull;
     nullpointer[26]=nullpointer[27]=(double *)&doublenull;
     /* addition */
     nullpointer[37]=nullpointer[38]=(double *)&doublenull;
     nullpointer[39]=nullpointer[40]=(double *)&doublenull;
     nullpointer[41]=nullpointer[42]=(double *)&doublenull;
     nullpointer[43]=nullpointer[45]=(double *)&doublenull;
     /* end addition */
     
     nullpointer[0]=(long *)&longnull;
     nullpointer[18]=(long *)&longnull;
     nullpointer[19]=(long *)&longnull;
     nullpointer[20]=(long *)&longnull;
     nullpointer[21]=(long *)&longnull;
     nullpointer[36]=(short *)&shortnull;
     nullpointer[44]=(short *)&shortnull;

     /* now prepare the data vectors for each column; multi-dimensional first */
     
     magaper=(float *)calloc(nrows*6,sizeof(float));
     pointer[3]=(float *)magaper;
     
     magerrap=(float *)calloc(nrows*6,sizeof(float));
     pointer[4]=(float *)magerrap;

     /* prepare data vectors for single entry */
     objnum=(long *)calloc(nrows,sizeof(long));
     pointer[0]=(long *)objnum;
     
     magauto=(float *)calloc(nrows,sizeof(float));
     pointer[1]=(float *)magauto;
       
     magerrauto=(float *)calloc(nrows,sizeof(float));
     pointer[2]=(float *)magerrauto;
       
     alphara=(double *)calloc(nrows,sizeof(double));
     pointer[5]=(double *)alphara;
     
     deltadec=(double *)calloc(nrows,sizeof(double));
     pointer[6]=(double *)deltadec;
       
     alphapeak=(double *)calloc(nrows,sizeof(double));
     pointer[7]=(double *)alphapeak;
     
     deltapeak=(double *)calloc(nrows,sizeof(double));
     pointer[8]=(double *)deltapeak;

     x2world=(double *)calloc(nrows,sizeof(double));
     pointer[9]=(double *)x2world;
 
     x2errworld=(double *)calloc(nrows,sizeof(double));
     pointer[10]=(double *)x2errworld;

     y2world=(double *)calloc(nrows,sizeof(double));
     pointer[11]=(double *)y2world;
     
     y2errworld=(double *)calloc(nrows,sizeof(double));
     pointer[12]=(double *)y2errworld;

     xyworld=(double *)calloc(nrows,sizeof(double));
     pointer[13]=(double *)xyworld;

     xyerrworld=(double *)calloc(nrows,sizeof(double));
     pointer[14]=(double *)xyerrworld;

     threshold=(float *)calloc(nrows,sizeof(float));
     pointer[15]=(float *)threshold;

     ximage=(float *)calloc(nrows,sizeof(float));
     pointer[16]=(float *)ximage;

     yimage=(float *)calloc(nrows,sizeof(float));
     pointer[17]=(float *)yimage;

     xminimage=(long *)calloc(nrows,sizeof(long));
     pointer[18]=(long *)xminimage;

     yminimage=(long *)calloc(nrows,sizeof(long));
     pointer[19]=(long *)yminimage;
     
     xmaximage=(long *)calloc(nrows,sizeof(long));
     pointer[20]=(long *)xmaximage;

     ymaximage=(long *)calloc(nrows,sizeof(long));
     pointer[21]=(long *)ymaximage;

     x2image=(double *)calloc(nrows,sizeof(double));
     pointer[22]=(double *)x2image;

     x2errimage=(double *)calloc(nrows,sizeof(double));
     pointer[23]=(double *)x2errimage;

     y2image=(double *)calloc(nrows,sizeof(double));
     pointer[24]=(double *)y2image;

     y2errimage=(double *)calloc(nrows,sizeof(double));
     pointer[25]=(double *)y2errimage;
     
     xyimage=(double *)calloc(nrows,sizeof(double));
     pointer[26]=(double *)xyimage;

     xyerrimage=(double *)calloc(nrows,sizeof(double));
     pointer[27]=(double *)xyerrimage;

     aimage=(float *)calloc(nrows,sizeof(float));
     pointer[28]=(float *)aimage;

     aerrimage=(float *)calloc(nrows,sizeof(float));
     pointer[29]=(float *)aerrimage;

     bimage=(float *)calloc(nrows,sizeof(float));
     pointer[30]=(float *)bimage;
     
     berrimage=(float *)calloc(nrows,sizeof(float));
     pointer[31]=(float *)berrimage;
       
     thetaimage=(float *)calloc(nrows,sizeof(float));
     pointer[32]=(float *)thetaimage;

     thetaerrimage=(float *)calloc(nrows,sizeof(float));
     pointer[33]=(float *)thetaerrimage;

     ellipticity=(float *)calloc(nrows,sizeof(float));
     pointer[34]=(float *)ellipticity;

     class=(float *)calloc(nrows,sizeof(float));
     pointer[35]=(float *)class;

     fflags=(short *)calloc(nrows,sizeof(short));
     pointer[36]=(short *)fflags;

     /* addition */
     flux_radius=(float *)calloc(nrows,sizeof(float));
     pointer[37]=(float *)flux_radius;
     
     fwhm_world=(float *)calloc(nrows,sizeof(float));
     pointer[38]=(float *)fwhm_world;

     theta_j2000=(float *)calloc(nrows,sizeof(float));
     pointer[39]=(float *)theta_j2000;

     background=(float *)calloc(nrows,sizeof(float));
     pointer[40]=(float *)background;

     mag_prof=(float *)calloc(nrows,sizeof(float));
     pointer[41]=(float *)mag_prof;

     xprof_image=(float *)calloc(nrows,sizeof(float));
     pointer[42]=(float *)xprof_image;

     yprof_image=(float *)calloc(nrows,sizeof(float));
     pointer[43]=(float *)yprof_image;

     niter_prof=(short *)calloc(nrows,sizeof(short));
     pointer[44]=(short *)niter_prof;

     chi2_prof=(float *)calloc(nrows,sizeof(float));
     pointer[45]=(float *)chi2_prof;

     vector_prof=(float *)calloc(nrows*vector_length,sizeof(float));
     pointer[46]=(float *)vector_prof;
     /* end addition */

     /* this looks good to me */
     for (i=0;i<ncols;i++) {
       if (fits_read_col(fptr,typecode[i],
			 i+1,1,1,nrows*repeat[i],nullpointer[i],
			 pointer[i],&anynul,&status));
     }
   }    
   fits_close_file(fptr, &status);

   
   /* Output the column values */

   for (j=0;j<nrows;j++) {
     
     if(magerrauto[j] > 999.99)
       magerrauto[j] = 999.99;
     
              
     printf("%2.1f|%s|%2d|%2.1f|0.0|%10d|%8.4f|%8.4f|",equinox,band,imageid,magzp,objnum[j],magauto[j],magerrauto[j]);
     for (k=0;k<6;k++) {
       if(magerrap[j*6+k] > 999.99)
	 magerrap[j*6+k] = 999.99;
       printf("%8.4f|%8.4f|",magaper[j*6+k],magerrap[j*6+k]);
     }
     printf("%11.7f|%11.7f|%11.7f|%11.7f|",alphara[j],deltadec[j],alphapeak[j],deltapeak[j]);
     printf("%19.13e|%19.13e|%19.13e|%19.13e|%19.13e|",x2world[j],x2errworld[j],y2world[j],y2errworld[j],xyworld[j]);
     printf("%19.13e|%12g|%10.3f|%10.3f|",xyerrworld[j],threshold[j],ximage[j],yimage[j]);
     printf("%10d|%10d|%10d|%10d|",xminimage[j],yminimage[j],xmaximage[j],ymaximage[j]);
     //if(y2image[j] > 1e7)
     //y2image[j] = 0.0;
     printf("%19.13e|%19.13e|%19.13e|%19.13e|",x2image[j],x2errimage[j],y2image[j],y2errimage[j]);
     printf("%19.13e|%19.13e|%9.3f|%8.4f|",xyimage[j],xyerrimage[j],aimage[j],aerrimage[j]);
     printf("%9.3f|%8.4f|%5.1f|%5.1f|",bimage[j],berrimage[j],thetaimage[j],thetaerrimage[j]);
     printf("%8.3f|%5.2f|%d|",ellipticity[j],class[j],fflags[j]);

     /* get htmID */
     htmID = cc_radec2ID(alphara[j], deltadec[j], htmdepth);  
     printf("%16Ld|",htmID);
     /* get cx,cy,cz */
     getxyz(alphara[j],deltadec[j],&cx,&cy,&cz);
     printf("%11.6f|%11.6f|%11.6f",cx,cy,cz);

     /* output for testing */
     printf("|%8.4f|%10.3f|%10.3f|%d|%3.6f",mag_prof[j],xprof_image[j],yprof_image[j],niter_prof[j],chi2_prof[j]);
     for (k=0;k<vector_length;k++) 
       printf("|%8.4f",vector_prof[j*vector_length+k]);
     
     printf("\n");
   }

   if (status) fits_report_error(stderr, status); /* print any error message */

   return 0;
}
 

