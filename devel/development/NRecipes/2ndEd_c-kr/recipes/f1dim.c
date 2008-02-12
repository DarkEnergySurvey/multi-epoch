#include "nrutil.h"

extern int ncom;
extern float *pcom,*xicom,(*nrfunc)();

float f1dim(x)
float x;
{
	int j;
	float f,*xt;

	xt=vector(1,ncom);
	for (j=1;j<=ncom;j++) xt[j]=pcom[j]+x*xicom[j];
	f=(*nrfunc)(xt);
	free_vector(xt,1,ncom);
	return f;
}
