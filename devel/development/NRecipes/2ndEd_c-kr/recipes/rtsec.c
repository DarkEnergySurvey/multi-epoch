#include <math.h>
#define MAXIT 30

float rtsec(func,x1,x2,xacc)
float (*func)(),x1,x2,xacc;
{
	void nrerror();
	int j;
	float fl,f,dx,swap,xl,rts;

	fl=(*func)(x1);
	f=(*func)(x2);
	if (fabs(fl) < fabs(f)) {
		rts=x1;
		xl=x2;
		swap=fl;
		fl=f;
		f=swap;
	} else {
		xl=x1;
		rts=x2;
	}
	for (j=1;j<=MAXIT;j++) {
		dx=(xl-rts)*f/(f-fl);
		xl=rts;
		fl=f;
		rts += dx;
		f=(*func)(rts);
		if (fabs(dx) < xacc || f == 0.0) return rts;
	}
	nrerror("Maximum number of iterations exceeded in rtsec");
	return 0.0;
}
#undef MAXIT
