void eclass(nf,n,lista,listb,m)
int lista[],listb[],m,n,nf[];
{
	int l,k,j;

	for (k=1;k<=n;k++) nf[k]=k;
	for (l=1;l<=m;l++) {
		j=lista[l];
		while (nf[j] != j) j=nf[j];
		k=listb[l];
		while (nf[k] != k) k=nf[k];
		if (j != k) nf[j]=k;
	}
	for (j=1;j<=n;j++)
		while (nf[j] != nf[nf[j]]) nf[j]=nf[nf[j]];
}
