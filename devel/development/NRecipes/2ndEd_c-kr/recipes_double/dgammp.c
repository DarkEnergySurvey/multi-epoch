#define float double
float dgammp(a,x)
float a,x;
{
	void dgcf(),dgser();
	void nrerror();
	float gamser,gammcf,gln;

	if (x < 0.0 || a <= 0.0) nrerror("Invalid arguments in routine gammp");
	if (x < (a+1.0)) {
		dgser(&gamser,a,x,&gln);
		return gamser;
	} else {
		dgcf(&gammcf,a,x,&gln);
		return 1.0-gammcf;
	}
}
