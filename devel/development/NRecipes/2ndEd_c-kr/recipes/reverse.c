void reverse(iorder,ncity,n)
int iorder[],n[],ncity;
{
	int nn,j,k,l,itmp;

	nn=(1+((n[2]-n[1]+ncity) % ncity))/2;
	for (j=1;j<=nn;j++) {
		k=1 + ((n[1]+j-2) % ncity);
		l=1 + ((n[2]-j+ncity) % ncity);
		itmp=iorder[k];
		iorder[k]=iorder[l];
		iorder[l]=itmp;
	}
}
