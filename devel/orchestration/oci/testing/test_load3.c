#include "imageproc.h"
#include "desw_rel.h"
#include <sqlca.h>

#define LINK "dber.ncsa.uiuc.edu/sony"
#define USER "sony_admin"
#define PASS "sonymgr"





typedef unsigned long long uint64;

int htmdepth=20;

fitsfile *fptr;      /* FITS file pointer, defined in fitsio.h */
char *val, value[1000], nullstr[]="*";
char keyword[FLEN_KEYWORD], colname[FLEN_VALUE], comment[500];
int status = 0;   /*  CFITSIO status value MUST be initialized to zero!  */
int hdunum, hdutype, ncols, ii, anynul, dispwidth[1000], parentid, softid;
int i,firstcol= 1, lastcol, linewidth, imageid,flag_ascii=0;
long jj, nrows;
float magzp,equinox=2000.0;
float *magaper,*magerrap,*magauto,*magerrauto,*threshold,*ximage;
float *yimage,*aimage,*aerrimage,*bimage,*berrimage,*thetaimage;
float *thetaerrimage,*ellipticity,*class,floatnull=0;
int *objnum,*xminimage,*yminimage,*xmaximage,*ymaximage;
short *fflags,shortnull=0;
int **pointer,**nullpointer, j,k,*typecode,intnull=0;
char **prstring;
long *repeat,*width,longnull=0;
double doublenull=0.0,*alphara,*deltadec,*alphapeak,*deltapeak,*x2world;
double *x2errworld,*y2world,*y2errworld,*xyworld,*xyerrworld,*x2image;
double *x2errimage,*y2image,*y2errimage,*xyimage,*xyerrimage;
char band[5], command[1000],line[1000];
//double cx,cy,cz;
double *cx,*cy,*cz;


void getxyz(), set_piece(), insert_objects();
uint64 cc_radec2ID();



//uint64 htmID;
uint64 *htmID;



/* for ascii input */
FILE  *pip,*fin;
int objnum_ascii;
double magauto_ascii, magerrauto_ascii;
double magaper1_ascii, magerrap1_ascii, magaper2_ascii, magerrap2_ascii, magaper3_ascii, magerrap3_ascii, magaper4_ascii, magerrap4_ascii, magaper5_ascii, magerrap5_ascii, magaper6_ascii, magerrap6_ascii;
double alphara_ascii, deltadec_ascii, alphapeak_ascii, deltapeak_ascii;
double x2world_ascii, x2errworld_ascii, y2world_ascii, y2errworld_ascii, xyworld_ascii;
double xyerrworld_ascii, threshold_ascii, ximage_ascii, yimage_ascii;
int xminimage_ascii, yminimage_ascii, xmaximage_ascii, ymaximage_ascii;
double x2image_ascii, x2errimage_ascii, y2image_ascii, y2errimage_ascii;
double xyimage_ascii, xyerrimage_ascii, aimage_ascii, aerrimage_ascii;
double bimage_ascii, berrimage_ascii, thetaimage_ascii, thetaerrimage_ascii;
double ellipticity_ascii, class_ascii;
int fflags_ascii;

int main(int argc, char *argv[])
{

char instStmt[5000];

   if(argc < 2)
   {
       printf("Usage: %s <#_cat.fits> <imageid (from Files table)> <band> <equinox>\n",argv[0]);
       printf("          -ascii (if the catalog is in ascii) \n",argv[0]);
       exit(0);
   }

   imageid = atoi(argv[2]);
   sprintf(band, "%s", argv[3]);
   sscanf(argv[4],"%f",&equinox);

   for (i=4;i<argc;i++) {
     if (!strcmp(argv[i],"-ascii")) flag_ascii=1;
   }




desw_ctx ctx;
r_set rs1;

initialize(&ctx, (text *)USER, (text *)PASS, (text *)LINK);

//cleanup(&ctx);




   if(!flag_ascii) {

     if (!fits_open_file(&fptr, argv[1], READONLY, &status))  {

       fits_read_key_flt(fptr,"SEXMGZPT",&magzp,comment,&status);

       fits_get_hdu_num(fptr, &hdunum);

       if(hdunum == 1) {
         /* This is the primary array;  try to move to the */
         /* first extension and see if it is a table */
         fits_movabs_hdu(fptr, 2, &hdutype, &status);
       }
       else
         fits_get_hdu_type(fptr, &hdutype, &status); /* Get the HDU type */

       if (hdutype == IMAGE_HDU)
         printf("Error: this program only displays tables, not images\n");
       else
         {
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

       nullpointer[0]=(int *)&intnull;
       nullpointer[18]=(int *)&intnull;
       nullpointer[19]=(int *)&intnull;
       nullpointer[20]=(int *)&intnull;
       nullpointer[21]=(int *)&intnull;
       nullpointer[36]=(short *)&shortnull;

       /* now prepare the data vectors for each column; multi-dimensional first */

       magaper=(float *)calloc(nrows*6,sizeof(float));
       pointer[3]=(float *)magaper;

       magerrap=(float *)calloc(nrows*6,sizeof(float));
       pointer[4]=(float *)magerrap;

       /* prepare data vectors for single entry */
       objnum=(int *)calloc(nrows,sizeof(int));
       pointer[0]=(int *)objnum;

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

       xminimage=(int *)calloc(nrows,sizeof(int));
       pointer[18]=(int *)xminimage;

       yminimage=(int *)calloc(nrows,sizeof(int));
       pointer[19]=(int *)yminimage;

       xmaximage=(int *)calloc(nrows,sizeof(int));
       pointer[20]=(int *)xmaximage;

       ymaximage=(int *)calloc(nrows,sizeof(int));
       pointer[21]=(int *)ymaximage;

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



htmID=(uint64 *)calloc(nrows,sizeof(uint64));

cx=(double *)calloc(nrows,sizeof(double));

cy=(double *)calloc(nrows,sizeof(double));

cz=(double *)calloc(nrows,sizeof(double));


       /* this looks good to me */
       for (i=0;i<ncols;i++) {

         if (fits_read_col(fptr,typecode[i],
                           i+1,1,1,nrows*repeat[i],nullpointer[i],
                           pointer[i],&anynul,&status));
       }
     }

     fits_close_file(fptr, &status);

     /* Output the column values */
printf( "ROW COUTN: %d\n", nrows );

     for (j=0;j<nrows;j++) {
//     for (j=0;j<2;j++) {

       if(magerrauto[j] > 999.99)
         magerrauto[j] = 999.99;

//printf( "J: %d\n", j );
/**
       printf("%2.1f|%s|%2d|%2.1f|0.0|%10d|%8.4f|%8.4f|",equinox,band,imageid,magzp,objnum[j],magauto[j],magerrauto[j]);
       for (k=0;k<6;k++) {
         if(magerrap[j*6+k] > 999.99)
           magerrap[j*6+k] = 999.99;
         printf("%8.4f|%8.4f|", magaper[j*6+k],magerrap[j*6+k]);
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
**/
       /* get htmID */
       htmID[j] = cc_radec2ID(alphara[j], deltadec[j], htmdepth);
//       printf("%16Ld|",htmID[j]);
       printf("%16Ld\n",htmID[j]);
       /* get cx,cy,cz */
       getxyz(alphara[j],deltadec[j],&cx[j],&cy[j],&cz[j]);
//       printf("%11.6f|%11.6f|%11.6f\n",cx[j],cy[j],cz[j]);







// WORKING ONE-4-ONE

//instStmt[0] = '\0';
//sprintf( instStmt, "INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval, %2.1f, '%s', %16Ld, %11.6f, %11.6f, %11.6f, 0 , 0, %2d, %2.1f, NULL, NULL, %10d, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %11.7f, %11.7f, %11.7f, %11.7f, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %12g, %10.3f, %10.3f, %10d, %10d, %10d, %10d, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %9.3f, %8.4f, %9.3f, %8.4f, %5.1f, %5.1f, %8.3f, %5.2f, %d)", equinox,band,htmID[j],cx[j],cy[j],cz[j],imageid,magzp,objnum[j],magauto[j],magerrauto[j],magaper[j*6],magerrap[j*6],magaper[j*6+1],magerrap[j*6+1],magaper[j*6+2],magerrap[j*6+2],magaper[j*6+3],magerrap[j*6+3],magaper[j*6+4],magerrap[j*6+4],magaper[j*6+5],magerrap[j*6+5],alphara[j],deltadec[j],alphapeak[j],deltapeak[j],x2world[j],x2errworld[j],y2world[j],y2errworld[j],xyworld[j],xyerrworld[j],threshold[j],ximage[j],yimage[j],xminimage[j],yminimage[j],xmaximage[j],ymaximage[j],x2image[j],x2errimage[j],y2image[j],y2errimage[j],xyimage[j],xyerrimage[j],aimage[j],aerrimage[j],bimage[j],berrimage[j],thetaimage[j],thetaerrimage[j],ellipticity[j],class[j],fflags[j] );


//printf( "%s\n", instStmt );

//printf( "INSERT: %d\n", j );
//sql_stmt_execute(&ctx, (text *)instStmt );










//       printf("equinox 2.1f %2.1f|band %s|imageid 2d %2d|magzp 2.1f %2.1f|0.0|objnum 10d %10d|magauto 8.4f %8.4f|magerrauto 8.4f %8.4f\n",equinox,band,imageid,magzp,objnum[j],magauto[j],magerrauto[j]);
//       for (k=0;k<6;k++) {
//         if(magerrap[j*6+k] > 999.99)
//           magerrap[j*6+k] = 999.99;
//         printf("magaper 8.4f [%d*6+%d] %8.4f|magerrap 8.4f %8.4f\n",j, k, magaper[j*6+k],magerrap[j*6+k]);
//       }
//       printf("alphara 11.7f %11.7f|deltadec 11.7f %11.7f|alphapeak 11.7f %11.7f|deltapeak 11.7f %11.7f\n",alphara[j],deltadec[j],alphapeak[j],deltapeak[j]);
//       printf("x2world 19.13e %19.13e|x2errworld  19.13e %19.13e|y2world 19.13e %19.13e|y2errworld 19.13e %19.13e|xyworld 19.13e %19.13e\n",x2world[j],x2errworld[j],y2world[j],y2errworld[j],xyworld[j]);
//       printf("xyerrworld 19.13e %19.13e|threshold 12g %12g|ximage 10.3f %10.3f|yimage 10.3f %10.3f\n",xyerrworld[j],threshold[j],ximage[j],yimage[j]);
//       printf("xminimage 10d %10d|yminimage 10d %10d|xmaximage 10d %10d|ymaximage 10d %10d\n",xminimage[j],yminimage[j],xmaximage[j],ymaximage[j]);
       //if(y2image[j] > 1e7)
       //y2image[j] = 0.0;
//       printf("x2image 19.13e %19.13e|x2errimage 19.13e %19.13e|y2image 19.13e %19.13e|y2errimage 19.13e %19.13e\n",x2image[j],x2errimage[j],y2image[j],y2errimage[j]);
//       printf("xyimage 19.13e  %19.13e|xyerrimage 19.13e %19.13e|aimage 9.3f %9.3f|aerrimage 8.4f %8.4f\n",xyimage[j],xyerrimage[j],aimage[j],aerrimage[j]);
//       printf("bimage 9.3f %9.3f|berrimage 8.4f %8.4f|thetaimage 5.1f %5.1f|thetaerrimage 5.1f %5.1f\n",bimage[j],berrimage[j],thetaimage[j],thetaerrimage[j]);
//       printf("ellipticity 8.3f %8.3f|class 5.2f %5.2f|fflags d %d\n",ellipticity[j],class[j],fflags[j]);
       /* get htmID */
       //htmID = cc_radec2ID(alphara[j], deltadec[j], htmdepth);
       //printf("htmID 16Ld %16Ld\n",htmID);
//       htmID[j] = cc_radec2ID(alphara[j], deltadec[j], htmdepth);
//       printf("htmID 16Ld %16Ld\n",htmID[j]);



       /* get cx,cy,cz */
       //getxyz(alphara[j],deltadec[j],&cx,&cy,&cz);
       //printf("cx 11.6f %11.6f|cy 11.6f %11.6f|cz 11.6f %11.6f\n\n\n",cx,cy,cz);
//       getxyz(alphara[j],deltadec[j],&cx[j],&cy[j],&cz[j]);
//       printf("cx 11.6f %11.6f|cy 11.6f %11.6f|cz 11.6f %11.6f\n\n\n",cx[j],cy[j],cz[j]);
//printf("Curr count: %d\n", j );









     }

     if (status) fits_report_error(stderr, status); /* print any error message */

//printf( "nrows: %d\n", nrows );
insert_objects(&ctx);
cleanup(&ctx);
exit;

     //return(status);
   }
   else {

     if (!fits_open_file(&fptr, argv[1], READONLY, &status))
       fits_read_key_flt(fptr,"SEXMGZPT",&magzp,comment,&status);
     fits_close_file(fptr, &status);

     sprintf(command, "wc -l %s", argv[1]);

     pip=popen(command,"r");
     fgets(line,1000,pip);
     sscanf(line,"%d %s",&nrows,command);
     pclose(pip);

     fin=fopen(argv[1], "r");

     for(j=0;j<nrows;j++) {
       fscanf(fin,"%d %lg %lg ",&objnum_ascii, &magauto_ascii, &magerrauto_ascii);
       fscanf(fin, "%lg %lg ", &magaper1_ascii,&magaper2_ascii);
       fscanf(fin, "%lg %lg ", &magaper3_ascii,&magaper4_ascii);
       fscanf(fin, "%lg %lg ", &magaper5_ascii,&magaper6_ascii);
       fscanf(fin, "%lg %lg ", &magerrap1_ascii,&magerrap2_ascii);
       fscanf(fin, "%lg %lg ", &magerrap3_ascii,&magerrap4_ascii);
       fscanf(fin, "%lg %lg ", &magerrap5_ascii,&magerrap6_ascii);

       fscanf(fin, "%lg %lg %lg %lg ", &alphara_ascii,&deltadec_ascii,&alphapeak_ascii,&deltapeak_ascii);
       fscanf(fin, "%lg %lg %lg %lg %lg %lg ", &x2world_ascii,&x2errworld_ascii,&y2world_ascii,&y2errworld_ascii,&xyworld_ascii,&xyerrworld_ascii);
       fscanf(fin, "%lg %lg %lg ", &threshold_ascii,&ximage_ascii,&yimage_ascii);
       fscanf(fin, "%d %d %d %d ", &xminimage_ascii,&yminimage_ascii,&xmaximage_ascii,&ymaximage_ascii);
       fscanf(fin, "%lg %lg %lg %lg ", &x2image_ascii,&x2errimage_ascii,&y2image_ascii,&y2errimage_ascii);
       fscanf(fin, "%lg %lg ", &xyimage_ascii,&xyerrimage_ascii);
       fscanf(fin, "%lg %lg %lg %lg ", &aimage_ascii,&aerrimage_ascii,&bimage_ascii,&berrimage_ascii);
       fscanf(fin, "%lg %lg %lg ", &thetaimage_ascii,&thetaerrimage_ascii,&ellipticity_ascii);
       fscanf(fin, "%lg %d", &class_ascii,&fflags_ascii);

       if(magerrauto_ascii > 999.99)
       magerrauto_ascii = 999.99;
       printf("%2.1f|%s|%2d|%2.1f|0.0|%10d|%8.4f|%8.4f|",equinox,band,imageid,magzp,objnum_ascii,magauto_ascii,magerrauto_ascii);
       if(magaper1_ascii > 999.99)
         magaper1_ascii = 999.99;
       printf("%8.4f|%8.4f|",magaper1_ascii,magerrap1_ascii);
       if(magaper2_ascii > 999.99)
         magaper2_ascii = 999.99;
       printf("%8.4f|%8.4f|",magaper2_ascii,magerrap2_ascii);
       if(magaper3_ascii > 999.99)
         magaper3_ascii = 999.99;
       printf("%8.4f|%8.4f|",magaper3_ascii,magerrap3_ascii);
       if(magaper4_ascii > 999.99)
         magaper4_ascii = 999.99;
       printf("%8.4f|%8.4f|",magaper4_ascii,magerrap4_ascii);
       if(magaper5_ascii > 999.99)
         magaper5_ascii = 999.99;
       printf("%8.4f|%8.4f|",magaper5_ascii,magerrap5_ascii);
       if(magaper6_ascii > 999.99)
         magaper6_ascii = 999.99;
       printf("%8.4f|%8.4f|",magaper6_ascii,magerrap6_ascii);

       printf("%11.7f|%11.7f|%11.7f|%11.7f|",alphara_ascii,deltadec_ascii,alphapeak_ascii,deltapeak_ascii);
       printf("%19.13e|%19.13e|%19.13e|%19.13e|%19.13e|",x2world_ascii,x2errworld_ascii,y2world_ascii,y2errworld_ascii,xyworld_ascii);
       printf("%19.13e|%12g|%10.3f|%10.3f|",xyerrworld_ascii,threshold_ascii,ximage_ascii,yimage_ascii);
       printf("%10d|%10d|%10d|%10d|",xminimage_ascii,yminimage_ascii,xmaximage_ascii,ymaximage_ascii);
       //if(y2image > 1e7)
       //y2image = 0.0;
       printf("%19.13e|%19.13e|%19.13e|%19.13e|",x2image_ascii,x2errimage_ascii,y2image_ascii,y2errimage_ascii);
       printf("%19.13e|%19.13e|%9.3f|%8.4f|",xyimage_ascii,xyerrimage_ascii,aimage_ascii,aerrimage_ascii);
       printf("%9.3f|%8.4f|%5.1f|%5.1f|",bimage_ascii,berrimage_ascii,thetaimage_ascii,thetaerrimage_ascii);
       printf("%8.3f|%5.2f|%d|",ellipticity_ascii,class_ascii,fflags_ascii);

       /* get htmID */
       htmID = cc_radec2ID(alphara_ascii, deltadec_ascii, htmdepth);
       printf("%Ld|",htmID);
       /* get cx,cy,cz */
       getxyz(alphara_ascii,deltadec_ascii,&cx,&cy,&cz);
       printf("%4.6f|%4.6f|%4.6f\n",cx,cy,cz);
     }
     fclose(fin);
   }

   return (0);
}

/*perform piecewise insert with polling*/
void insert_objects(ctxptr)
desw_ctx *ctxptr;
{
//  text *ins_stmt1 = (text *)"INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11,NULL,NULL, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24, :25, :26, :27, :28, :29, :30, :31, :32, :33, :34, :35, :36, :37, :38, :39, :40, :41, :42, :43, :44, :45, :46, :47, :48, :49, :50, :51, :52, :53, :54, :55, :56, :57, :58, :59, :60)";
  text *ins_stmt1 = (text *)"INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval, %2.1f, %s, :1, :2, :3, :4, :8, :9, %2d, %2.1f,NULL,NULL, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24, :25, :26, :27, :28, :29, :30, :31, :32, :33, :34, :35, :36, :37, :38, :39, :40, :41, :42, :43, :44, :45, :46, :47, :48, :49, :50, :51, :52, :53, :54, :55, :56, :57, :58, :59, :60)";



//  OCIBind *bndp1 = (OCIBind *) NULL;
//  OCIBind *bndp2 = (OCIBind *) NULL;
  sword status, i = 0;
//  char col2[PIECE_SIZE];
//  ub1 col3[PIECE_SIZE];
//  ub1 piece;
//  ub4 alenp2= PIECE_SIZE;
//  ub2 rcode2;

  ub4 rlsk = 0;
  ub4 rcsk = 0;
  ub4 indsk = 0;

char instStmt[5000];

//sprintf( instStmt, "INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval, %2.1f, '%s', :1, :2, :3, :4, 0, 0, %2d, %2.1f,NULL,NULL, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24, :25, :26, :27, :28, :29, :30, :31, :32, :33, :34, :35, :36, :37, :38, :39, :40, :41, :42, :43, :44, :45, :46, :47, :48, :49, :50, :51, :52, :53, :54, :55, :56, :57, :58, :59, :60)", equinox,band,imageid,magzp );





sprintf( instStmt, "INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval, %2.1f, '%s', :1, :2, :3, :4, 0, 0, %2d, %2.1f,NULL,NULL, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24, :25, :26, :27, :28, :29, :30, :31, :32, :33, :34, :35, :36, :37, :38, :39, :40, :41, :42, :43, :44, :45, :46, :47, :48, :49, :50, :51, :52, :53, :54, :55, :56, :57, :58, :59, :60)", equinox,band,imageid,magzp );



  checkerr(ctxptr->errhp, OCIStmtPrepare(ctxptr->stmthp, ctxptr->errhp,
                            //             ins_stmt1,
                            //             (ub4) strlen((char *)ins_stmt1),

(text *)instStmt,
(ub4) strlen((char *)instStmt),

                                         (ub4) OCI_NTV_SYNTAX,
                                         (ub4) OCI_DEFAULT),
                                         __LINE__);

  printf("\nBEGINING PIECEWISE INSERT INTO PERSON_1 WITH POLLING ... \n");
printf( "%s\n\n", instStmt );

/**
  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndp1, ctxptr->errhp,
                                       (ub4) 1, (dvoid *) &ssn, 
                                       (sb4) sizeof(ssn),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT));

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndp2,
                                       ctxptr->errhp, (ub4) 2,
                                       (dvoid *) col2, (sb4) DATA_SIZE, 
                                       SQLT_CHR,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DATA_AT_EXEC));
  
  
  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndp1, ctxptr->errhp,
                                       (ub4) 1, (dvoid *) OBJECT_ID, 
                                       (sb4) sizeof(),,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT));
**/



/**********************
  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 1, (dvoid *) &equinox, 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                        __LINE__);
**********************/



//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

char bnd[5];
strcpy( bnd, band );
printf( "BAND\n" );


/**********************
  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 2, (dvoid *) bnd, 
                                       (sb4) sizeof(bnd),SQLT_CHR,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                        __LINE__);
**********************/


//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(band), indsk, rlsk,
//                                              rcsk), __LINE__);



printf("htmID 16Ld %16Ld\n",htmID[0]);
printf("htmID 16Ld %16Ld\n",htmID[1]);
printf("htmID 16Ld %16Ld\n",htmID[2]);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 1, (dvoid *) &htmID, 
                                       (sb4) sizeof(uint64),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                              (ub4)sizeof(uint64), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 2, (dvoid *) &cx[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                              (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 3, (dvoid *) &cy[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                              (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 4, (dvoid *) &cz[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                              (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);


/****************************************
  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 7, (dvoid *) &parentid, 
                                       (sb4) sizeof(int),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(int), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 8, (dvoid *) &softid, 
                                       (sb4) sizeof(int),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(int), indsk, rlsk,
//                                              rcsk), __LINE__);
*********************************************/


/**************************
  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 9, (dvoid *) &imageid, 
                                       (sb4) sizeof(int),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
**************************/
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(int), indsk, rlsk,
//                                              rcsk), __LINE__);



/*************************************
  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 10, (dvoid *) &magzp, 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
***********************************/
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

/**
  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndp1, ctxptr->errhp,
                                       (ub4) 12, (dvoid *) ERRZEROPOINT, 
                                       (sb4) sizeof(float),SQLT_NUM,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT));

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndp1, ctxptr->errhp,
                                       (ub4) 13, (dvoid *) ZEROPOINTID, 
                                       (sb4) sizeof(int),SQLT_NUM,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT));
**/

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 5, (dvoid *) &objnum[0], 
                                       (sb4) sizeof(int),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(int), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 6, (dvoid *) &magauto[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 7, (dvoid *) &magerrauto[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 8, (dvoid *) &magaper[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i],
                                        ctxptr->errhp,
                                       (ub4) 9, (dvoid *) &magerrap[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 10, (dvoid *) &magaper[1], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 11, (dvoid *) &magerrap[1], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 12, (dvoid *) &magaper[2], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 13, (dvoid *) &magerrap[2], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 14, (dvoid *) &magaper[3], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 15, (dvoid *) &magerrap[3], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 16, (dvoid *) &magaper[4], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 17, (dvoid *) &magerrap[4], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 18, (dvoid *) &magaper[5], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 19, (dvoid *) &magerrap[5], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float)*6, indsk, rlsk,
//                                              rcsk), __LINE__);

/**
printf( "per: %lx, err: %lx\n", &magaper[0],&magerrap[0]);
printf( "per: %lx, err: %lx\n", &magaper,&magerrap);
printf( "per: %lx, err: %lx\n", &magaper[6],&magerrap[6]);
printf( "per: %lx, err: %lx\n", &(magaper+6),&(magerrap+6));

printf( "\n\n\n" );

printf("%8.4f|%8.4f|\n",magaper[0],magerrap[0]);
printf("%8.4f|%8.4f|\n\n",*magaper,*magerrap);
printf("%8.4f|%8.4f|\n",*(magaper+6),*(magerrap+6));
printf("%8.4f|%8.4f|\n\n",magaper[6],magerrap[6]);

printf("%8.4f|%8.4f|\n",magaper[1],magerrap[1]);
printf("%8.4f|%8.4f|\n\n",magaper[7],magerrap[7]);

printf("%8.4f|%8.4f|\n",*(magaper+6+1),*(magerrap+6+1));


printf( "D: %d, F: %d, %11.7f\n", sizeof(double),sizeof(float),alphara[1]);
**/

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 20, (dvoid *) &alphara[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 21, (dvoid *) &deltadec[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 22, (dvoid *) &alphapeak[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 23, (dvoid *) &deltapeak[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 24, (dvoid *) &x2world[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 25, (dvoid *) &x2errworld[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 26, (dvoid *) &y2world[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 27, (dvoid *) &y2errworld[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 28, (dvoid *) &xyworld[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 29, (dvoid *) &xyerrworld[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 30, (dvoid *) &threshold[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 31, (dvoid *) &ximage[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 32, (dvoid *) &yimage[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 33, (dvoid *) &xminimage[0], 
                                       (sb4) sizeof(int),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(int), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 34, (dvoid *) &yminimage[0], 
                                       (sb4) sizeof(int),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(int), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 35, (dvoid *) &xmaximage[0], 
                                       (sb4) sizeof(int),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(int), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 36, (dvoid *) &ymaximage[0], 
                                       (sb4) sizeof(int),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(int), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 37, (dvoid *) &x2image[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 38, (dvoid *) &x2errimage[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 39, (dvoid *) &y2image[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 40, (dvoid *) &y2errimage[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 41, (dvoid *) &xyimage[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
 //                                             rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 42, (dvoid *) &xyerrimage[0], 
                                       (sb4) sizeof(double),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(double), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 43, (dvoid *) &aimage[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 44, (dvoid *) &aerrimage[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                         __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
 //                                             rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 45, (dvoid *) &bimage[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                       __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 46, (dvoid *) &berrimage[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                        __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 47, (dvoid *) &thetaimage[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                        __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
 //                                             rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 48, (dvoid *) &thetaerrimage[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                        __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 49, (dvoid *) &ellipticity[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                        __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 50, (dvoid *) &class[0], 
                                       (sb4) sizeof(float),SQLT_FLT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                        __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(float), indsk, rlsk,
//                                              rcsk), __LINE__);

  checkerr(ctxptr->errhp, OCIBindByPos(ctxptr->stmthp, &bndhpArray[i], 
                                       ctxptr->errhp,
                                       (ub4) 51, (dvoid *) &fflags[0], 
                                       (sb4) sizeof(short),SQLT_INT,
                                       (dvoid *) 0, (ub2 *)0, (ub2 *)0,
                                       (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT),
                                        __LINE__);
//  checkerr(ctxptr->errhp, OCIBindArrayOfStruct(bndhpArray[i++], ctxptr->errhp,
//                                               (ub4)sizeof(short), indsk, rlsk,
//                                              rcsk), __LINE__);






printf( "BEFORE...\n" );
/**  
  checkerr(ctxptr->errhp, OCIStmtExecute(ctxptr->svchp, ctxptr->stmthp, 
                                        // ctxptr->errhp,(ub4) nrows, (ub4) 0,
                                         ctxptr->errhp,(ub4) (nrows-1), (ub4) 0,
                                         (CONST OCISnapshot*) 0, 
                                         (OCISnapshot*) 0,
                                         (ub4) OCI_DEFAULT), __LINE__); 
**/
/**
  checkerr(ctxptr->errhp, OCIStmtExecute(ctxptr->svchp, ctxptr->stmthp,
                                        // ctxptr->errhp,(ub4) nrows, (ub4) 0,
                                         ctxptr->errhp,(ub4) (1339), (ub4) 1338,
                                         (CONST OCISnapshot*) 0,
                                         (OCISnapshot*) 0,
                                         (ub4) OCI_BATCH_ERRORS), __LINE__);
**/

printf( "AFTER....\n\n\n" );






printf( "BEFORE2...\n" );
//  checkerr(ctxptr->errhp, 
OCIStmtExecute(ctxptr->svchp, ctxptr->stmthp,
                                        // ctxptr->errhp,(ub4) nrows, (ub4) 0,
                                         //ctxptr->errhp,(ub4) (nrows-1), (ub4) 1301,
                                         ctxptr->errhp,(ub4) (nrows-1), (ub4) 0,
                                         (CONST OCISnapshot*) 0,
                                         (OCISnapshot*) 0,
                                         (ub4) OCI_BATCH_ERRORS);
//, __LINE__);
printf( "AFTER2(%d)....\n", nrows );

/**
ub4 num_errs, row_off[500];
OCIError *errhndl[500];
OCIError *errhp2;
text errbuf[512];
ub4 buflen;
sb4 errcode;

printf( "GET # errors...\n" );
OCIAttrGet (ctxptr->stmthp, OCI_HTYPE_STMT, &num_errs, 0,
            OCI_ATTR_NUM_DML_ERRORS, ctxptr->errhp);
printf( "#errors: %d\n", num_errs );
if (num_errs) 
{
   for (i = 0; i < num_errs; i++) 
   {
printf("I = %d\n", i );
printf("1\n");
      OCIHandleAlloc( (dvoid *)ctxptr->envhp, (dvoid **)&errhndl[i],
      (ub4) OCI_HTYPE_ERROR, 0, (dvoid *) 0);
printf("2\n");
      OCIParamGet(ctxptr->errhp, OCI_HTYPE_ERROR, ctxptr->errhp, &errhndl[i], i);
printf("3\n");
      OCIAttrGet (errhndl[i], OCI_HTYPE_ERROR, &row_off[i], 0,
                  OCI_ATTR_DML_ROW_OFFSET, errhp2);
printf("4\n");
OCIErrorGet ((dvoid *) errhndl[i], (ub4) 1, (text *) NULL, &errcode, errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);
printf("Error - %s\n", errbuf);
   }
exit(1);
}
**/












double cx,cy,cz;

j = 2;
//for( j = 1330; j < 1341; j++ )
for( j = 0; j < nrows-1; j++ )
{
/**
       printf("equinox 2.1f %2.1f|band %s|imageid 2d %2d|magzp 2.1f %2.1f|0.0|objnum 10d %10d|magauto 8.4f %8.4f|magerrauto 8.4f %8.4f\n",equinox,band,imageid,magzp,objnum[j],magauto[j],magerrauto[j]);
       for (k=0;k<6;k++) {
         if(magerrap[j*6+k] > 999.99)
           magerrap[j*6+k] = 999.99;
         printf("magaper 8.4f [%d*6+%d] %8.4f|magerrap 8.4f %8.4f\n",j, k, magaper[j*6+k],magerrap[j*6+k]);
       }
       printf("alphara 11.7f %11.7f|deltadec 11.7f %11.7f|alphapeak 11.7f %11.7f|deltapeak 11.7f %11.7f\n",alphara[j],deltadec[j],alphapeak[j],deltapeak[j]);
       printf("x2world 19.13e %19.13e|x2errworld  19.13e %19.13e|y2world 19.13e %19.13e|y2errworld 19.13e %19.13e|xyworld 19.13e %19.13e\n",x2world[j],x2errworld[j],y2world[j],y2errworld[j],xyworld[j]);
       printf("xyerrworld 19.13e %19.13e|threshold 12g %12g|ximage 10.3f %10.3f|yimage 10.3f %10.3f\n",xyerrworld[j],threshold[j],ximage[j],yimage[j]);
       printf("xminimage 10d %10d|yminimage 10d %10d|xmaximage 10d %10d|ymaximage 10d %10d\n",xminimage[j],yminimage[j],xmaximage[j],ymaximage[j]);
       //if(y2image[j] > 1e7)
       //y2image[j] = 0.0;
       printf("x2image 19.13e %19.13e|x2errimage 19.13e %19.13e|y2image 19.13e %19.13e|y2errimage 19.13e %19.13e\n",x2image[j],x2errimage[j],y2image[j],y2errimage[j]);
       printf("xyimage 19.13e  %19.13e|xyerrimage 19.13e %19.13e|aimage 9.3f %9.3f|aerrimage 8.4f %8.4f\n",xyimage[j],xyerrimage[j],aimage[j],aerrimage[j]);
       printf("bimage 9.3f %9.3f|berrimage 8.4f %8.4f|thetaimage 5.1f %5.1f|thetaerrimage 5.1f %5.1f\n",bimage[j],berrimage[j],thetaimage[j],thetaerrimage[j]);
       printf("ellipticity 8.3f %8.3f|class 5.2f %5.2f|fflags d %d\n",ellipticity[j],class[j],fflags[j]);
//        get htmID 
       htmID = cc_radec2ID(alphara[j], deltadec[j], htmdepth);
**/
       printf("%16Ld\n",htmID[j]);
/**
//        get cx,cy,cz 
       getxyz(alphara[j],deltadec[j],&cx,&cy,&cz);
       printf("cx 11.6f %11.6f|cy 11.6f %11.6f|cz 11.6f %11.6f\n\n\n",cx,cy,cz);
**/




instStmt[0] = '\0';
//sprintf( instStmt, "INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval,EQUINOX %2.1f,BAND '%s',HTMID %16Ld,CX %11.6f,CY %11.6f,CZ %11.6f, 0 , 0,IMAGEID %2d,ZEROPOINT %2.1f, NULL, NULL,OBJECT_NUMBER %10d,MAG_AUTO %8.4f,MAGERR_AUTO %8.4f,MAG_APER_1 %8.4f,MAGERR_APER_1 %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %11.7f, %11.7f, %11.7f, %11.7f, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %12g, %10.3f, %10.3f, %10d, %10d, %10d, %10d, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %9.3f, %8.4f, %9.3f, %8.4f, %5.1f, %5.1f, %8.3f, %5.2f, %d)", equinox,band,htmID,cx,cy,cz,imageid,magzp,objnum[j],magauto[j],magerrauto[j],magaper[j*6],magerrap[j*6],magaper[j*6+1],magerrap[j*6+1],magaper[j*6+2],magerrap[j*6+2],magaper[j*6+3],magerrap[j*6+3],magaper[j*6+4],magerrap[j*6+4],magaper[j*6+5],magerrap[j*6+5],alphara[j],deltadec[j],alphapeak[j],deltapeak[j],x2world[j],x2errworld[j],y2world[j],y2errworld[j],xyworld[j],xyerrworld[j],threshold[j],ximage[j],yimage[j],xminimage[j],yminimage[j],xmaximage[j],ymaximage[j],x2image[j],x2errimage[j],y2image[j],y2errimage[j],xyimage[j],xyerrimage[j],aimage[j],aerrimage[j],bimage[j],berrimage[j],thetaimage[j],thetaerrimage[j],ellipticity[j],class[j],fflags[j] );

//printf( "%s\n", instStmt );



/**
printf( "HERE1\n" );
printf("cx 11.6f %11.6f|cy 11.6f %11.6f|cz 11.6f %11.6f\n",cx,cy,cz);
printf(" %11.6f, %11.6f, %11.6f\n\n\n", cx,cy,cz);
printf("%11.6f\n",cx);
printf("%11.6f\n",cy);
printf("%11.6f\n\n\n",cz);
printf(" %11.6f, %11.6f, %11.6f\n\n\n", cx,cy,cz);
printf("%2.1f\n", equinox);
printf("%11.6f\n",cx);
printf("%11.6f\n",cy);
printf("%11.6f\n\n\n",cz);
printf(" %11.6f, %11.6f, %11.6f\n\n\n", cx,cy,cz);
printf("'%s'\n", band);
printf("%11.6f\n",cx);
printf("%11.6f\n",cy);
printf("%11.6f\n\n\n",cz);
printf(" %11.6f, %11.6f, %11.6f\n\n\n", cx,cy,cz);
printf("%16Ld\n", htmID);
printf("%11.6f\n",cx);
printf("%11.6f\n",cy);
printf("%11.6f\n\n\n",cz);
printf(" %11.6f, %11.6f, %11.6f\n\n\n", cx,cy,cz);
printf(" %2.1f, '%s', %16Ld, %11.6f, %11.6f, %11.6f, %2d, %2.1f, %10d\n\n\n", equinox,band,htmID,cx,cy,cz,imageid,magzp,objnum[j]);
printf(" %11.6f, %11.6f, %11.6f\n\n\n", cx,cy,cz);

sprintf( instStmt, "INSERT INTO objects (OBJECT_ID, EQUINOX, BAND, HTMID, CX, CY, CZ, PARENTID, SOFTID, IMAGEID, ZEROPOINT, ERRZEROPOINT, ZEROPOINTID, OBJECT_NUMBER, MAG_AUTO, MAGERR_AUTO, MAG_APER_1, MAGERR_APER_1, MAG_APER_2, MAGERR_APER_2, MAG_APER_3, MAGERR_APER_3, MAG_APER_4, MAGERR_APER_4, MAG_APER_5, MAGERR_APER_5, MAG_APER_6, MAGERR_APER_6, ALPHA_J2000, DELTA_J2000, ALPHAPEAK_J2000, DELTAPEAK_J2000, X2_WORLD, ERRX2_WORLD, Y2_WORLD, ERRY2_WORLD, XY_WORLD, ERRXY_WORLD, THRESHOLD, X_IMAGE, Y_IMAGE, XMIN_IMAGE, YMIN_IMAGE, XMAX_IMAGE, YMAX_IMAGE, X2_IMAGE, ERRX2_IMAGE, Y2_IMAGE, ERRY2_IMAGE, XY_IMAGE, ERRXY_IMAGE, A_IMAGE, ERRA_IMAGE, B_IMAGE, ERRB_IMAGE, THETA_IMAGE, ERRTHETA_IMAGE, ELLIPTICITY, CLASS_STAR, FLAGS) VALUES (objects_seq.nextval, %2.1f, '%s', %16Ld, %11.6f, %11.6f, %11.6f, 0 , 0, %2d, %2.1f, NULL, NULL, %10d, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %8.4f, %11.7f, %11.7f, %11.7f, %11.7f, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %12g, %10.3f, %10.3f, %10d, %10d, %10d, %10d, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %19.13e, %9.3f, %8.4f, %9.3f, %8.4f, %5.1f, %5.1f, %8.3f, %5.2f, %d)", equinox,band,htmID,cx,cy,cz,imageid,magzp,objnum[j],magauto[j],magerrauto[j],magaper[j*6],magerrap[j*6],magaper[j*6+1],magerrap[j*6+1],magaper[j*6+2],magerrap[j*6+2],magaper[j*6+3],magerrap[j*6+3],magaper[j*6+4],magerrap[j*6+4],magaper[j*6+5],magerrap[j*6+5],alphara[j],deltadec[j],alphapeak[j],deltapeak[j],x2world[j],x2errworld[j],y2world[j],y2errworld[j],xyworld[j],xyerrworld[j],threshold[j],ximage[j],yimage[j],xminimage[j],yminimage[j],xmaximage[j],ymaximage[j],x2image[j],x2errimage[j],y2image[j],y2errimage[j],xyimage[j],xyerrimage[j],aimage[j],aerrimage[j],bimage[j],berrimage[j],thetaimage[j],thetaerrimage[j],ellipticity[j],class[j],fflags[j] );

printf( "HERE2: %2d\n", imageid );
printf("cx 11.6f %11.6f|cy 11.6f %11.6f|cz 11.6f %11.6f\nn",cx,cy,cz);
printf(" %2.1f, '%s', %16Ld, %11.6f, %11.6f, %11.6f, %2d, %2.1f, %10d\n\n\n", equinox,band,htmID,cx,cy,cz,imageid,magzp,objnum[j]);

printf( "%s\n", instStmt );

printf( "INSERT: %d\n", j );
sql_stmt_execute(ctxptr, (text *)instStmt );



printf( "IID: %2d\n", imageid );
printf("Curr count: %d\n", j );
**/
}







  
/**
  while (1)
  {
    status = OCIStmtExecute(ctxptr->svchp, ctxptr->stmthp, ctxptr->errhp,
    (ub4) 1, (ub4) 0, (CONST OCISnapshot*) 0,
        (OCISnapshot*) 0, (ub4) OCI_DEFAULT);
    switch(status)
    {
      case OCI_NEED_DATA:
           set_piece(&piece);
           if(OCIStmtSetPieceInfo((dvoid *)bndp2,
              (ub4)OCI_HTYPE_BIND,ctxptr->errhp, (dvoid *)col2,
              &alenp2, piece, (dvoid *) 0, &rcode2))
           {
             printf("ERROR: OCIStmtSetPieceInfo returned %d \n", status);
             break;
           }
           status = OCI_NEED_DATA;
      break;
      case OCI_SUCCESS:
      break;
      default:
           printf( "oci exec returned %d \n", status);
           checkerr(ctxptr->errhp, status);
           status = 0;
    }
    
    if (!status) break;
  }
**/
}

/*set piece information for piecewise insert with polling*/
void set_piece(piecep)
	ub1 *piecep;
{
	static sword piece_cnt = 0;

	switch (piece_cnt)
	{
	case 0:
		*piecep = OCI_FIRST_PIECE;
		break;
	case NPIECE - 1:
		*piecep = OCI_LAST_PIECE;
		piece_cnt = 0;
		return;
	default:
		*piecep = OCI_NEXT_PIECE;
	}
	piece_cnt++;
	return;
}

