#define NUSE1 5
#define NUSE2 5
#define float double

void dbeschb(x,gam1,gam2,gampl,gammi)
double *gam1,*gam2,*gammi,*gampl,x;
{
	float dchebev();
	float xx;
	static float c1[] = {
		-1.142022680371172e0,6.516511267076e-3,
		3.08709017308e-4,-3.470626964e-6,6.943764e-9,
		3.6780e-11,-1.36e-13};
	static float c2[] = {
		1.843740587300906e0,-0.076852840844786e0,
		1.271927136655e-3,-4.971736704e-6,-3.3126120e-8,
		2.42310e-10,-1.70e-13,-1.0e-15};

	xx=8.0*x*x-1.0;
	*gam1=dchebev(-1.0,1.0,c1,NUSE1,xx);
	*gam2=dchebev(-1.0,1.0,c2,NUSE2,xx);
	*gampl= *gam2-x*(*gam1);
	*gammi= *gam2+x*(*gam1);
}
#undef NUSE1
#undef NUSE2
