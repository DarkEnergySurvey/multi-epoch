#include <stdio.h>
#include <math.h>
#include "nrutil.h"
#define PI 3.14159265
#define N 40

main()  /* Program fredex */
{
	void lubksb(),ludcmp(),quadmx();
	float **a,d,*g,x;
	int *indx,j;

	indx=ivector(1,N);
	a=matrix(1,N,1,N);
	g=vector(1,N);
	quadmx(a,N);
	ludcmp(a,N,indx,&d);
	for (j=1;j<=N;j++) g[j]=sin((j-1)*PI/(N-1));
	lubksb(a,N,indx,g);
	for (j=1;j<=N;j++) {
		x=(j-1)*PI/(N-1);
		printf("%6.2d %12.6f %12.6f\n",j,x,g[j]);
	}
	free_vector(g,1,N);
	free_matrix(a,1,N,1,N);
	free_ivector(indx,1,N);
	return 0;
}
#undef N
#undef PI
