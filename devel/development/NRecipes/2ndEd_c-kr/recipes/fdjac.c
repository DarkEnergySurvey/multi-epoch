#include <math.h>
#include "nrutil.h"
#define EPS 1.0e-4

void fdjac(n,x,fvec,df,vecfunc)
float **df,fvec[],x[];
int n;
void (*vecfunc)();
{
	int i,j;
	float h,temp,*f;

	f=vector(1,n);
	for (j=1;j<=n;j++) {
		temp=x[j];
		h=EPS*fabs(temp);
		if (h == 0.0) h=EPS;
		x[j]=temp+h;
		h=x[j]-temp;
		(*vecfunc)(n,x,f);
		x[j]=temp;
		for (i=1;i<=n;i++) df[i][j]=(f[i]-fvec[i])/h;
	}
	free_vector(f,1,n);
}
#undef EPS
