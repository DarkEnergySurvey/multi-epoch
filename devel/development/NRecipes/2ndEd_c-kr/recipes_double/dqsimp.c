#include <math.h>
#define EPS 1.0e-6
#define JMAX 20
#define float double

float dqsimp(func,a,b)
float (*func)(),a,b;
{
	float dtrapzd2();
	void nrerror();
	int j;
	float s,st,ost,os;

	ost = os = -1.0e30;
	for (j=1;j<=JMAX;j++) {
		st=dtrapzd2(func,a,b,j);
		s=(4.0*st-ost)/3.0;
		if (fabs(s-os) < EPS*fabs(os)) return s;
		os=s;
		ost=st;
	}
	nrerror("Too many steps in routine dqsimp");
	return 0.0;
}
#undef EPS
#undef JMAX
