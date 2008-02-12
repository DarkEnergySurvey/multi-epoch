#include "nrutil.h"

void simpr(y,dydx,dfdx,dfdy,n,xs,htot,nstep,yout,derivs)
float **dfdy,dfdx[],dydx[],htot,xs,y[],yout[];
int n,nstep;
void (*derivs)();
{
	void lubksb(),ludcmp();
	int i,j,nn,*indx;
	float d,h,x,**a,*del,*ytemp;

	indx=ivector(1,n);
	a=matrix(1,n,1,n);
	del=vector(1,n);
	ytemp=vector(1,n);
	h=htot/nstep;
	for (i=1;i<=n;i++) {
		for (j=1;j<=n;j++) a[i][j] = -h*dfdy[i][j];
		++a[i][i];
	}
	ludcmp(a,n,indx,&d);
	for (i=1;i<=n;i++)
		yout[i]=h*(dydx[i]+h*dfdx[i]);
	lubksb(a,n,indx,yout);
	for (i=1;i<=n;i++)
		ytemp[i]=y[i]+(del[i]=yout[i]);
	x=xs+h;
	(*derivs)(x,ytemp,yout);
	for (nn=2;nn<=nstep;nn++) {
		for (i=1;i<=n;i++)
			yout[i]=h*yout[i]-del[i];
		lubksb(a,n,indx,yout);
		for (i=1;i<=n;i++)
			ytemp[i] += (del[i] += 2.0*yout[i]);
		x += h;
		(*derivs)(x,ytemp,yout);
	}
	for (i=1;i<=n;i++)
		yout[i]=h*yout[i]-del[i];
	lubksb(a,n,indx,yout);
	for (i=1;i<=n;i++)
		yout[i] += ytemp[i];
	free_vector(ytemp,1,n);
	free_vector(del,1,n);
	free_matrix(a,1,n,1,n);
	free_ivector(indx,1,n);
}
