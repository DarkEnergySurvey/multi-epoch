#include "nrutil.h"

void lfit(x,y,sig,ndat,a,ia,ma,covar,chisq,funcs)
float **covar,*chisq,a[],sig[],x[],y[];
int ia[],ma,ndat;
void (*funcs)();
{
	void covsrt(),gaussj();
	int i,j,k,l,m,mfit=0;
	float ym,wt,sum,sig2i,**beta,*afunc;

	beta=matrix(1,ma,1,1);
	afunc=vector(1,ma);
	for (j=1;j<=ma;j++)
		if (ia[j]) mfit++;
	if (mfit == 0) nrerror("lfit: no parameters to be fitted");
	for (j=1;j<=mfit;j++) {
		for (k=1;k<=mfit;k++) covar[j][k]=0.0;
		beta[j][1]=0.0;
	}
	for (i=1;i<=ndat;i++) {
		(*funcs)(x[i],afunc,ma);
		ym=y[i];
		if (mfit < ma) {
			for (j=1;j<=ma;j++)
				if (!ia[j]) ym -= a[j]*afunc[j];
		}
		sig2i=1.0/SQR(sig[i]);
		for (j=0,l=1;l<=ma;l++) {
			if (ia[l]) {
				wt=afunc[l]*sig2i;
				for (j++,k=0,m=1;m<=l;m++)
					if (ia[m]) covar[j][++k] += wt*afunc[m];
				beta[j][1] += ym*wt;
			}
		}
	}
	for (j=2;j<=mfit;j++)
		for (k=1;k<j;k++)
			covar[k][j]=covar[j][k];
	gaussj(covar,mfit,beta,1);
	for (j=0,l=1;l<=ma;l++)
		if (ia[l]) a[l]=beta[++j][1];
	*chisq=0.0;
	for (i=1;i<=ndat;i++) {
		(*funcs)(x[i],afunc,ma);
		for (sum=0.0,j=1;j<=ma;j++) sum += a[j]*afunc[j];
		*chisq += SQR((y[i]-sum)/sig[i]);
	}
	covsrt(covar,ma,ia,mfit);
	free_vector(afunc,1,ma);
	free_matrix(beta,1,ma,1,1);
}
