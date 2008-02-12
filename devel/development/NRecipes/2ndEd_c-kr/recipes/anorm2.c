#include <math.h>

double anorm2(a,n)
double **a;
int n;
{
	int i,j;
	double sum=0.0;

	for (j=1;j<=n;j++)
		for (i=1;i<=n;i++)
			sum += a[i][j]*a[i][j];
	return sqrt(sum)/n;
}
