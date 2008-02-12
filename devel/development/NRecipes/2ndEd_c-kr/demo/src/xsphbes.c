/* Driver for routine rj */

#include <stdio.h>
#include <string.h>
#include "nr.h"
#include "nrutil.h"

#define MAXSTR 80

main()
{
	char txt[MAXSTR];
	int i,n,nval;
	float sj,sy,sjp,syp,x,xsj,xsy,xsjp,xsyp;
	FILE *fp;

	if ((fp = fopen("fncval.dat","r")) == NULL)
		nrerror("Data file fncval.dat not found\n");
	fgets(txt,MAXSTR,fp);
	while (strncmp(txt,"Spherical Bessel Functions",26)) {
		fgets(txt,MAXSTR,fp);
		if (feof(fp)) nrerror("Data not found in fncval.dat\n");
	}
	fscanf(fp,"%d %*s",&nval);
	printf("\n%s\n",txt);
	for (i=1;i<=nval;i++) {
		fscanf(fp,"%d %f %f %f %f %f",&n,&x,&sj,&sy,&sjp,&syp);
		printf("%5s %4s\n%14s %16s %17s %16s\n%14s %16s %17s %16s\n",
			"n","x","sj","sy","sjp","syp","xsj","xsy","xsjp","xsyp");
		sphbes(n,x,&xsj,&xsy,&xsjp,&xsyp);
		printf("%5d %5.2f\n\t%16.6e %16.6e %16.6e %16.6e\n",n,x,sj,sy,sjp,syp);
		printf("\t%16.6e %16.6e %16.6e %16.6e\n",xsj,xsy,xsjp,xsyp);
	}
	fclose(fp);
	return 0;
}
