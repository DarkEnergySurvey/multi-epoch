#include "nrutil.h"

void sort3(n,ra,rb,rc)
float ra[],rb[],rc[];
unsigned long n;
{
	void indexx();
	unsigned long j,*iwksp;
	float *wksp;

	iwksp=lvector(1,n);
	wksp=vector(1,n);
	indexx(n,ra,iwksp);
	for (j=1;j<=n;j++) wksp[j]=ra[j];
	for (j=1;j<=n;j++) ra[j]=wksp[iwksp[j]];
	for (j=1;j<=n;j++) wksp[j]=rb[j];
	for (j=1;j<=n;j++) rb[j]=wksp[iwksp[j]];
	for (j=1;j<=n;j++) wksp[j]=rc[j];
	for (j=1;j<=n;j++) rc[j]=wksp[iwksp[j]];
	free_vector(wksp,1,n);
	free_lvector(iwksp,1,n);
}
