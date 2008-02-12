#define float double
float dgammq(a,x)
float a,x;
{
	void dgcf(),dgser();
	void nrerror();
	float gamser,gammcf,gln;

	if (x < 0.0 || a <= 0.0) nrerror("Invalid arguments in routine gammq");
	if (x < (a+1.0)) {
		dgser(&gamser,a,x,&gln);
		return 1.0-gamser;
	} else {
		dgcf(&gammcf,a,x,&gln);
		return gammcf;
	}
}
