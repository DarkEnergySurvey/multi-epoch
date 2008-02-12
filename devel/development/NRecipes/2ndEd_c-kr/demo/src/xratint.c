/* Driver for routine ratint */

#include <stdio.h>
#include <math.h>
#include "nr.h"
#include "nrutil.h"

#define NPT 6
#define EPS 1.0

float f(x,eps)
float eps,x;
{
	return x*exp(-x)/(SQR(x-1.0)+eps*eps);
}

main()
{
	int i;
	float dyy,xx,yexp,yy,*x,*y;

	x=vector(1,NPT);
	y=vector(1,NPT);
	for (i=1;i<=NPT;i++) {
		x[i]=i*2.0/NPT;
		y[i]=f(x[i],EPS);
	}
	printf("\nDiagonal rational function interpolation\n");
	printf("\n%5s %13s %14s %12s\n","x","interp.","accuracy","actual");
	for (i=1;i<=10;i++) {
		xx=0.2*i;
		ratint(x,y,NPT,xx,&yy,&dyy);
		yexp=f(xx,EPS);
		printf("%6.2f %12.6f    %11f %13.6f\n",xx,yy,dyy,yexp);
	}
	free_vector(y,1,NPT);
	free_vector(x,1,NPT);
	return 0;
}
