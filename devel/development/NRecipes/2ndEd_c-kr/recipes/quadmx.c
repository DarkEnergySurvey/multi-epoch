#include <math.h>
#include "nrutil.h"
#define PI 3.14159265

double x;

void quadmx(a,n)
float **a;
int n;
{
	void kermom(),wwghts();
	int j,k;
	float h,*wt,xx,cx;

	wt=vector(1,n);
	h=PI/(n-1);
	for (j=1;j<=n;j++) {
		x=xx=(j-1)*h;
		wwghts(wt,n,h,kermom);
		cx=cos(xx);
		for (k=1;k<=n;k++) a[j][k]=wt[k]*cx*cos((k-1)*h);
		++a[j][j];
	}
	free_vector(wt,1,n);
}
#undef PI
