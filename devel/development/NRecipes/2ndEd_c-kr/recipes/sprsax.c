void sprsax(sa,ija,x,b,n)
float b[],sa[],x[];
unsigned long ija[],n;
{
	void nrerror();
	unsigned long i,k;

	if (ija[1] != n+2) nrerror("sprsax: mismatched vector and matrix");
	for (i=1;i<=n;i++) {
		b[i]=sa[i]*x[i];
		for (k=ija[i];k<=ija[i+1]-1;k++)
			b[i] += sa[k]*x[ija[k]];

		}
}
