/* Driver for routine newt */

#include <stdio.h>
#include <math.h>
#include "nr.h"
#include "nrutil.h"

void funcv(n,x,f)
float f[],x[];
int n;
{
	f[1]=SQR(x[1])+SQR(x[2])-2.0;
	f[2]=exp(x[1]-1.0)+x[2]*SQR(x[2])-2.0;
}

#define N 2

main()
{
	int i,check;
	float *x,*f;

	x=vector(1,N);
	f=vector(1,N);
	x[1]=2.0;
	x[2]=0.5;
	newt(x,N,&check,funcv);
	funcv(N,x,f);
	if (check) printf("Convergence problems.\n");
	printf("%7s %3s %12s\n","Index","x","f");
	for (i=1;i<=N;i++) printf("%5d %12.6f %12.6f\n",i,x[i],f[i]);
	free_vector(f,1,N);
	free_vector(x,1,N);
	return 0;
}
