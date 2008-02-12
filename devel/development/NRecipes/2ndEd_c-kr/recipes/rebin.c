void rebin(rc,nd,r,xin,xi)
float r[],rc,xi[],xin[];
int nd;
{
	int i,k=0;
	float dr=0.0,xn=0.0,xo;

	for (i=1;i<nd;i++) {
		while (rc > dr) {
			dr += r[++k];
			xo=xn;
			xn=xi[k];
		}
		dr -= rc;
		xin[i]=xn-(xn-xo)*dr/r[k];
	}
	for (i=1;i<nd;i++) xi[i]=xin[i];
	xi[nd]=1.0;
}
