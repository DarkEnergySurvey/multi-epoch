#include "imageproc.h"

#define TOL 10e-5
#define MAXITER 2000
#define SIGSTART 0.05
#define MAGDIFFCUT 0.2

void calc_wmean(int N, float data[], float err[], float *wmean, float *wmeanerr);
void calc_intsigma(int N, float data[], float err[], float wmean, float *insigma);

main(argc,argv)
     int argc;
     char *argv[];
{
  char infile[1000];
  float tm,tmag,tmagdiff,tmagerr;
  float wmean,wmeanerr,insigma;
  float bin,binstart;
  float *mag,*magdiff,*magerr;
  int i,count,N;
  unsigned long nn;
  void sort3();
  FILE *fin;

  bin=1.0;
  binstart=12.5;

  if ( argc < 2 ) {
    printf ("Usage: %s <input datafile (format: mag delta_mag err_mag)>\n");
    exit(0);
  }
  
  sprintf(infile,"%s",argv[1]);
  
  /* count the datafile */
  count=0;
  fin=fopen(infile,"r");
  while(fscanf(fin,"%f %f %f",&tm,&tm,&tm)!=EOF) 
    count++;
  fclose(fin);
  N=count;
  
  nn=N;

  /* memory allocation */
  mag=(float *)calloc(N+1,sizeof(float));
  magdiff=(float *)calloc(N+1,sizeof(float));
  magerr=(float *)calloc(N+1,sizeof(float));

  /* input data and calculate weighted mean */
  fin=fopen(infile,"r");
  count=0;
  for(i=1;i<=N;i++) {
    fscanf(fin,"%f %f %f",&tmag,&tmagdiff,&tmagerr);
    if(fabs(tmagdiff)< MAGDIFFCUT) {
      count++;
      mag[count]=tmag;
      magdiff[count]=tmagdiff;
      magerr[count]=tmagerr;
    }
  }
  fclose(fin);

  N=count;

  calc_wmean(N,magdiff,magerr,&wmean,&wmeanerr);
  calc_intsigma(N,magdiff,magerr,wmean,&insigma);
  
  printf(" -- weighted mean = %2.5f +- %2.5f for %d data points\n",wmean,wmeanerr,N);
  printf(" -- intrinsic dispersion = %2.4f\n",insigma);


  /* sort the data */
  sort3(nn,mag,magdiff,magerr);


  /* free memory */
  free(mag); free(magdiff); free(magerr);
}


void calc_wmean(int N, float data[], float err[], float *wmean, float *wmeanerr)
{
  int i;
  double sum=0.0;
  double sigsq=0.0;

  for(i=1;i<=N;i++) {
    sigsq+=1.0/Squ(err[i]);
    sum+=data[i]/Squ(err[i]);
  }

  *wmean=(float)(sum/sigsq);
  *wmeanerr=(float)sqrt(1.0/sigsq);
}

void calc_intsigma(int N, float data[], float err[], float wmean, float *insigma)
{
  int i,j;
  double chi2;
  float sigtem;
  
  /* initial guess of the intrinsic sigma */
  sigtem=SIGSTART;
  chi2=0.0;
  for(i=1;i<=N;i++) 
    chi2+=Squ(data[i]-wmean)/(Squ(err[i])+Squ(sigtem));

  chi2=chi2/(float)N;

  j=0;
  while(fabs(chi2-1.0)>TOL) {
    if(chi2>1.0) sigtem+=fabs(chi2-1.0)*0.001;
    if(chi2<1.0) sigtem-=fabs(chi2-1.0)*0.001;

    chi2=0.0;
    for(i=1;i<=N;i++) 
      chi2+=Squ(data[i]-wmean)/(Squ(err[i])+Squ(sigtem));
    chi2=chi2/(float)N;

    j++;
    if(j>MAXITER) {
      printf("Iteration exceed %d, abort!\n",MAXITER);
      exit(0);
    }
    //printf("  Iteration %d:  chi^2=%.4f sigma_int=%.4f\n",j,chi2,sigtem); 
 }

  *insigma=sigtem;
}

#undef TOL
#undef MAXITER
#undef SIGSTART
#undef MAGDIFFCUT
