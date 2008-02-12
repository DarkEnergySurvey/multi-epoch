#include "imageproc.h"  
#include "nrutil.h"

#define VERSION 1.14
#define SVN_VERSION 626
/* 

1.00  -- original (working) version
1.10  -- change the call of swarp that input a list (with @list) and only getting the first extension with image.fits[0] in the list
         doing the same for variance list with image.fits[1]
         replace the select_remaps call with DB call 
         add the capability of input a list of nites and/or runid (comma separated)
         rewrite the part for matching 4 corners of overlapping images
1.11  -- change the range for setting the reference ZP from 20-35 to 25-35
         add output of ZP comparison (calculated ZP vs direct ZP) 
1.12  -- add output of imageid to a file
1.13  -- change background subtrack to Y
1.14  -- include ZP solutions to the imageid file

*/

#define TOLERANCE (2.0/3600.0) /* arcsec */
#define ACCURACY 1e-6 /* accuracy in the sigma clipping algorithm */
#define LGE 0.434294 /* log_10(e) */
#define MAX 1000 /* Max number of matched stars from two overlapped images */
#define MAXMAGDIFF 1.25 /* Max mag difference between image(i) and reference image */
#define MOSAIC 8 
#define DECAM 62
#define Squ(x) ((x)*(x))

void helpmessage(int dummy);
void print_matrix(float **matrix, int nrow, int ncol);
void print_dmatrix(double **matrix, int nrow, int ncol);
void print_imatrix(int **matrix, int nrow, int ncol);
void print_vector(float *vector, int n);
void initialize_matrix(float **matrix, int nrow, int ncol);
void initialize_dmatrix(double **matrix, int nrow, int ncol);
void initialize_imatrix(int **matrix, int nrow, int ncol);
void initialize_vector(float *vector, int n);
void initialize_dvector(double *vector, int n);
void svd_fit(float **A, float *b, float *berr, int nrow, int ncol, float *x, float *xerr);
void iter_mean(double data[], double err[], int N, double *mean, double *rms, int *N_good, int *N_bad, int *N_iter, int flag, int flag_iter, int flag_Niterate, int Nmax_iterate, double THRESHOLD);
double getmean(double data[],double err[],int N,int flag);
double getrms(double data[],double err[],int N,int flag);
double calc_ccdratio(double **skybrite_bookeep, double **skybrite_ccdpair, double **skybriteerr_ccdpair, int ccdtotal, int Nexposure);

main(argc,argv)
     int argc;
     char *argv[];
{
  char project[100],tilenamein[300],binpath[800],etcpath[800],err[1000];
  char bandlist[1000],nitelist[1000],runidlist[1000],temp[200],imgin[1000];
  char selectremapfile[1000],basedir[1000],outfile[800],imgfullpath[15000];
  char command[2000],comment[800],line[1000],sqlcall[1000],combinetype[50];
  char mag1[20],magerr1[20],mag2[20],magerr2[20],image1[1000],image2[1000];
  char detector[20],ctype1[32],ctype2[32],bandtemp[5],prev[200],image[800];
  char sqlscript[1000],swarpscript[1000],varimagearray[8000],tempnew[1000];
  char magerr_input[100],imagetemp[1000],dblogin[500],zpout[1000],idout[800];
  char **tilerunid,**runidnites,**nites,**runids,**bands,**nites_in,**runid_in;

  int flag_quiet=0,flag_proj=0,flag_tilein=0,flag_outfile=0;
  int flag_binpath=0,flag_bandlist=0,flag_nitelist=0,flag_fwhm=0;
  int flag_runidlist=0,flag_check=0,flag_selectremap=0,flag_rmscut=0;
  int flag_etcpath=0,flag_basedir=0,flag_combinetype=0,flag_magerr=0;
  int flag_detector=0,flag_magcut=0,flag_weightfit=0;
  int flag_Niterate=0,flag_Nstarmatch=0,flag_mean=0;
  int flag_nostarmatch=0,flag_noskybrite=0,flag_exptime=0;
  int flag_nophotozp=0,flag_quickcoadd=0;
  int status=0,newcount=0;

  int i,j,k,s,m,maxzp,flag_zpmax,endloop,istart,jstart,len,countstar;
  int nimage,nzp,nzpin,nstarmatch,nskybrite,imageid_temp,count,npix_ra,npix_dec;
  int nimage_in,flag,flag_se,flag_iter,flag_weight,magtype,check_exist;
  int N_good,N_bad,N_iter,ccdtotal,Nmax_iterate,Nrun;
  int ncol,nrow,nconstrain,newcountstar,Nmin_starmatch;
  int Nrunidnite,Nband,Nnite,Nrunid,ccdnum,Nexposure,track;
  int ncomma,nnite_in,nrunid_in;
  int *countimg,*saveid1,*saveid2,*keepimg;
  int **flag_mag_mean,**flag_skybrite;

  float class_star_lo,class_star_hi,radius,sigma,magerrin,fwhm_in,fwhm;
  float exptime_in,exptime,magout1,magout2,magerrout1,magerrout2;
  float magrmscut,mag_base,fluxscale,mag_low,mag_high,pixelsize;
  float *skybrite,*b,*berr,*x,*xerr,**A;

  double crpix1,crpix2,crval1,crval2;
  double cd1_1,cd1_2,cd2_1,cd2_2;
  double rho,rho_a,rho_b,crota2;
  double cdelt1,cdelt2;
  double xpix,ypix;
  double ra_tem,dec_tem,ra_tem1,dec_tem1,ra_tem2,dec_tem2;
  double maxra,minra,maxdec,mindec;
  double tile_ra,tile_dec,ra_lo,ra_hi,dec_lo,dec_hi;
  double mag_mean,magrms_mean,skybrite_temp,weight_temp;
  double *ra1,*ra2,*ra3,*ra4,*dec1,*dec2,*dec3,*dec4;
  double **skybrite_bookeep,**skybrite_ccdpair;
  double **skybriteerr_ccdpair,**save_skybriteerr;
  double *datain,*errin,**save_mag_mean;
  double **save_mag_rms,**save_skybrite;

  long axes0,axes1;
  time_t curtime=time (NULL), lsttime;
  
  db_tiles      *tileinfo, *tileinfo_in;
  db_zeropoint  *zp, *zp_latest;

  FILE *fselectremap,*fsqlout,*pip,*fout,*fswarp,*ferr,*fswarpimg,*fswarpvar,*fzpcompare,*fidout;
  fitsfile *fptr;
  void select_dblogin(),printerror();
  
  if (argc<2) {
    printf("Usage: %s \n",argv[0]);
    printf("       -project <project name>\n");
    printf("       -tilename <tile string>\n");
    //printf("       -band <band#1,band#2,...>\n");
    printf("       -band <band>\n");
    printf("       -basedir <basedir> (dir level before runid/data/nite/band/...) \n");
    printf("       -detector <detector> (either DECam or Mosaic2) \n");
    printf("\n");

    printf("    Option:\n");
    printf("          -binpath <binpath>\n");
    printf("          -etcpath <etcpath>\n");
    printf("          -nite <nite#1,nite#2,...>\n");
    printf("          -runid <runid#1,runid#2,...>\n");
    //printf("          -selectremap <filename> (output file from select_remap with [-type REMAP -coadd -quiet] options)\n");
    printf("          -class_star <lower_#> <upper_#> (set the range of CLASS_STAR; default is 0.99 to 1)\n");
    printf("          -sigmaclip <#> (threshold in sigma-clipping if use sigma-clipping; default is not using)\n");
    printf("          -Niterate <#> (maximum number of iteration in sigma-clipping; default is not set)\n");
    printf("          -Nstarmatch <#> (minimum number of matched stars; default is 1)\n");
    printf("          -rmscut <#> (RMS cut for the mean magnitudes in star matching; default is 0.05)\n");
    printf("          -flag <#> (upper limit of flag value from SExtractor; default is 0)\n");
    printf("          -radius <#> (radius of search in arcmin;default is 0.034)\n");
    printf("          -magtype <#> (default is 0)\n");
    printf("                       (0 = mag_auto)\n");
    printf("                       (1 = mag_aper1)\n");
    printf("                       (2 = mag_aper2)\n");
    printf("                       (3 = mag_aper3)\n");
    printf("                       (4 = mag_aper4)\n");
    printf("                       (5 = mag_aper5)\n");
    printf("                       (6 = mag_aper6)\n");
    printf("          -magerr <#> (magerr filtering; default is 0.10)\n");
    printf("          -magcut <low_mag> <high_mag> (range of magnitude cut; default is no magnitude cut applied)\n");
    printf("          -fwhm <#> (maximum value for fhwm cut; default is not using fwhm cut)\n");
    printf("          -exptime <#> (minimum value for fhwm cut; default is not using exposure time cut)\n");
    printf("          -weightmean (if use weighted mean; default is using unweighted mean)\n");
    printf("          -weightfit (if use weighted fit in matrix equation; default is using unweighted fit)\n");
    printf("          -weight (if use both weighted mean and weighted fit; default is using unweighted)\n");
    printf("          -combinetype <median,average,min,max,weighted,chi2,sum; default is median> \n");
    printf("          -output <coadd filename>\n");
    printf("          -nostarmatch\n");
    printf("          -noskybrite\n");
    printf("          -nophotozp\n");
    printf("          -quickcoadd\n");
    printf("          -help\n");
    printf("          -version\n");
    printf("          -quiet\n");
    exit(0);
  }
  
  /* set default value */
  flag_se=0;
  flag_iter=0;
  flag_weight=0;
  class_star_lo=0.99;
  class_star_hi=1.0; 
  magtype=0;
  magrmscut=0.05;
  magerrin=0.10;
  radius=0.034;
  sigma=2.5;
  Nmin_starmatch=1;
  
  sprintf(combinetype,"MEDIAN");
  nconstrain=0;
  nzp=nstarmatch=nskybrite=0;

  /* process the command line */
  for (i=1;i<argc;i++) {
    if (!strcmp(argv[i],"-project"))  {
      flag_proj=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -project option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {     
	sprintf(project,"%s",argv[i+1]);
	if (!strncmp(&project[0],"-",1)) {
	  printf(" ** %s error: wrong input of <project name>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-tilename"))  {
      flag_tilein=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -tilename option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(tilenamein,"%s",argv[i+1]);
	if (!strncmp(&tilenamein[0],"-",1)) {
	  printf(" ** %s error: wrong input of <tile string>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-band"))  {
      flag_bandlist=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -band option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(bandlist,"%s",argv[i+1]);
	if (!strncmp(&bandlist[0],"-",1)) {
	  //printf(" ** %s error: wrong input of <band#1,band#2,...>\n",argv[0]);
	  printf(" ** %s error: wrong input of <band>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-basedir"))  {
      flag_basedir=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -basedir option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(basedir,"%s",argv[i+1]);
	if (!strncmp(&basedir[0],"-",1)) {
	  printf(" ** %s error: wrong input of <basedir>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-detector"))  {
      flag_detector=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -detector option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(detector,"%s",argv[i+1]);
	if (!strncmp(&basedir[0],"-",1)) {
	  printf(" ** %s error: wrong input of <detector>\n",argv[0]);
	  exit(0);
	}
	
	if(!strcmp(detector,"DECam")) ccdtotal=DECAM;
	else if(!strcmp(detector,"Mosaic2")) ccdtotal=MOSAIC;
	else {
	  printf(" ** %s error: check the input of <detector>\n",argv[0]);
	  exit(0);
	}
      }
    }
    
    /* options */
    if (!strcmp(argv[i],"-binpath"))  {
      flag_binpath=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -binpath option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(binpath,"%s",argv[i+1]);
	if (!strncmp(&binpath[0],"-",1)) {
	  printf(" ** %s error: wrong input of <binpath>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-etcpath"))  {
      flag_etcpath=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -etcpath option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(etcpath,"%s",argv[i+1]);
	if (!strncmp(&etcpath[0],"-",1)) {
	  printf(" ** %s error: wrong input of <etcpath>\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-nite"))  {
      flag_nitelist=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -nite option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(nitelist,"%s",argv[i+1]);
	if (!strncmp(&nitelist[0],"-",1)) {
	  printf(" ** %s error: wrong input of <nite#1,nite#2,...>\n",argv[0]);
	  exit(0);
	}

	ncomma=0;
	len=strlen(nitelist);
	for (j=len;j>0;j--) {
	  if (!strncmp(&(nitelist[j]),",",1)) { 
	    ncomma++;
	    nitelist[j]=32;
	  }
	}
	nnite_in=ncomma+1;	
	nites_in=(char **)calloc(nnite_in,sizeof(char *));
	for(j=0;j<nnite_in;j++) nites_in[j]=(char *)calloc(64,sizeof(char ));
	s=0;
	for(k=0;k<nnite_in;k++) {
	  sscanf(nitelist+s,"%s%[\0]",temp);
	  sprintf(nites_in[k],"%s",temp);
	  len=strlen(temp);
	  s+=len+1;
	}	
      }
    }

    if (!strcmp(argv[i],"-runid"))  {
      flag_runidlist=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -runid option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {    
	sprintf(runidlist,"%s",argv[i+1]);
	if (!strncmp(&runidlist[0],"-",1)) {
	  printf(" ** %s error: wrong input of <runid#1,runid#2,...>\n",argv[0]);
	  exit(0);
	}

	ncomma=0;
	len=strlen(runidlist);
	for (j=len;j>0;j--) {
	  if (!strncmp(&(runidlist[j]),",",1)) { 
	    ncomma++;
	    runidlist[j]=32;
	  }
	}
	nrunid_in=ncomma+1;
	runid_in=(char **)calloc(nrunid_in,sizeof(char *));
	for(j=0;j<nrunid_in;j++) runid_in[j]=(char *)calloc(1024,sizeof(char ));
	s=0;
	for(k=0;k<nrunid_in;k++) {
	  sscanf(runidlist+s,"%s%[\0]",temp);
	  sprintf(runid_in[k],"%s",temp);
	  len=strlen(temp);
	  s+=len+1;
	}
      }
    }

    if (!strcmp(argv[i],"-selectremap"))  {
      flag_selectremap=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: filename for -selectremap option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(selectremapfile,"%s",argv[i+1]);
	if (!strncmp(&selectremapfile[0],"-",1)) {
	  printf(" ** %s error: wrong input of <filename> for -selectremap\n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-class_star")) {
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -class_star option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input of <lower#> <upper#> for -class_star\n",argv[0]);
	  exit(0);
	}
	sscanf(argv[i+1],"%f",&class_star_lo);
	sscanf(argv[i+2],"%f",&class_star_hi);
      }
    }
     
    if (!strcmp(argv[i],"-magcut")) {
      flag_magcut=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -magcut option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input of <low_mag> <upp_mag> for -magcut\n",argv[0]);
	  exit(0);
	}
	sscanf(argv[i+1],"%f",&mag_low);
	sscanf(argv[i+2],"%f",&mag_high);
      }
    }
 
    if (!strcmp(argv[i],"-sigmaclip")) {
      flag_iter=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -sigmaclip option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -sigmaclip\n",argv[0]);
	  exit(0);
	}
	sscanf(argv[i+1],"%f",&sigma);
      }
    }

    if (!strcmp(argv[i],"-Niterate")) {
      flag_iter=1;
      flag_Niterate=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -Niterate option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -Niterate\n",argv[0]);
	  exit(0);
	}
	Nmax_iterate=atoi(argv[i+1]);
      }
    }

    if (!strcmp(argv[i],"-Nstarmatch")) {
      flag_Nstarmatch=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -Nstarmatch option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -Nstarmatch\n",argv[0]);
	  exit(0);
	}
	Nmin_starmatch=atoi(argv[i+1]);
      }
    }

    if (!strcmp(argv[i],"-rmscut")) { 
      flag_rmscut=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -rmscut option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -rmscut\n",argv[0]);
	  exit(0);
	}
	sscanf(argv[i+1],"%f",&magrmscut);
      }
    }

    if (!strcmp(argv[i],"-flag")) {
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -flag option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -flag\n",argv[0]);
	  exit(0);
	}
	flag_se=atoi(argv[i+1]);
      }
    }

    if (!strcmp(argv[i],"-radius")) {
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -radius option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -radius\n",argv[0]);
	  exit(0);
	}
	sscanf(argv[i+1],"%f",&radius);
      }
    }

    if (!strcmp(argv[i],"-magtype")) {
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -magtype option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -magtype\n",argv[0]);
	  exit(0);
	}
	magtype=atoi(argv[i+1]);
      }
    }
    
    if (!strcmp(argv[i],"-magerr")) {
      flag_magerr=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -magerr option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -magerr\n",argv[0]);
	  exit(0);
	}
	magerrin=atoi(argv[i+1]);
      }
    }

    if (!strcmp(argv[i],"-fwhm")) { 
      flag_exptime=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -fwhm option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -fwhm\n",argv[0]);
	  exit(0);
	}
	sscanf(argv[i+1],"%f",&fwhm_in);
      }
    }

    if (!strcmp(argv[i],"-exptime")) { 
      flag_exptime=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: value for -exptime option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(temp,"%s",argv[i+1]);
	if (!strncmp(&temp[0],"-",1)) {
	  printf(" ** %s error: wrong input for -exptime\n",argv[0]);
	  exit(0);
	}
	sscanf(argv[i+1],"%f",&exptime_in);
      }
    }

    if (!strcmp(argv[i],"-output"))  {
      flag_outfile=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: filename for -output option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(outfile,"%s",argv[i+1]);
	if (!strncmp(&outfile[0],"-",1)) {
	  printf(" ** %s error: wrong input of <filename> for -output\n",argv[0]);
	  exit(0);
	}
	if (strncmp(&(outfile[strlen(outfile)-5]),".fits",5)) {
	  printf("  ** %s error: output file must end with .fits \n",argv[0]);
	  exit(0);
	}
      }
    }

    if (!strcmp(argv[i],"-combinetype"))  {
      flag_combinetype=1;
      if(argv[i+1]==NULL) {
        printf(" ** %s ERROR: input for -combinetype option is not set. Abort!\n",argv[0]);
        exit(0);
      }
      else {
	sprintf(combinetype,"%s",argv[i+1]);
	if (!strncmp(&combinetype[0],"-",1)) {
	  printf(" ** %s error: wrong input of <combinetype>\n",argv[0]);
	  exit(0);
	}
	if (!strcmp(argv[i+1],"median")) sprintf(combinetype,"MEDIAN");
	else if (!strcmp(argv[i+1],"average")) sprintf(combinetype,"AVERAGE");
	else if (!strcmp(argv[i+1],"min")) sprintf(combinetype,"MIN");
	else if (!strcmp(argv[i+1],"max")) sprintf(combinetype,"MAX");
	else if (!strcmp(argv[i+1],"weighted")) sprintf(combinetype,"WEIGHTED");
	else if (!strcmp(argv[i+1],"chi2")) sprintf(combinetype,"CHI2");
	else if (!strcmp(argv[i+1],"sum")) sprintf(combinetype,"SUM");
	else {
	  printf(" ** %s error: wrong input for <combinetype>, reset to default\n");
	  sprintf(combinetype,"WEIGHTED");
	}
      }
    }

    if (!strcmp(argv[i],"-weightmean")) flag_weight=1;
    if (!strcmp(argv[i],"-weightfit")) flag_weightfit=1;
    if (!strcmp(argv[i],"-weight")) {
      flag_weightfit=1; flag_weight=1;
    }
    if (!strcmp(argv[i],"-help")) {
      helpmessage(0);
      exit(0);
    }
    if (!strcmp(argv[i],"-version")) {
      printf("%s: Version %2.2f (SVN Version %d)\n",argv[0],VERSION,SVN_VERSION);
      exit(0);
    }
    if (!strcmp(argv[i],"-quiet")) flag_quiet=1;

    if (!strcmp(argv[i],"-nostarmatch")) flag_nostarmatch=1;
    if (!strcmp(argv[i],"-noskybrite")) flag_noskybrite=1;
    if (!strcmp(argv[i],"-nophotozp")) flag_nophotozp=1;
    if (!strcmp(argv[i],"-quickcoadd")) flag_quickcoadd=1;
 }

  /* print out the time of processing */
  if(!flag_quiet)
    printf("\n ** Running %s (Version %2.2f) on %s \n",argv[0],VERSION,asctime(localtime (&curtime)));

  /* check the required input commands */
  if(!flag_proj) { printf(" ** %s error: -project is not set, abort!\n", argv[0]); flag_check=1; }
  if(!flag_tilein) { printf(" ** %s error: -tilename is not set, abort!\n", argv[0]); flag_check=1; }
  if(!flag_bandlist) { printf(" ** %s error: -band is not set, abort!\n", argv[0]); flag_check=1; }
  if(!flag_basedir) { printf(" ** %s error: -basedir is not set, abort!\n", argv[0]); flag_check=1; }
  if(!flag_detector) { printf(" ** %s error: -detector is not set, abort!\n", argv[0]); flag_check=1; }
  if(flag_check) exit(0);

  /* for quickcoadd */
  if(flag_quickcoadd) {
    flag_nostarmatch=1;
    flag_noskybrite=1;
    flag_nophotozp=1;
  }

  /* printout the parameters */
  if(!flag_quiet) {
    if(!flag_quickcoadd) {
      printf(" ** Using CLASS_STAR range of %2.2f-%2.2f\n",class_star_lo,class_star_hi); 
      printf(" ** Using matching-radius of %2.4f (arcmin)\n",radius); 
      printf(" ** Using SExtractor flag of %d\n",flag_se); 
      if(flag_iter) printf(" ** Using %2.2f-sigma clipping algorithm\n",sigma);
      if(flag_Niterate) printf(" ** Using Nmax_iteration of %d\n",Nmax_iterate);
      printf(" ** Using minimum Nstarmatch of %d\n",Nmin_starmatch);
      printf(" ** Using %2.3f as maximum RMS in star matching calculation\n",magrmscut);
      if(flag_fwhm) printf(" ** Using maximum FWHM of %2.3f\n",fwhm_in);
      if(flag_exptime) printf(" ** Using minimum EXPTIME of %2.3f\n",exptime_in);
      if(flag_weight) printf(" ** Using weighted mean\n");
      else printf(" ** Using unweighted mean\n");
      if(flag_weightfit) printf(" ** Using weighted fit in SVD\n");
      else printf(" ** Using unweighted fit in SVD\n");
      printf(" ** Using magerr of %2.3f in filtering when matching stars\n",magerrin);
      if(flag_magcut) printf(" ** Using magnitude cut from %2.3f to %2.3f\n",mag_low,mag_high);
      printf(" ** Using -COMBINETYPE %s for SWarp\n",combinetype);
    }
  }

  /* assign the magtype */
  switch(magtype) {
  case 0: sprintf(mag1,"mag_auto_1"); sprintf(mag2,"%s","mag_auto_2"); 
    sprintf(magerr1,"magerr_auto_1"); sprintf(magerr2,"%s","magerr_auto_2"); 
    sprintf(magerr_input,"magerr_auto");
    break; 
  case 1: sprintf(mag1,"mag_aper_1_1"); sprintf(mag2,"%s","mag_aper_1_2"); 
    sprintf(magerr1,"magerr_aper_1_1"); sprintf(magerr2,"%s","magerr_aper_1_2"); 
    sprintf(magerr_input,"magerr_aper_1");
    break; 
  case 2: sprintf(mag1,"mag_aper_2_1"); sprintf(mag2,"%s","mag_aper_2_2"); 
    sprintf(magerr1,"magerr_aper_2_1"); sprintf(magerr2,"%s","magerr_aper_2_2"); 
    sprintf(magerr_input,"magerr_aper_2");
    break; 
  case 3: sprintf(mag1,"mag_aper_3_1"); sprintf(mag2,"%s","mag_aper_3_2"); 
    sprintf(magerr1,"magerr_aper_3_1"); sprintf(magerr2,"%s","magerr_aper_3_2"); 
    sprintf(magerr_input,"magerr_aper_3");
    break; 
  case 4: sprintf(mag1,"mag_aper_4_1"); sprintf(mag2,"%s","mag_aper_4_2"); 
    sprintf(magerr1,"magerr_aper_4_1"); sprintf(magerr2,"%s","magerr_aper_4_2"); 
    sprintf(magerr_input,"magerr_aper_4");
    break; 
  case 5: sprintf(mag1,"mag_aper_5_1"); sprintf(mag2,"%s","mag_aper_5_2"); 
    sprintf(magerr1,"magerr_aper_5_1"); sprintf(magerr2,"%s","magerr_aper_5_2"); 
    sprintf(magerr_input,"magerr_aper_5");
    break; 
  case 6: sprintf(mag1,"mag_aper_6_1"); sprintf(mag2,"%s","mag_aper_6_2"); 
    sprintf(magerr1,"magerr_aper_6_1"); sprintf(magerr2,"%s","magerr_aper_6_2"); 
    sprintf(magerr_input,"magerr_aper_6");
    break; 
  default: printf(" ** %s error: wrong input of <magtype> for -magtype\n", argv[0]); exit(0);
  }

  if(!flag_quiet) {
    if(!flag_quickcoadd) {
      if(!magtype)  printf(" ** Using mag_auto for magnitudes\n"); 
      else printf(" ** Using mag_aper_%d for magnitudes\n",magtype); 
      printf("\n");
    }
  }

  /* grab dblogin */
  select_dblogin(dblogin);


  /****************************************************/
  /* run the select_remaps to get image list and info */
  /****************************************************/
  
  /* run select_remaps if necessary */
  //if(!flag_selectremap) {
  //if(flag_binpath)
  //sprintf(command,"%s/select_remaps",binpath);
  //else
  //sprintf(command,"select_remaps");
  //sprintf(command,"%s %s %s %s %s -type REMAP -coadd -quiet ", 
  //command, project, tilenamein, bandlist, basedir);
  //if(flag_runidlist) sprintf(command, "%s -runid %s", command, runidlist);
  //if(flag_nitelist) sprintf(command, "%s -nite %s", command, nitelist);
  //sprintf(command, "%s > selectremap.out", command);
  //if(!flag_quiet)
  //printf(" ** Running %s\n",command);
  //system(command);
  //}

  /* find out how many images from select_remap */
  //if(!flag_selectremap) 
  //sprintf(command, "wc -l selectremap.out");
  //else
  //sprintf(command, "wc -l %s",selectremapfile);

  //pip=popen(command,"r");
  //while (fgets(line,1000,pip)!=NULL)
  //sscanf(line,"%d %s",&nimage_in,line);
  //pclose(pip);


  /****************************************************/
  /* run DB call to get image list and info */
  /****************************************************/

  sprintf(sqlscript,"%s_remap.sql",tilenamein);
  fsqlout=fopen(sqlscript, "w");
  fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsqlout,"select count(1) from files where ");
  fprintf(fsqlout,"lower(imagetype)='remap' ");
  fprintf(fsqlout,"and band='%s' ",bandlist);
  fprintf(fsqlout,"and tilename='%s' ",tilenamein);
  if(flag_nitelist) {
    fprintf(fsqlout,"and (nite like '%s%%' ",nites_in[0]);
    if(nnite_in>1) 
      for(k=1;k<nnite_in;k++) fprintf(fsqlout,"or nite like '%s%%' ",nites_in[k]);
    fprintf(fsqlout,") ");
  }
  if(flag_runidlist) {
    fprintf(fsqlout,"and (runiddesc like '%s%%' ",runid_in[0]);
    if(nrunid_in>1) 
      for(k=1;k<nrunid_in;k++) fprintf(fsqlout,"or runiddesc like '%s%%' ",runid_in[k]);
    fprintf(fsqlout,") ");
  }
  fprintf(fsqlout,";\n"); 
  fprintf(fsqlout,"select IMAGEID,CCD_NUMBER,BAND,IMAGENAME,NITE,RUNIDDESC from files where ");
  fprintf(fsqlout,"lower(imagetype)='remap' ");
  fprintf(fsqlout,"and band='%s' ",bandlist);
  fprintf(fsqlout,"and tilename='%s' ",tilenamein);
  if(flag_nitelist) {
    fprintf(fsqlout,"and (nite like '%s%%' ",nites_in[0]);
    if(nnite_in>1) 
      for(k=1;k<nnite_in;k++) fprintf(fsqlout,"or nite like '%s%%' ",nites_in[k]);
    fprintf(fsqlout,") ");
  }
  if(flag_runidlist) {
    fprintf(fsqlout,"and (runiddesc like '%s%%' ",runid_in[0]);
    if(nrunid_in>1) 
      for(k=1;k<nrunid_in;k++) fprintf(fsqlout,"or runiddesc like '%s%%' ",runid_in[k]);
    fprintf(fsqlout,") ");
  }
  fprintf(fsqlout,";\n");
  fprintf(fsqlout,"exit;\n");
  fclose(fsqlout);

  /* construct sql call */
  sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);

  i=0;
  pip=popen(sqlcall,"r");
  while (fgets(line,1000,pip)!=NULL) {
    if (!i) {

      /* find out how many images */
      sscanf(line,"%d",&nimage_in);
      
      if(!nimage_in) {
	if(!flag_quiet)
	  printf(" ** %s error: no image found for co-adding, abort\n",argv[0]);
	exit(0);
      }
      else {
	if(!flag_quiet) 
	  printf(" ** Found %d images for co-adding with tile %s\n",nimage_in,tilenamein);
	
	/* memory allocation for the output and initialize keepimg */
	tileinfo_in=(db_tiles *)calloc(nimage_in+1,sizeof(db_tiles));
	keepimg=(int *)calloc(nimage_in+1,sizeof(int));
	for(j=0;j<=nimage_in;j++) keepimg[j]=1;
      }
    }
    else {
      sscanf(line,"%d %d %s %s %s %s",&(tileinfo_in[i].imageid),&(tileinfo_in[i].ccdnum),tileinfo_in[i].band,imagetemp,tileinfo_in[i].nite,tileinfo_in[i].runiddesc);
      sprintf(tileinfo_in[i].imagename,"%s/%s_%02d.%s.fits",imagetemp,imagetemp,tileinfo_in[i].ccdnum,tilenamein);
    }
    i++;
  }
  pclose(pip);

  /* get the info for the tile as well */
  sprintf(sqlscript,"%s_tile.sql",tilenamein);
  fsqlout=fopen(sqlscript, "w");
  fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
  fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
  fprintf(fsqlout,"select RA,DEC,PIXELSIZE,NPIX_RA,NPIX_DEC ");
  fprintf(fsqlout,"from coaddtile where ");
  fprintf(fsqlout,"project='%s' ",project);
  fprintf(fsqlout," and tilename='%s' ",tilenamein);
  fprintf(fsqlout,";\n");
  fprintf(fsqlout,"exit;\n");
  fclose(fsqlout);

  /* construct sql call */
  sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);

  pip=popen(sqlcall,"r");
  fscanf(pip,"%lf %lf %f %d %d",&tile_ra,&tile_dec,&pixelsize,&npix_ra,&npix_dec);
  pclose(pip); 


  /* input the information from the file */
  /* open the output file from select_remap */
  //if(!flag_selectremap)
  //fselectremap=fopen("selectremap.out","r");
  //else
  //fselectremap=fopen(selectremapfile,"r");

  //for(i=1;i<=nimage_in;i++) {
  //fscanf(fselectremap,"%d %s %s %s %s %s %d %d %lg %lg %f %d %d %lg %lg %lg %lg",
  //&(tileinfo_in[i].id),tileinfo_in[i].tilename,tileinfo_in[i].runiddesc,tileinfo_in[i].nite,
  //tileinfo_in[i].band,tileinfo_in[i].imagename,&(tileinfo_in[i].ccdnum),&(tileinfo_in[i].imageid),
  //&(tileinfo_in[i].ra),&(tileinfo_in[i].dec),&(tileinfo_in[i].pixelsize),
  //&(tileinfo_in[i].npix_ra),&(tileinfo_in[i].npix_dec),
  //&(tileinfo_in[i].ra_lo),&(tileinfo_in[i].ra_hi),&(tileinfo_in[i].dec_lo),&(tileinfo_in[i].dec_hi));
  //}

  /* close [and clean up] the output of select_remap */
  //fclose(fselectremap);

  //if(!flag_selectremap) 
  //system ("rm selectremap.out");

  free(nites_in); free(runid_in); 

  if(!flag_quiet) printf("\n");

  
  /***********************************/
  /* various filtering of the images */
  /***********************************/

  /* filtering images with FWHM */
  if(flag_fwhm) {

    /* construct the sql script */
    sprintf(sqlscript,"%s_fwhm.sql",tilenamein);
    fsqlout=fopen(sqlscript, "w");
    fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
    fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    for(i=1;i<=nimage_in;i++) 
      fprintf(fsqlout,"SELECT MNSEEING from files where imageid=%d;\n",tileinfo_in[i].imageid);
    fprintf(fsqlout,"exit;\n");
    fclose(fsqlout);

    /* construct sql call */
    sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);

    pip=popen(sqlcall,"r");
    for(i=1;i<=nimage_in;i++) {
      fgets(line,1000,pip);
      sscanf(line,"%f", &fwhm);
      if(fwhm > fwhm_in) { 
	keepimg[i]=0;
	if(!flag_quiet)
	  printf(" ** image %s/%s/%s/%s not used: FWHM = %2.3f\n",tileinfo_in[i].runiddesc,tileinfo_in[i].nite,
           tileinfo_in[i].band,tileinfo_in[i].imagename,fwhm);
      }
    }
    pclose(pip);
  }

  /* filtering images with EXPTIME */
  if(flag_exptime) {

    /* construct the sql script */
    sprintf(sqlscript,"%s_exptime.sql",tilenamein);
    fsqlout=fopen(sqlscript, "w");
    fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
    fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    for(i=1;i<=nimage_in;i++) 
      fprintf(fsqlout,"SELECT EXPTIME from files where imageid=%d;\n",tileinfo_in[i].imageid);
    fprintf(fsqlout,"exit;\n");
    fclose(fsqlout);

    /* construct sql call */
    sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);

    pip=popen(sqlcall,"r");
    for(i=1;i<=nimage_in;i++) {
      fgets(line,1000,pip);
      sscanf(line,"%f", &exptime);
      if(exptime < exptime_in) { 
	keepimg[i]=0;
	if(!flag_quiet)
	  printf(" ** image %s/%s/%s/%s not used: EXPTIME = %2.3f \n",tileinfo_in[i].runiddesc,tileinfo_in[i].nite,
		 tileinfo_in[i].band,tileinfo_in[i].imagename,exptime);
      }
    }
    pclose(pip);
  }


  /* check the number of images that pass the filterings */
  nimage=0;
  for(i=1;i<=nimage_in;i++) 
    if(keepimg[i]) nimage++;

  if(!flag_quiet)
    printf(" ** %d images used for co-adding (after filtering)\n\n",nimage);
  
  if(!nimage) {
    if(!flag_quiet)
      printf(" ** No images pass the filtering, abort!\n");
    exit(0);
  }
 
  /* memory allocation again */
  tileinfo=(db_tiles *)calloc(nimage+1,sizeof(db_tiles));
  tilerunid=(char **)calloc(nimage+1,sizeof(char *));
  for(j=1;j<=nimage;j++) tilerunid[j]=(char *)calloc(100,sizeof(char ));

  /* input the information from the file */
  j=0;
  for(i=1;i<=nimage_in;i++) {

    if(keepimg[i]) {
      //tileinfo[j+1].id=tileinfo_in[i].id;
      //sprintf(tileinfo[j+1].tilename,"%s",tileinfo_in[i].tilename);
      sprintf(tileinfo[j+1].runiddesc,"%s",tileinfo_in[i].runiddesc);
      sprintf(tileinfo[j+1].nite,"%s",tileinfo_in[i].nite);
      sprintf(tileinfo[j+1].band,"%s",tileinfo_in[i].band);
      sprintf(tileinfo[j+1].imagename,"%s",tileinfo_in[i].imagename);
      tileinfo[j+1].ccdnum=tileinfo_in[i].ccdnum;
      tileinfo[j+1].imageid=tileinfo_in[i].imageid;
      //tileinfo[j+1].ra=tileinfo_in[i].ra;
      //tileinfo[j+1].dec=tileinfo_in[i].dec;
      //tileinfo[j+1].pixelsize=tileinfo_in[i].pixelsize;
      //tileinfo[j+1].npix_ra=tileinfo_in[i].npix_ra;
      //tileinfo[j+1].npix_dec=tileinfo_in[i].npix_dec;
      //tileinfo[j+1].ra_lo=tileinfo_in[i].ra_lo;
      //tileinfo[j+1].ra_hi=tileinfo_in[i].ra_hi;
      //tileinfo[j+1].dec_lo=tileinfo_in[i].dec_lo;
      //tileinfo[j+1].dec_hi=tileinfo_in[i].dec_hi;
    
      /* get the runid as well */
      sprintf(temp,"%s",tileinfo_in[i].runiddesc);
      len=strlen(temp);
      for (m=0;m<len;m++) 
	if (!strncmp(&(temp[m]),"_",1)) {
	  temp[m]=0;
	  break;
	}
      sscanf(temp,"%s",tilerunid[j+1]);

      /* update counter */
      j++;
    }
  }
  
  /* output the query results */
  if(!flag_quiet) {
    printf("\tImageID\tImagePath\n");
    for(i=1;i<=nimage;i++) 
      printf("%d\t%d\t%s/data/%s/%s/%s\n",
	     i,tileinfo[i].imageid,tileinfo[i].runiddesc,tileinfo[i].nite,
	     tileinfo[i].band,tileinfo[i].imagename);
  }


  /**************************************************************/
  /* query zeropoint table to get the latest zp for each images */
  /**************************************************************/

  /* set memory allocation */  
  zp_latest=(db_zeropoint *)calloc(nimage+1,sizeof(db_zeropoint));
  
  if(!flag_nophotozp) {

    /* construct the sql script */
    sprintf(sqlscript,"%s_%s_zp.sql",tilenamein, bandlist);
    fsqlout=fopen(sqlscript, "w");
    fprintf(fsqlout,"SET NEWP 0 SPA 1 PAGES 0 FEED OFF ");
    fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    //fprintf(fsqlout,"SELECT zeropoint.zp_n,zeropoint.image_n,zeropoint.mag_zero,zeropoint.sigma_zp,zeropoint.b,zeropoint.berr ");
    //fprintf(fsqlout,"FROM zeropoint,files WHERE ");
    for(i=1;i<=nimage;i++) {
      fprintf(fsqlout,"SELECT zeropoint.zp_n,zeropoint.image_n,zeropoint.mag_zero,zeropoint.sigma_zp,zeropoint.b,zeropoint.berr ");
      fprintf(fsqlout,"FROM zeropoint,files WHERE ");
      fprintf(fsqlout,"files.imageid=zeropoint.image_n and files.photflag=1 and ");
      fprintf(fsqlout,"zeropoint.image_n=%d order by zeropoint.zp_n;\n",tileinfo[i].imageid);
    }
    //fprintf(fsqlout,"zeropoint.image_n=%d ORDER BY zeropoint.image_n;\n",tileinfo[nimage].imageid);
    fprintf(fsqlout,"exit;\n");
    fclose(fsqlout);

    /* construct sql call */
    sprintf(sqlcall, "${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);
    
    /* find out how many zp returned */  
    sprintf(command, "%s | wc -l",sqlcall);
    pip=popen(command,"r");
    while (fgets(line,1000,pip)!=NULL)
      sscanf(line,"%d %s",&nzpin,line);
    pclose(pip);
    
    if(!flag_quiet) 
      printf("\n ** Found %d zp from zeropoint table\n\n",nzpin);

    /* update number of constrains */
    //if(nzp)
    //nconstrain+=nzp;
    //else
    //nconstrain+=1;
  
    /* set memory allocation */  
    zp=(db_zeropoint *)calloc(nzpin+1,sizeof(db_zeropoint));

    /* initialize arrays */
    for(i=0;i<=nimage;i++) {
      zp_latest[i].mag_zero=0.0; 
      zp_latest[i].sigma_zp=0.0;
    }

    /* input the zp info */
    pip=popen(sqlcall,"r"); i=1; 
    while (fgets(line,1000,pip)!=NULL) {
      sscanf(line,"%d %d %f %f %f %f",&zp[i].zp_n,&zp[i].imageid,&zp[i].mag_zero,&zp[i].sigma_zp,&zp[i].b,&zp[i].berr);
      i++;
    }
    pclose(pip);

    /* loop over to find the latest zp for each images */
    nzp=0;
    for(i=1;i<=nimage;i++) {
      
      flag_zpmax=0;
      for(j=1;j<=nzpin;j++) {
	
	if(tileinfo[i].imageid == zp[j].imageid) {
	  endloop=j;
	  
	  /* first assign the maxzp with first zp */
	  if(!flag_zpmax) {
	    maxzp=zp[j].zp_n;
	    zp_latest[i].mag_zero=zp[j].mag_zero;
	    zp_latest[i].sigma_zp=zp[j].sigma_zp;
	    flag_zpmax=1;
	    nzp++;
	  }
	  else { /* find the latest zp using largest zp_n */
	    if(zp[j].zp_n > maxzp) { 
	      maxzp=zp[j].zp_n;
	      zp_latest[i].mag_zero=zp[j].mag_zero;
	      zp_latest[i].sigma_zp=zp[j].sigma_zp;
	    }
	  } 
	} // if(imageid) loop

	/* quit the j-loop after finding the latest zp */
	if(j > endloop && flag_zpmax) break;
      } // j loop
    } 

    if(!flag_quiet) {
      if(nzp) {
	printf(" ** %d images have zero-points from database\n",nzp);
	printf(" ** The (latest) zero-points for the images:\n");
	for(i=1;i<=nimage;i++) {
	  if(zp_latest[i].mag_zero)
	    printf("%d\t%s/data/%s/%s/%s\t%2.4f +- %2.4f\n",i,tileinfo[i].runiddesc,tileinfo[i].nite,
		   tileinfo[i].band,tileinfo[i].imagename,zp_latest[i].mag_zero,zp_latest[i].sigma_zp);
	  else {
	    zp_latest[i].mag_zero=25.0;
	    printf("%d\t%s/data/%s/%s/%s\t%2.4f +- %2.4f\n",i,tileinfo[i].runiddesc,tileinfo[i].nite,
		   tileinfo[i].band,tileinfo[i].imagename,zp_latest[i].mag_zero,zp_latest[i].sigma_zp);
	  }
	}
	printf("\n");
      }
      else 
	printf(" ** Setting the ZP for the first image to be 25.0\n");
    }
  }
  else 
    printf("\n ** Setting the ZP for first image to be 25.0\n");
  

  /********************************************/
  /* find the overlap images for a given tile */
  /********************************************/

  /* memory allocation */
  saveid1=(int *)calloc(nimage*(nimage-1)/2,sizeof(int));
  saveid2=(int *)calloc(nimage*(nimage-1)/2,sizeof(int));
  save_mag_mean = dmatrix(0, nimage, 0, nimage);
  initialize_dmatrix(save_mag_mean, nimage, nimage);
  save_mag_rms = dmatrix(0, nimage, 0, nimage);
  initialize_dmatrix(save_mag_rms, nimage, nimage);
  flag_mag_mean = imatrix(0, nimage, 0, nimage);
  initialize_imatrix(flag_mag_mean, nimage, nimage);

  ra1=(double *)calloc(nimage+1,sizeof(double));
  ra2=(double *)calloc(nimage+1,sizeof(double));
  ra3=(double *)calloc(nimage+1,sizeof(double));
  ra4=(double *)calloc(nimage+1,sizeof(double));
  dec1=(double *)calloc(nimage+1,sizeof(double));
  dec2=(double *)calloc(nimage+1,sizeof(double));
  dec3=(double *)calloc(nimage+1,sizeof(double));
  dec4=(double *)calloc(nimage+1,sizeof(double));

  if(!flag_nostarmatch) {

    sprintf(sqlscript,"%s_%s_getremapimg.sql",tilenamein, bandlist);

    /* calling database to get WCS info for remap images */
    sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);
  
    fsqlout=fopen(sqlscript,"w");
    fprintf(fsqlout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");       
    fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    for(i=1;i<=nimage;i++) 
      fprintf(fsqlout,"SELECT IMAGEID,NPIX1,NPIX2,CTYPE1,CTYPE2,CRVAL1,CRVAL2,CRPIX1,CRPIX2,CD1_1,CD1_2,CD2_1,CD2_2 FROM Files WHERE imageid=%d;\n",
	      tileinfo[i].imageid);
    fprintf(fsqlout,"exit;\n ");  
    fclose(fsqlout);
    
    if(!flag_quiet) {
      printf("\n --------------------------------------------------------------\n");
      printf("\tImage(A)                            \n");
      printf("\tImage(B)                            \n");
      printf("\tNtotal delta(mag)=Image(A)-Image(B) RMS Ngood Nbad Niter.\n");
      printf(" --------------------------------------------------------------\n");	   
    }

    /* getting the 4 corners of the images */
    pip=popen(sqlcall,"r");
    for(i=1;i<=nimage;i++) {
      fgets(line,1000,pip);
      sscanf(line,"%d %ld %ld %s %s %lg %lg %lg %lg %lg %lg %lg %lg", &imageid_temp,
	     &axes0,&axes1,ctype1,ctype2,&crval1,&crval2,&crpix1,&crpix2,
	     &cd1_1,&cd1_2,&cd2_1,&cd2_2);

      if(imageid_temp == tileinfo[i].imageid) {  /* check imageid */
	if(!strcmp(ctype1,"RA---TAN") && !strcmp(ctype2,"DEC--TAN")) { /* check ctype keyword */
	  /* getting the four courners here */
	  
	  /* evaluate rho_a and rho_b as in Calabretta & Greisen (2002), eq 191 */
	  if(cd2_1>0) rho_a=atan(cd2_1/cd1_1);
	  else if(cd2_1<0) rho_a=atan(-cd2_1/-cd1_1);
	  else rho_a=0.0;
	
	  if(cd1_2>0) rho_b=atan(cd1_2/-cd2_2);
	  else if(cd1_2<0) rho_b=atan(-cd1_2/cd2_2);
	  else rho_b=0.0;
	  
	  if(fabs(rho_a-rho_b) < ACCURACY) {
	  
	    /* evaluate rho and CDELTi as in Calabretta & Greisen (2002), eq 193 */
	    rho=0.5*(rho_a+rho_b);
	    cdelt1=cd1_1/cos(rho);
	    cdelt2=cd2_2/cos(rho);
	    
	    crota2=rho*(180.0/M_PI);
            
	    
	    /* get the 4 corners using cfitsio subroutine */
	    /* lower left corner, ra1/dec1 */
	    status=0;
	    xpix=0.0; ypix=0.0;
	    fits_pix_to_world(xpix,ypix,crval1,crval2,crpix1,crpix2,cdelt1,cdelt2,crota2,"-TAN",&ra_tem,&dec_tem,&status);
	    ra1[i]=ra_tem; dec1[i]=dec_tem;
	    //printf("%d\t1:%2.8f %2.8f\t",i,ra_tem,dec_tem);
	    
	    /* upper left corner, ra2/dec2 */
	    status=0;
	    xpix=0.0; ypix=(double)axes1+0.5;
	    fits_pix_to_world(xpix,ypix,crval1,crval2,crpix1,crpix2,cdelt1,cdelt2,crota2,"-TAN",&ra_tem,&dec_tem,&status);
	    ra2[i]=ra_tem; dec2[i]=dec_tem;
	    //printf("2:%2.8f %2.8f\t",ra_tem,dec_tem);

	    /* lower right corner, ra3/dec3 */
	    status=0;
	    xpix=(double)axes0+0.5; ypix=0.0;
	    fits_pix_to_world(xpix,ypix,crval1,crval2,crpix1,crpix2,cdelt1,cdelt2,crota2,"-TAN",&ra_tem,&dec_tem,&status);
	    ra3[i]=ra_tem; dec3[i]=dec_tem;
	    //printf("3:%2.8f %2.8f\t",ra_tem,dec_tem);

	    /* upper right corner, ra4/dec4 */
	    status=0;
	    xpix=(double)axes0+0.5; ypix=(double)axes1+0.5;
	    fits_pix_to_world(xpix,ypix,crval1,crval2,crpix1,crpix2,cdelt1,cdelt2,crota2,"-TAN",&ra_tem,&dec_tem,&status);
	    ra4[i]=ra_tem; dec4[i]=dec_tem;
	    //printf("4:%2.8f %2.8f\n",ra_tem,dec_tem);

	    /* !!add checking of ra range? */
	    	    
	  }	    
	  else printf(" ** %s error: solutions of rotation angle do not converge for imageid = %d\n",argv[0],tileinfo[i].imageid);
	}
	else printf(" ** %s error: image with imageid = %d is not in -TAN projection\n",argv[0],tileinfo[i].imageid);
      }
      else printf(" ** %s error: image with imageid = %d does not have WCS data in Files table\n",argv[0],tileinfo[i].imageid);
    }
    pclose(pip);
  
    /* setup sql script */
    sprintf(sqlscript,"%s_%s_matchstar.sql",tilenamein, bandlist);
    sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);
  
    /* loops over the images to find the overlap images and construct sql call */
    count=0;
    for(i=1;i<=nimage-1;i++) { 

      maxra=ra1[i];
      if(ra2[i]>maxra) maxra=ra2[i];
      if(ra3[i]>maxra) maxra=ra3[i];
      if(ra4[i]>maxra) maxra=ra4[i];

      minra=ra1[i];
      if(ra2[i]<minra) minra=ra2[i];
      if(ra3[i]<minra) minra=ra3[i];
      if(ra4[i]<minra) minra=ra4[i];
      
      maxdec=dec1[i];
      if(dec2[i]>maxdec) maxdec=dec2[i];
      if(dec3[i]>maxdec) maxdec=dec3[i];
      if(dec4[i]>maxdec) maxdec=dec4[i];

      mindec=dec1[i];
      if(dec2[i]<mindec) mindec=dec2[i];
      if(dec3[i]<mindec) mindec=dec3[i];
      if(dec4[i]<mindec) mindec=dec4[i];

    
      for(j=i+1;j<=nimage;j++) {
	flag=0;


	/* compare 4 corners */
	if (ra1[j]>=minra && ra1[j]<=maxra && dec1[j]>=mindec && dec1[j]<=maxdec) flag=1;
	if (ra2[j]>=minra && ra2[j]<=maxra && dec2[j]>=mindec && dec2[j]<=maxdec) flag=2;
	if (ra3[j]>=minra && ra3[j]<=maxra && dec3[j]>=mindec && dec3[j]<=maxdec) flag=3;
	if (ra4[j]>=minra && ra4[j]<=maxra && dec4[j]>=mindec && dec4[j]<=maxdec) flag=4;

	/* match two images */
	if(flag) {
	  
	  /* save the id for matched images */
	  saveid1[count]=tileinfo[i].id;
	  saveid2[count]=tileinfo[j].id;

	  /* memory allocation and initialize for data arrays */
	  datain=(double *)calloc(MAX+1,sizeof(double));
	  errin=(double *)calloc(MAX+1,sizeof(double));
	  
	  initialize_dvector(datain,MAX);
	  initialize_dvector(errin,MAX);
	  
	  /* sql script for Dora's stored procedure */
	  fsqlout=fopen(sqlscript,"w");
	  fprintf(fsqlout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");       
	  fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");

	  sprintf(command," from table(fMatchImages(%d, %d, %2.3f, %2.3f, %d, %2.4f, \'%s\', %2.3f));\n", 
	  tileinfo[i].imageid,tileinfo[j].imageid,class_star_lo, class_star_hi, flag_se, radius, magerr_input, magerrin);
	  fprintf(fsqlout,"SELECT %s,%s,%s,%s %s", mag1,magerr1,mag2,magerr2,command);
	  fprintf(fsqlout,"exit;\n ");  
	  fclose(fsqlout); 

	  /* database call to get matched stars */
	  pip=popen(sqlcall, "r");
	  newcountstar=0;
	  while (fgets(line,1000,pip)!=NULL) {
	    sscanf(line,"%f %f %f %f",&magout1,&magerrout1,&magout2,&magerrout2); 
	    
	    if(flag_magcut) {
	      if((magout1 >= mag_low && magout1 <= mag_high) ||  (magout2 >= mag_low && magout2 <= mag_high)) {
		if(!zp_latest[i].mag_zero || !zp_latest[j].mag_zero) {
		  datain[newcountstar]=(magout1)-(magout2);
		  errin[newcountstar] =sqrt(Squ(magerrout1)+Squ(magerrout2)); // need to update later for including zp_error
		  newcountstar++;
		}
		else {
		  if(fabs((magout1-zp_latest[i].mag_zero)-(magout2-zp_latest[j].mag_zero)) < MAXMAGDIFF) {
		    datain[newcountstar]=(magout1-zp_latest[i].mag_zero)-(magout2-zp_latest[j].mag_zero);
		    errin[newcountstar] =sqrt(Squ(magerrout1)+Squ(magerrout2)); // need to update later for including zp_error
		    newcountstar++;
		  }
		}
	      }
	    }
	    else {
	      if(!zp_latest[i].mag_zero || !zp_latest[j].mag_zero) {
		datain[newcountstar]=(magout1)-(magout2);
		errin[newcountstar] =sqrt(Squ(magerrout1)+Squ(magerrout2)); // need to update later for including zp_error
		newcountstar++;
	      }
	      else {
		if(fabs((magout1-zp_latest[i].mag_zero)-(magout2-zp_latest[j].mag_zero)) < MAXMAGDIFF) {
		  datain[newcountstar]=(magout1-zp_latest[i].mag_zero)-(magout2-zp_latest[j].mag_zero);
		  errin[newcountstar] =sqrt(Squ(magerrout1)+Squ(magerrout2)); // need to update later for including zp_error
		  newcountstar++;
		}
	      }
	    }
	    
	    if(newcountstar == MAX) {
	      printf(" ** number of matched stars exceed %d for imageid %d and %d\n",MAX,tileinfo[i].imageid,tileinfo[j].imageid);
	      exit(0);
	    }
	  }
	  pclose(pip);


	  flag_mean=1;
	  /* calculate the average and rms of mag. difference here */
	  if(flag_Nstarmatch) {
	    if(newcountstar>=Nmin_starmatch)
	      iter_mean(datain,errin,newcountstar,&mag_mean,&magrms_mean,&N_good,&N_bad,&N_iter,flag_weight,flag_iter,flag_Niterate,Nmax_iterate,sigma);
	  }
	  else {
	    if(newcountstar>1)
	      iter_mean(datain,errin,newcountstar,&mag_mean,&magrms_mean,&N_good,&N_bad,&N_iter,flag_weight,flag_iter,flag_Niterate,Nmax_iterate,sigma);
	    else if(newcountstar==1){
	      mag_mean=datain[0];
	      //magrms_mean=0.0;
	      magrms_mean=errin[0];
	      N_good=1;N_bad=N_iter=0;
	    }
	    else {
	      mag_mean=magrms_mean=0.0;
	      N_good=N_bad=N_iter=0;
	      flag_mean=0;
	    }     
	  }

	  
	  //if(fabs(mag_mean) > MAXMAGDIFF)
	  //flag_mean=0;

	  if(!flag_quiet) {
	    if(flag_mean && magrms_mean<magrmscut) {

	      save_mag_mean[i][j]=-mag_mean;
	      //save_mag_mean[j][i]=-mag_mean;
	      
	      save_mag_rms[i][j]=magrms_mean;
	      //save_mag_rms[j][i]=-magrms_mean;

	      flag_mag_mean[i][j]=1;

	      printf("%d\t%s/data/%s/%s/%s\n",i,tileinfo[i].runiddesc,tileinfo[i].nite,tileinfo[i].band,tileinfo[i].imagename);
	      printf("%d\t%s/data/%s/%s/%s\n",j,tileinfo[j].runiddesc,tileinfo[j].nite,tileinfo[j].band,tileinfo[j].imagename);
	      printf("\t%d\t%2.5f\t%2.5f\t%d\t%d\t%d\n",newcountstar,save_mag_mean[i][j],magrms_mean,N_good,N_bad,N_iter);
	      fflush(stdout);
	      newcount++;	
	    }
	  }
	    
	  count++;
	  free(datain); free(errin);
	}
      }
    }

    if(!flag_quiet) {
      printf("\n ** Initially found %d pairs overlapped images\n",count);
      printf(" ** Found %d pairs of overlapped images with common stars\n",newcount);
    }

    //nconstrain+=newcount;
  }


  /*********************************************/
  /* Obtain SKYBRITE information for each CCDs */
  /*********************************************/

  /* memory allocation and initialization */
  save_skybrite = dmatrix(0, nimage, 0, nimage);
  save_skybriteerr = dmatrix(0, nimage, 0, nimage);
  flag_skybrite = imatrix(0, nimage, 0, nimage);
  skybrite_ccdpair = dmatrix(0, ccdtotal+1, 0,ccdtotal+1);
  skybriteerr_ccdpair = dmatrix(0, ccdtotal+1, 0,ccdtotal+1);

  skybrite=(float *)calloc(nimage+1,sizeof(float)); 

  runidnites=(char **)calloc(nimage+1,sizeof(char *));
  for(j=0;j<=nimage;j++) runidnites[j]=(char *)calloc(100,sizeof(char ));
  nites=(char **)calloc(nimage+1,sizeof(char *));
  for(j=0;j<=nimage;j++) nites[j]=(char *)calloc(100,sizeof(char ));
  runids=(char **)calloc(nimage+1,sizeof(char *));
  for(j=0;j<=nimage;j++) runids[j]=(char *)calloc(100,sizeof(char ));
  bands=(char **)calloc(nimage+1,sizeof(char *));
  for(j=0;j<=nimage;j++) bands[j]=(char *)calloc(100,sizeof(char ));

  initialize_dmatrix(save_skybrite, nimage, nimage);
  initialize_vector(skybrite,nimage);
  initialize_imatrix(flag_skybrite, nimage, nimage);

  if(!flag_noskybrite) {

    /* find out the distinct runid_nites */
    Nrunidnite=1; 
    sprintf(runidnites[1],"%s",tileinfo[1].runiddesc);
    
    for(i=2;i<=nimage;i++) {
      flag=0;
      for(j=1;j<=Nrunidnite;j++) {    
	if(!strcmp(tileinfo[i].runiddesc,runidnites[j])) 
	  flag=1;
      }
      if(!flag) {
	Nrunidnite++;
	sprintf(runidnites[Nrunidnite],"%s",tileinfo[i].runiddesc);
      }
    }
    
    if(!flag_quiet) {
      printf("\n ** Found %d distinct runid_nites: ",Nrunidnite);
      for(i=1;i<=Nrunidnite;i++) printf("%s ",runidnites[i]);
      printf("\n");
    }

    /* find out the distinct runid */
    Nrunid=1; 
    sprintf(runids[1],"%s",tilerunid[1]);
    
    for(i=2;i<=nimage;i++) {
      flag=0;
      for(j=1;j<=Nrunid;j++) {    
	if(!strcmp(tilerunid[i],runids[j])) 
	  flag=1;
      }
      if(!flag) {
	Nrunid++;
	sprintf(runids[Nrunid],"%s",tilerunid[i]);
      }
    }
    
    if(!flag_quiet) {
      printf("\n ** Found %d distinct runid: ",Nrunid);
      for(i=1;i<=Nrunid;i++) printf("%s ",runids[i]);
      printf("\n");
    }

    /* find out the distinct nites */
    Nnite=1; 
    sprintf(nites[1],"%s",tileinfo[1].nite);
    
    for(i=2;i<=nimage;i++) {
      flag=0;
      for(j=1;j<=Nnite;j++) {    
	if(!strcmp(tileinfo[i].nite,nites[j])) 
	  flag=1;
      }
      if(!flag) {
	Nnite++;
	sprintf(nites[Nnite],"%s",tileinfo[i].nite);
      }
    }
    
    if(!flag_quiet) {
      printf("\n ** Found %d distinct nites: ",Nnite);
      for(i=1;i<=Nnite;i++) printf("%s ",nites[i]);
      printf("\n");
    }

    /* find out the distinct bands */
    Nband=1; 
    sprintf(bands[1],"%s",tileinfo[1].band);
    
    for(i=2;i<=nimage;i++) {
      flag=0;
      for(j=1;j<=Nband;j++) {    
	if(!strcmp(tileinfo[i].band,bands[j])) 
	  flag=1;
      }
      if(!flag) {
	Nband++;
	sprintf(bands[Nband],"%s",tileinfo[i].band);
      }
    }

    //if(!flag_quiet) {
    //printf("\n ** Found %d distinct bands: ",Nband);
    //for(i=1;i<=Nband;i++) printf("%s ",bands[i]);
    //printf("\n");
    //}

    /* cycle through the distinct nites to get SKYBRITE values */
    sprintf(sqlscript,"%s_%s_skybrite.sql",tilenamein, bandlist);
    sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);
    
    /* database query for all exposures for a given nite/runid/band */
    fsqlout=fopen(sqlscript,"w");
    fprintf(fsqlout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");
    fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
    for(i=1;i<=nimage;i++)
      fprintf(fsqlout,"SELECT SKYBRITE from files where imageid=%d;\n",tileinfo[i].imageid);
    fprintf(fsqlout,"exit;\n ");
    fclose(fsqlout);
    
    if(!flag_quiet) 
      printf("\n ** Skybrite values from DB for each images:\n");
    
    pip=popen(sqlcall,"r");
    for(i=1;i<=nimage;i++) {
      fgets(line,1000,pip);
      sscanf(line,"%lg",&skybrite_temp);
      if(!flag_quiet)
	printf("%d\t%s/data/%s/%s/%s\t%2.4f(ADU)\n",i,tileinfo[i].runiddesc,
	       tileinfo[i].nite,tileinfo[i].band,tileinfo[i].imagename,skybrite_temp);
    }
    pclose(pip);
    
    if(!flag_quiet) printf("\n");
    
    if(flag_nitelist) Nrun=Nnite;
    else Nrun=Nrunidnite;

    /* database query for all exposures for all nite/runid/band */
    for(i=1;i<=Nrun;i++) {

      if(flag_nitelist) 
	sprintf(sqlscript,"%s_%s_%s.sql",tilenamein, bandlist,nites[i]);  
      else
	sprintf(sqlscript,"%s_%s_%s.sql",tilenamein, bandlist,runidnites[i]);
  
      sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);

      /* count how many exposures in a given nite/runid/band */
      fsqlout=fopen(sqlscript,"w");
      fprintf(fsqlout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");       
      fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
      fprintf(fsqlout,"SELECT distinct(imagename) from files where imagetype='reduced' ");
      if(flag_nitelist) 
	fprintf(fsqlout," AND nite='%s' ",nites[i]);
      else
	fprintf(fsqlout," AND runiddesc='%s' ",runidnites[i]);
      for(j=1;j<=Nband;j++) 
	fprintf(fsqlout,"AND band='%s' ",bands[j]);
      fprintf(fsqlout,";\n");
      fprintf(fsqlout,"exit;\n ");  
      fclose(fsqlout); 
      
      if(!flag_quiet) {
	if(flag_nitelist) printf(" ** Exposure list for %s:\n",nites[i]);
	else printf(" ** Exposure list for %s:\n",runidnites[i]);
      }

      pip=popen(sqlcall,"r"); Nexposure=0;
      while (fgets(line,1000,pip)!=NULL) {
	sscanf(line,"%s",temp);
	if(!flag_quiet) printf("\t%s\n",temp);
	Nexposure++;
      }
      pclose(pip);
      
      skybrite_bookeep = dmatrix(0, ccdtotal, 0, Nexposure);
      initialize_dmatrix(skybrite_bookeep,ccdtotal, Nexposure);
      
      /* query to get the info */
      if(flag_nitelist) 
	sprintf(sqlscript,"%s_%s_skybrite_%s.sql",tilenamein, bandlist,nites[i]); 
      else
	sprintf(sqlscript,"%s_%s_skybrite_%s.sql",tilenamein, bandlist,runidnites[i]);  
     
      sprintf(sqlcall,"${ORACLE_HOME}/bin/sqlplus -S %s < %s",dblogin,sqlscript);

      fsqlout=fopen(sqlscript,"w");
      fprintf(fsqlout,"SET ECHO OFF NEWP 0 SPA 1 PAGES 0 FEED OFF ");       
      fprintf(fsqlout,"HEAD OFF TRIMS ON LINESIZE 1000;\n");
      fprintf(fsqlout,"SELECT imagename,ccd_number,band,SKYBRITE from files where imagetype='reduced' ");
      if(flag_nitelist) 
	fprintf(fsqlout," AND nite='%s' ",nites[i]);
      else
	fprintf(fsqlout," AND runiddesc='%s' ",runidnites[i]);
      for(j=1;j<=Nband;j++) 
	fprintf(fsqlout,"AND band='%s' ",bands[j]);
      fprintf(fsqlout,"order by imagename;\n");
      fprintf(fsqlout,"exit;\n ");  
      fclose(fsqlout); 
    
      /* worry about band later */
      pip=popen(sqlcall,"r");
      fgets(line,1000,pip); 
      sscanf(line,"%s %d %s %lg",prev,&ccdnum,bandtemp,&skybrite_temp);
      pclose(pip);
      
      pip=popen(sqlcall,"r");
      j=1;
      while (fgets(line,1000,pip)!=NULL) {
	sscanf(line,"%s %d %s %lg",temp,&ccdnum,bandtemp,&skybrite_temp);
	
	if(!strcmp(temp,prev))
	  skybrite_bookeep[ccdnum][j]=skybrite_temp;
	else {
	  j++;
	  sprintf(prev,"%s",temp);
	  skybrite_bookeep[ccdnum][j]=skybrite_temp;
	}
      }
      pclose(pip);
     
      /* check if SKYBRITE=0 or not for given runid_nite */
      flag=0;
      for(j=1;j<=ccdtotal;j++) 
	for(k=1;k<=Nexposure;k++) 
	  if(skybrite_bookeep[j][k]==0) flag=1;
       
      if(flag) {
	if(!flag_quiet) printf(" ** No SKYBRITE ratio available for %s (SKYBRITE value=0)\n",runidnites[i]);
      }
      else {
	/* get the median of ccdratio */      
	calc_ccdratio(skybrite_bookeep,skybrite_ccdpair,skybriteerr_ccdpair,ccdtotal,Nexposure);
	
	/* print out result */
	if(!flag_quiet) {
	  if(flag_nitelist)
	     printf("\n ** SKYBRITE ratio for %s from %d exposures\n",nites[i],Nexposure);
	  else
	    printf("\n ** SKYBRITE ratio for %s from %d exposures\n",runidnites[i],Nexposure);
	
	  printf("--------------------------------------------------------------\n");
	  printf("\tImage(A)                            \n");
	  printf("\tImage(B)                            \n");
	  printf("\tdelta_mag=Image(A)-Image(B) \n");
	  printf(" --------------------------------------------------------------\n");
	}
      
	/* assign the ratio to images (worry the band match later) */  
	track=0;
	for(j=1;j<=nimage-1;j++) {
	  for(k=(j+1);k<=nimage;k++) {
	    	    
	    if(flag_nitelist) {

	      sprintf(temp,"data/%s/%s/%s",tileinfo[j].nite,tileinfo[j].band,tileinfo[j].imagename);
	      len=strlen(temp);
	      for (m=len;m>0;m--) 
		if (!strncmp(&(temp[m]),"/",1)) {
		  temp[m]=0;
		  break;
		}
	      sscanf(temp,"%s",image1);
	      
	      sprintf(temp,"data/%s/%s/%s",tileinfo[k].nite,tileinfo[k].band,tileinfo[k].imagename);
	      len=strlen(temp);
	      for (m=len;m>0;m--) 
		if (!strncmp(&(temp[m]),"/",1)) {
		  temp[m]=0;
		  break;
		}
	      sscanf(temp,"%s",image2);

	      /* check if from same exposure and same nite */
	      if(!strcmp(image1,image2) && !strcmp(nites[i],tileinfo[j].nite) && !strcmp(nites[i],tileinfo[k].nite)) {

		save_skybrite[j][k]=skybrite_ccdpair[tileinfo[j].ccdnum][tileinfo[k].ccdnum];		
		save_skybriteerr[j][k]*=(2.5/log(10.0)/save_skybrite[j][k]);
		save_skybrite[j][k]=2.5*log10(save_skybrite[j][k]);

		flag_skybrite[j][k]=1;

		if(!flag_quiet) {
		  printf("For exposure %s\n",image1);
		  printf("%d\t%s/data/%s/%s/%s\n",j,tileinfo[j].runiddesc,tileinfo[j].nite,tileinfo[j].band,tileinfo[j].imagename);
		  printf("%d\t%s/data/%s/%s/%s\n",k,tileinfo[k].runiddesc,tileinfo[k].nite,tileinfo[k].band,tileinfo[k].imagename);
		  printf("\t %2.4f +- %2.4f  Fluxratio=%2.4f\n",save_skybrite[j][k],save_skybriteerr[j][k],skybrite_ccdpair[tileinfo[j].ccdnum][tileinfo[k].ccdnum]);
		}
		track++;
	      }
	    }
	    else {

	      sprintf(temp,"%s/data/%s/%s/%s",tileinfo[j].runiddesc,tileinfo[j].nite,tileinfo[j].band,tileinfo[j].imagename);
	      len=strlen(temp);
	      for (m=len;m>0;m--) 
		if (!strncmp(&(temp[m]),"/",1)) {
		  temp[m]=0;
		  break;
		}
	      sscanf(temp,"%s",image1);
	  
	      sprintf(temp,"%s/data/%s/%s/%s",tileinfo[k].runiddesc,tileinfo[k].nite,tileinfo[k].band,tileinfo[k].imagename);
	      len=strlen(temp);
	      for (m=len;m>0;m--) 
		if (!strncmp(&(temp[m]),"/",1)) {
		  temp[m]=0;
		  break;
		}
	      sscanf(temp,"%s",image2);
	      
	      /* check if from same exposure and same runid_nite */
	      if(!strcmp(image1,image2) && !strcmp(runidnites[i],tileinfo[j].runiddesc) && !strcmp(runidnites[i],tileinfo[k].runiddesc)) {
	 
		save_skybrite[j][k]=skybrite_ccdpair[tileinfo[j].ccdnum][tileinfo[k].ccdnum];
		save_skybriteerr[j][k]*=(2.5/log(10.0)/save_skybrite[j][k]);
		save_skybrite[j][k]=2.5*log10(save_skybrite[j][k]);

		flag_skybrite[j][k]=1;

		if(!flag_quiet) {
		  printf("For exposure %s\n",image1);
		  printf("%d\t%s/data/%s/%s/%s\n",j,tileinfo[j].runiddesc,tileinfo[j].nite,tileinfo[j].band,tileinfo[j].imagename);
		  printf("%d\t%s/data/%s/%s/%s\n",k,tileinfo[k].runiddesc,tileinfo[k].nite,tileinfo[k].band,tileinfo[k].imagename);
		  printf("\t %2.4f +- %2.4f  Fluxratio=%2.4f\n",save_skybrite[j][k],save_skybriteerr[j][k],skybrite_ccdpair[tileinfo[j].ccdnum][tileinfo[k].ccdnum]);
		}
		track++;
	      } 
	    } // if(flag_nitelist) loop

	  } // k-loop
	} //j-loop

	if(!track) {
	  if(!flag_quiet) {
	    if(flag_nitelist) printf(" ** %s warning: images are not from the same exposures for %s\n",argv[0],nites[i]);
	    else printf(" ** %s warning: images are not from the same exposures for %s\n",argv[0],runidnites[i]);
	  }
	}
      }
      if(!flag_quiet) printf("\n");
    
      free_dmatrix(skybrite_bookeep,0,ccdtotal,0,Nexposure);
    }
  }


  /*************************/
  /* construct the matrix  */
  /*************************/

  if(flag_nophotozp==1 || nzp==0) {
    nzp=1; 
    zp_latest[1].mag_zero=25.0; 	
    zp_latest[1].sigma_zp=1.0;
  }

  ncol=nimage;
  //nrow=nimage+nconstrain;

  /* get the nrow */
  for(i=1;i<=nimage;i++) {
    for(j=1;j<=nimage;j++) { 
      if(flag_mag_mean[i][j]) 
	nstarmatch++;
      if(flag_skybrite[i][j]) 
	nskybrite++;
    }
  }

  nrow=nzp+nstarmatch+nskybrite;
  
  if(!flag_quiet) 
    printf(" ** Number of constrains: N(zp) = %d\t N(matched_stars) = %d\t N(skybrite) = %d\n",nzp,nstarmatch,nskybrite);

  /* memory allocation and initialization for matrix/vector eqn */
  A = matrix(0, nrow, 0, ncol);
  initialize_matrix(A, nrow, ncol);  
  
  b=(float *)calloc(nrow+1,sizeof(float));
  initialize_vector(b, nrow);

  berr=(float *)calloc(nrow+1,sizeof(float));
  for(i=0;i<=nrow;i++) berr[i]=1.0;

  x=(float *)calloc(ncol+1,sizeof(float));
  initialize_vector(x, ncol);
    
  xerr=(float *)calloc(ncol+1,sizeof(float));
  initialize_vector(xerr, ncol);

  if(!flag_quickcoadd) {

  /* !! include the weigh here, alter to Glazebrook expression? !! */
  /* assign the A matrix and b vector with overlap & SKYBRITE information */
    
    /* for the ZP info */
    jstart=1;
    for(i=1;i<=nzp;i++)
      for(j=jstart;j<=nimage;j++) 
	if(zp_latest[j].mag_zero!=0.0 || zp_latest[j].mag_zero!=25.0) {

	  if(flag_weightfit) {
	    weight_temp=1.0;
	    A[i][i]=1.0/weight_temp;
	    b[i]=zp_latest[j].mag_zero/weight_temp;
	  }
	  else {
	    A[i][i]=1.0;
	    b[i]=zp_latest[j].mag_zero;
	  }
	  jstart=j+1;
	  break;
	}
    
    /* for the matched star info */
    k=1;
    for(i=1;i<=nimage;i++) {
      for(j=1;j<=nimage;j++) {
	
	if(flag_mag_mean[i][j]) {

	  if(flag_weightfit) {
	    weight_temp=1.0; 
	    A[k+nzp][i]+=1.0/weight_temp;
	    A[k+nzp][j]-=1.0/weight_temp;
	    b[k+nzp] += save_mag_mean[i][j]/weight_temp;
	  }
	  else {
	    A[k+nzp][i]+=1.0;
	    A[k+nzp][j]-=1.0;
	    b[k+nzp] += save_mag_mean[i][j];
	  }
	    
	  k++;
	}
      }
    }
    
    /* for the skybrite info */
    k=1;
    for(i=1;i<=nimage;i++) {
      for(j=1;j<=nimage;j++) {
	
	if(flag_skybrite[i][j]) {

	  if(flag_weightfit) {
	    weight_temp=1.0; 
	    A[k+nzp+nstarmatch][i]+=1.0/weight_temp;
	    A[k+nzp+nstarmatch][j]-=1.0/weight_temp;
	    b[k+nzp+nstarmatch] +=save_skybrite[i][j]/weight_temp;
	  }
	  else {
	    A[k+nzp+nstarmatch][i]+=1.0;
	    A[k+nzp+nstarmatch][j]-=1.0;
	    b[k+nzp+nstarmatch] +=save_skybrite[i][j];
	  }

	  k++;
	}
      }
    }
    
    //print_matrix(A,nrow,ncol);
    //print_vector(b,nrow);
  

    /*************************************************/
    /* solving matrix equation and print out results */
    /*************************************************/
    
    /* SVD solver here */
    
    svd_fit(A,b,berr,nrow,ncol,x,xerr);
    
    /* output result */

    sprintf(zpout,"%s_%s_ZP.dat",tilenamein,bandlist);
    fzpcompare=fopen(zpout,"w");
    sprintf(idout,"%s_%s_imageid.dat",tilenamein,bandlist);
    fidout=fopen(idout,"w");

    if(!flag_quiet) {
      if(flag_nostarmatch && flag_noskybrite) {
	printf("\n ** Input ZPs:\n");
	for(i=1;i<=nimage;i++) 
	  printf("%d\t%s/data/%s/%s/%s\t%2.4f\n",i,tileinfo[i].runiddesc,
		 tileinfo[i].nite,tileinfo[i].band,tileinfo[i].imagename,x[i]);
      }
      else {
	printf("\n ** Results from SVD:\n");
	fprintf(fzpcompare,"# i\tZP_dir\tZP_cal-ZP_dir\tError\n");
	for(i=1;i<=nimage;i++) {
	  printf("%d\t%s %s\t%2.4f +- %2.4f\tDirect_ZP=%2.4f\tDiff=%2.4f\n",i,tileinfo[i].runiddesc,
		 tileinfo[i].imagename,x[i],sqrt(xerr[i]),zp_latest[i].mag_zero,x[i]-zp_latest[i].mag_zero);
	  //printf("%d\t%s/data/%s/%s/%s\t%2.4f +- %2.4f\tDirect_ZP=%2.4f\tDiff=%2.4f\n",i,tileinfo[i].runiddesc,
	  //tileinfo[i].nite,tileinfo[i].band,tileinfo[i].imagename,x[i],sqrt(xerr[i]),zp_latest[i].mag_zero,x[i]-zp_latest[i].mag_zero);
	  fprintf(fzpcompare,"%d\t%2.4f\t%2.4f\t%2.4f\n",i,zp_latest[i].mag_zero,x[i]-zp_latest[i].mag_zero,sqrt(xerr[i]+Squ(zp_latest[i].sigma_zp)));
	}
      }
    }
    fclose(fzpcompare);
  } // for flag_quickcoadd
  else {
    x[1]=25.0;
    for(i=2;i<=nimage;i++)
      x[i]=0.0;
  }

  if(!flag_quiet) printf("\n");

  
  /*********************/
  /* coadd using swarp */
  /*********************/ 
  
  /* assign the zp for first image as base_mag */
  // or loopover to find non-zero zp etc 
  if(!flag_quickcoadd) {
    for(i=1;i<=nimage;i++) {
      if(x[i] > 25.0 && x[i] < 35.0) {
	mag_base = x[i];
	if(!flag_quiet)
	  printf(" ** Using image %d (%s) as reference image with ZP = %2.4f\n\n",i,tileinfo[i].imagename,x[i]);
	break;
      }
    }
  }

  /* set the output list */
  fswarpimg=fopen("image.list","w");
  fswarpvar=fopen("variance.list","w");
  
  /* output the swarp calls */
  sprintf(swarpscript,"%s_%s_swarp.cmd",tilenamein, bandlist);
  fswarp=fopen(swarpscript, "w");
  if(flag_binpath) fprintf(fswarp,"%s/swarp ",binpath);
  else fprintf(fswarp,"swarp ");
  fprintf(fswarp,"\@image.list ");
  if(flag_etcpath) fprintf(fswarp,"-c %s/default.swarp ",etcpath);
  else fprintf(fswarp,"-c default.swarp ");

  //if(!flag_quiet) printf(" ** Splitting images:\n");

  count=1;
  for(i=1;i<=nimage;i++) {

    /* calculat the flux-scale ratio */
    if(!flag_quickcoadd)
      fluxscale=pow(10.0,0.4*(mag_base-x[i]));
      //fluxscale=pow(10.0,-0.4*(mag_base-x[i]));
    else
      fluxscale=1.0;
    
    /* set the full path to the image */
    imgfullpath[0]=0;
    sprintf(imgfullpath, "%s/%s/data/%s/%s/%s",
	    basedir,tileinfo[i].runiddesc,tileinfo[i].nite,tileinfo[i].band,tileinfo[i].imagename);
    
    /* check the solution if the mag difference is within 1.5mag from the reference image */
    if(fabs(mag_base-x[i]) <= MAXMAGDIFF) {

      if(!flag_quiet) printf("%d\t%s\n",i,imgfullpath);

      /* no need to split the image in Version 1.1 if using swarp Version 2.16.5 or higher */
      /* split image */
      //if(flag_binpath) 
      //sprintf(command,"%s/splitimage %s -quiet -output image%02d > error", binpath,imgfullpath,count);
      //else
      //sprintf(command,"splitimage %s -quiet -output image%02d > error", imgfullpath,count);
      //system(command);

      //printf("\t\twith fluxscale = %2.4f (image%02d_im.fits)\n", fluxscale,count);

      /* check error message: check if the fits file exist or not after splitting */
      //ferr=fopen("error","r");
      //fscanf(ferr,"%s",err);
      //if (strcmp(err,"writing")) 
      //if(!flag_quiet) printf(" ** %s error: file %s not found\n",argv[0],imgfullpath);
      //fclose(ferr);
      //system ("rm error");

      /* check if the fits file exist or not */
      sprintf(command,"ls %s | wc",imgfullpath);
      pip=popen(command,"r");
      fscanf(pip,"%d",&check_exist);
      pclose(pip);
      if (!check_exist) {
	if(!flag_quiet) printf(" ** file %s not found\n",imgfullpath);
      }
      else {
	/* input the flux-scale ratio to FLXSCALE in header */
	//sprintf(image,"image%02d_im.fits",count);
	if(!flag_quiet)
	  printf("\tfluxscale = %2.4f for image %d\t(%s)\n", fluxscale,count,tileinfo[i].imagename);

	sprintf(image,"%s",imgfullpath);
	status=0;
	if(fits_open_file(&fptr,image,READWRITE,&status)) printerror(status);
	if(fits_update_key(fptr,TFLOAT,"FLXSCALE",&fluxscale,comment,&status)) printerror(status);
	if(fits_close_file(fptr,&status)) printerror(status);

	/* Swarp output */
	//if(count==1) {
	//fprintf(fswarp, "image%02d_im.fits",count);
	//sprintf(varimagearray,"image%02d_var.fits",count);
	//}
	//else {
	//fprintf(fswarp, " image%02d_im.fits",count);
	//sprintf(varimagearray,"%s,image%02d_var.fits",varimagearray,count);
	//}
	//fprintf(fswarp, ",image%02d_im.fits",count); /* due to latest changes in swarp ? */
	
	/* output to image.list and variance.list */
	fprintf(fswarpimg,"%s[0]\n",imgfullpath);
	fprintf(fswarpvar,"%s[1]\n",imgfullpath);
	fprintf(fidout,"%ld\t%2.4f\t%2.4f\n",tileinfo[i].imageid,x[i],xerr[i]);
	count++;
      }
    }
    else {
      if(!flag_quiet) {
	printf(" ** %s warning: following image is not used in coadd\n",argv[0]);
	printf("%d\t%s\n",i,imgfullpath);
	printf("\t\tbecause SVD solution=%2.3f (mag difference = %2.4f)\n", x[i],mag_base-x[i]);
      }
    }
  }

  if(!flag_quiet)
    printf("\n");
  
  /* assume all pixscale,tilera,tiledec etc are all the same at the moment */
  fprintf(fswarp," ");
  fprintf(fswarp,"-PIXELSCALE_TYPE MANUAL -PIXEL_SCALE %2.5f ",pixelsize);
  fprintf(fswarp,"-CENTER_TYPE MANUAL -CENTER %3.7f,%3.7f ",tile_ra,tile_dec);
  fprintf(fswarp,"-IMAGE_SIZE %d,%d ",npix_ra,npix_dec);
  fprintf(fswarp,"-SUBTRACT_BACK Y ");
  fprintf(fswarp,"-DELETE_TMPFILES Y ");
  fprintf(fswarp,"-RESAMPLE N ");
  fprintf(fswarp,"-WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE \@variance.list ");
  //fprintf(fswarp,"-WEIGHT_TYPE MAP_WEIGHT -WEIGHT_IMAGE %s ",varimagearray);
  fprintf(fswarp,"-COMBINE Y ");
  fprintf(fswarp,"-COMBINE_TYPE %s ", combinetype);
  if(flag_outfile) {
    fprintf(fswarp, "-IMAGEOUT_NAME %s ",outfile);
    /* for weight.fits */
    len=strlen(outfile);
    for (j=len;j>0;j--) {
      if (!strncmp(&(outfile[j]),".fits",5)) 
	outfile[j]=0;
    }
    fprintf(fswarp, "-WEIGHTOUT_NAME %s.weight.fits ",outfile);
  }
  fprintf(fswarp, "-HEADER_ONLY N ");
  fprintf(fswarp, "-WRITE_XML N ");
  if(flag_quiet)
    fprintf(fswarp,"-VERBOSE_TYPE QUIET ");
  fprintf(fswarp, "\n");
 
  fclose(fswarp);
  fclose(fswarpimg);
  fclose(fswarpvar);
  fclose(fidout);

  /* run the shell scripts */
  sprintf(command,"csh %s",swarpscript);
  system(command);

  /* insert the mag_base to image header */
  if(!flag_quickcoadd) {
    sprintf(image,"%s.fits",outfile);
    status=0;
    if(fits_open_file(&fptr,image,READWRITE,&status)) printerror(status);
    if(fits_update_key(fptr,TFLOAT,"SEXMGZPT",&mag_base,"Mag ZP",&status)) printerror(status);
    if(fits_close_file(fptr,&status)) printerror(status);
  }

  /* clean up */
  system("rm *sql");
  //system("rm *.cmd");
  //system("rm image.list variance.list");


  /********************/
  /* end of the code  */
  /********************/

  /* free memory allocation */
  free_imatrix(flag_mag_mean, 0, nimage, 0, nimage);
  free_imatrix(flag_skybrite, 0, nimage, 0, nimage);
  free_dmatrix(save_mag_mean, 0, nimage, 0, nimage);
  free_dmatrix(save_mag_rms, 0, nimage, 0, nimage);
  free_dmatrix(save_skybrite, 0, nimage, 0, nimage);
  free_dmatrix(save_skybriteerr, 0, nimage, 0, nimage);
  free_dmatrix(skybrite_ccdpair, 0, ccdtotal+1, 0, ccdtotal+1);
  free_dmatrix(skybriteerr_ccdpair, 0, ccdtotal+1, 0, ccdtotal+1);
  free_matrix(A, 0, nrow, 0, ncol);

  /* add the if statements for those arrays that really used? */
  free(tileinfo); free(tileinfo_in); free(zp); free(zp_latest);
  free(saveid1); free(saveid2); free(b); 
  free(berr); free(x); free(xerr); free(skybrite); 
  free(runidnites); free(bands); free(runids); 
  free(nites); free(tilerunid); free(keepimg);
  //free(nites_in); free(runid_in); 

  free(ra1); free(ra2); free(ra3); free(ra4);
  free(dec1); free(dec2); free(dec3); free(dec4);

  lsttime=time (NULL);
  if(!flag_quiet)
    printf("\n ** Done on %s \n",asctime(localtime (&lsttime)));
}

void print_dmatrix(double **matrix, int nrow, int ncol)
{
  int i,j;
  for(i=1; i<=nrow; i++){
      for(j=1; j<=ncol; j++)
	printf("%2.3f\t", matrix[i][j]);
      printf("\n");
    }
  printf("\n");
}

void print_imatrix(int **matrix, int nrow, int ncol)
{
  int i,j;
  for(i=1; i<=nrow; i++){
      for(j=1; j<=ncol; j++)
	printf("%d\t", matrix[i][j]);
      printf("\n");
    }
  printf("\n");
}

void print_matrix(float **matrix, int nrow, int ncol)
{
  int i,j;
  for(i=1; i<=nrow; i++){
      for(j=1; j<=ncol; j++)
	printf("%2.2f\t", matrix[i][j]);
      printf("\n");
    }
  printf("\n");
}

void print_vector(float *vector, int n)
{
  int i;
  for(i=1; i<=n; i++)
	printf("%2.5f\n", vector[i]);
  printf("\n");
}

void initialize_matrix(float **matrix, int nrow, int ncol)
{
  int i,j;
  for(i=0; i<=nrow; i++)
      for(j=0; j<=ncol; j++)
	matrix[i][j] = 0.0;
}

void initialize_dmatrix(double **matrix, int nrow, int ncol)
{
  int i,j;
  for(i=0; i<=nrow; i++)
      for(j=0; j<=ncol; j++)
	matrix[i][j] = 0.0;
}

void initialize_imatrix(int **matrix, int nrow, int ncol)
{
  int i,j;
  for(i=0; i<=nrow; i++)
      for(j=0; j<=ncol; j++)
	matrix[i][j] = 0;
}

void initialize_vector(float *vector, int n)
{
  int i;
  for(i=0; i<=n; i++)
    vector[i] = 0.0;
}

void initialize_dvector(double *vector, int n)
{
  int i;
  for(i=0; i<=n; i++)
    vector[i] = 0.0;
}

double getmean(double data[],double err[],int N,int flag)
{
  int i;
  double sum,mean,sigsq;

  sum=0.0;
  sigsq=0.0;
  for(i=0;i<N;i++) {
      sum+=data[i];
      sigsq+=1.0/(err[i]*err[i]);
  }

  if(flag==0)  /* unweighted average */  
    mean=sum/N;
  else {  /* weighted average */  
    sum = 0.0;
    for(i=0;i<N;i++)
      sum += data[i]/(err[i]*err[i]);
    mean = sum/sigsq;
  }

  return (mean);
}

double getrms(double data[],double err[],int N,int flag)
{
  int i;
  double var,s,ep,mean,rms;

  mean=getmean(data,err,N,flag);

  /* using two-pass formula as given in Numerical Recipes 14.1.8 */
  if(N>1) {
    var=ep=0.0;
    for(i=0;i<N;i++) { 
      s=data[i]-mean;
      ep+=s;
      var+=s*s;
    }
    rms=sqrt((var-ep*ep/N)/(double)(N-1));
  }
  else
    rms=0.0;

  return (rms);
}

double calc_ccdratio(double **skybrite_bookeep, double **skybrite_ccdpair, double **skybriteerr_ccdpair, int ccdtotal, int Nexposure)
{
  float median,medianerr,*vecsort;
  float sum,mean,std;
  unsigned long n;
  int i,j,k;
  void shell();

  n=(unsigned long)Nexposure;
  vecsort=(float *)calloc(Nexposure+1,sizeof(float));

  for(j=1;j<=ccdtotal;j++) {
    for(k=1;k<=ccdtotal;k++) {
      
      /* costruct the vector for ratio_jk=SKY_j/SKY_k (or ratio_jk=SKY_k/SKY_j ?) */
      sum=0.0;
      for(i=1;i<=Nexposure;i++) {
	vecsort[i]=skybrite_bookeep[j][i]/skybrite_bookeep[k][i];
	sum+=vecsort[i];
      }

      /* sort with N.R. subroutine */
      shell(n,vecsort);
    
      /* get median value */
      if( n%2)  median = vecsort[(n+1)/2];
      else median = 0.5 * (vecsort[n/2] + vecsort[(n/2)+1]);

      /* assign to skybrite_ccdpair[j][k] */
      //if(median==1.0)
      //median=0.0;
      //else
      //median=-2.5*log10(median);

      /* get medianerr value */
      mean=sum/(float)Nexposure;
      sum=0.0;
      for(i=1;i<=Nexposure;i++) 
	sum+=Squ(vecsort[i]-mean);
      std=sqrt(sum/(Nexposure-1.0));
      medianerr=1.253*std/sqrt((float)Nexposure);

      /* save results */
      skybrite_ccdpair[j][k]=(double)median;
      skybriteerr_ccdpair[j][k]=(double)medianerr;

    } // k-loop
  } // j-loop

  free(vecsort);
}

void iter_mean(double data[], double err[], int N, double *mean, double *rms, int *N_good, int *N_bad, int *N_iter, int flag, int flag_iter, int flag_Niterate, int Nmax_iterate, double THRESHOLD)
{
  int ii,Nt,Noutlier=0;
  double sum,t_mean,t_rms,new_mean,old_mean;
  double *olddata, *olderr;
  double *newdata, *newerr;
  
  *N_good=N;
  *N_bad=0;
  *N_iter=0;
  
  /* first find the sample average and  rms */
  t_mean=getmean(data,err,N,flag);
  t_rms=getrms(data,err,N,flag);

  /* memory allocation for the newdata and newerr */
  olddata=(double *)calloc(N+1,sizeof(double));
  olderr =(double *)calloc(N+1,sizeof(double));
  newdata=(double *)calloc(N+1,sizeof(double));
  newerr =(double *)calloc(N+1,sizeof(double));

  /* first find if any outliers when doing sigma-clipping */
  for(ii=0;ii<N;ii++) {
    if((fabs(data[ii]-t_mean) > THRESHOLD*t_rms))
      Noutlier++;
  }

  if(Noutlier==0 || flag_iter==0) { /* no outliers or not using sigma-clipping, simply return the results */
    *mean=t_mean;
    *rms=t_rms;
  }
  else { /* remove outliers and begin iterative process */

    /* initiallize the olddata array and parameters */
    for(ii=0;ii<N;ii++) {
      olddata[ii]=data[ii];
      olderr[ii]=err[ii];
    }
    old_mean=t_mean;
    new_mean=0.0;
    Nt = N;
  
    /* iterative procedure until the mean converge */
    while(fabs(old_mean-new_mean) >= ACCURACY) {

      /* get the mean and rms for the old data */
      t_mean=getmean(olddata,olderr,Nt,flag);
      t_rms =getrms(olddata,olderr,Nt,flag);
      old_mean=t_mean;

      /* find the number of outliers and put data in new array without the outlier */
      *N_good=0;
      for(ii=0;ii<Nt;ii++) {
	if((fabs(olddata[ii]-t_mean) > THRESHOLD*t_rms)) 
	  *N_bad+=1;
	else {
	  newdata[*N_good]=olddata[ii];
	  newerr[*N_good]=olderr[ii];
	  *N_good+=1;
	}
      }

      /* update information */
      if(Nt != (*N_good))
	*N_iter+=1;

      Nt = (*N_good);
      t_mean=getmean(newdata,newerr,Nt,flag);
      t_rms =getrms(newdata,newerr,Nt,flag);      
      new_mean = t_mean;

      /* put the newdata array back to olddata array */
      for(ii=0;ii<Nt;ii++) {
	olddata[ii]=newdata[ii];
	olderr[ii]=newerr[ii];
      }
     
      /* quick the loop if set the Nmax_iterate */
      if(flag_Niterate && *N_iter==Nmax_iterate) break;
    }

    if((*N_good)==0 || (*N_bad)==N) {
      t_mean=0.0;
      t_rms=0.0;
    }

    *mean=t_mean;
    *rms=t_rms;
  }

  /* free memory */
  free(olddata);free(olderr);
  free(newdata);free(newerr);
}

void svd_fit(float **A, float *b, float *berr, int nrow, int ncol, float *x, float *xerr)
{
  // need double precision version ?
  int i,j;
  float wmax,wmin,**u,*w,**v,**cvm;

  void svbksb(float **u, float w[], float **v, int m, int n, float b[], float x[]);
  void svdcmp(float **a, int m, int n, float w[], float **v);
  void svdvar(float **v, int ma, float w[], float **cvm);

  u = matrix(0, nrow, 0, ncol);
  v = matrix(0, ncol, 0, ncol);
  cvm = matrix(0, ncol, 0, ncol);
  w=(float *)calloc(ncol+1,sizeof(float));

  for(i=1;i<=nrow;i++)
    for(j=1;j<=ncol;j++)
      u[i][j]=A[i][j];
  
  svdcmp(u,nrow,ncol,w,v);

  wmax=0.0;
  for(j=1;j<=ncol;j++)
    if(w[j]>wmax) wmax=w[j];
  wmin=wmax*1.0e-6;
  for(j=1;j<=ncol;j++)
    if(w[j]<wmin) w[j]=0.0;

  svbksb(u,w,v,nrow,ncol,b,x);
  
  svdvar(v,ncol,w,cvm);

  for(i=1;i<=ncol;i++) 
    for(j=1;j<=ncol;j++) 
      if(i==j) 
	xerr[i]=cvm[i][j];
      
  free_matrix(u, 0, nrow, 0, ncol);
  free_matrix(v, 0, ncol, 0, ncol);
  free_matrix(cvm, 0, ncol, 0, ncol);
  free(w);
}


void helpmessage(int dummy)
{
  printf("WORKING ON THE HELP MESSAGE\n");
  printf("Required inputs for coadd_fluxscale (the order is not critical):\n");
  
  printf("  -project <project name>\n");
  printf("      Input the project name, such as DES, BCS or SCS\n");

  printf("  -tilename <tile string>\n");
  printf("      Input the name of the tile to be coadded, e.g. BCS0516-5223\n");

  printf("  -band <band>\n");
  printf("      Input the band for the tile to be coadded\n");

  printf("  -basedir <basedir>\n");
  printf("      Input the base-directory before runid/data/nite/band/... to built the absolute pathname to the remap images\n");
  printf("      On bcs.cosmology.uiuc.edu, it is /Archive/red\n");

  printf("  -detector <detector>\n");
  printf("      Input the detector, which should be either DECam or Mosaic2 for the CTIO Blanco 4m Telescope\n");
 
  printf("\n");
  printf("Optional inputs for coadd_fluxscale:\n");

  printf("  -binpath <binpath>\n");
  printf("      Setup the bin-path for select_remaps and swarp\n");

  printf("  -etcpath <etcpath>\n");
  printf("      Setup the etc-path for swarp configuration file\n");

  printf("  -nite <nite#1,nite#2,...>\n");
  printf("      Input nites for coadding with string of nite#1,nite#2,... (comma seperated). For single nite, just input nite#1\n");

  printf("  -runid <runid#1,runid#2,...>\n");
  printf("      Input runid for coadding with string of runid#1,runid#2,... (comma seperated). For single runid, just input runid#1\n");

  //printf("  -selectremap <filename>\n");
  //printf("      If select_remap is run outside the code, then input the the file output from select_remap. The select_remap has to run with -type REMAP -coadd -quiet option turn on\n");

  printf("  -class_star <lower_#> <upper_#>\n");
  printf("      Set the upper and lower values of class_star classification from SExtractor when doing star matching\n");

  printf("  -sigmaclip <#>\n");
  printf("      Set the threshold in sigma-clipping. For example, 2.5, is a common choice\n");

  printf("  -Niterate <#>\n");
  printf("      Set the maximum number of iteration in sigma-clipping\n");

  printf("  -Nstarmatch <#>\n");
  printf("      Set the minumum number of matched stars, image pairs with matched stars lower than this value will not be used in the matrix solution\n");

  printf("  -rmscut <#> \n");
  printf("      Set the maximum allowed RMS from the mean magnitudes in star matching, image pairs with RMS greater than this input value will be excluded\n");

  printf("  -flag <#>\n");
  printf("      Set the flag value from SExtractor when doing star matching\n");

  printf("  -radius <#>\n");
  printf("      Set the radius when doing star matching, in arcmin\n");

  printf("  -magtype <#>\n");
  printf("      Set the magnitude type when doing star matching, 0 is using mag_auto; 1 to 6 is using mag_aper_1 to mag_aper_6\n");

  printf("  -magcut <low_mag> <high_mag>\n");
  printf("      Set the upper and lower values of magnitude doing star matching\n");

  printf("  -fwhm <#>\n");
  printf("      Set the maximum value of FWHM for filtering out the images with FWHM greater than this value\n");

  printf("  -exptime <#>\n");
  printf("      Set the minimum value of EXPTIME for filtering out the images with EXPTIME smaller than this value\n");

  printf("  -weightmean\n");
  printf("      If calculating the weighted mean in the magnitude difference of matched stars\n");

  printf("  -weightfit \n");
  printf("      If using the wegith in matrix calculation\n");

  printf("  -weight \n");
  printf("      If using the weight calculation for both mean magnitude difference and matrix calculation\n");

  printf("  -combinetype <type>\n");
  printf("      Set the combinetype in swarp, which include: median,average,min,max,weighted,chi2,sum\n");

  printf("  -output <coadd filename>\n");
  printf("      Set the name of the coadded image\n");

  printf("  -nostarmatch\n");
  printf("      If not using star matching\n");

  printf("  -noskybrite\n");
  printf("      If not using SKYBRITE information\n");

  printf("  -nophotozp\n");
  printf("      If not using photometric ZP information\n");

  printf("  -quickcoadd\n");
  printf("      Running SWarp for coaddition without any fluxscale calculation\n");

  printf("  -help\n");
  printf("      Print this help message\n");

  printf("  -version\n");
  printf("      Print current version\n");

  printf("  -quiet\n");
  printf("      Run in quiet mode\n");

}


#undef ACCURACY
#undef TOLERANCE
#undef VERSION
#undef SVN_VERSION
#undef MAX
#undef MAXMAGDIFF
#undef MOSAIC
#undef DECAM
#undef LGE
