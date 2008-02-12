/* Driver for routine gammq */

#include <stdio.h>
#include <string.h>
#include "nr.h"
#include "nrutil.h"

#define MAXSTR 80

main()
{
	char txt[MAXSTR];
	int i,nval;
	float a,val,x;
	FILE *fp;

	if ((fp = fopen("fncval.dat","r")) == NULL)
		nrerror("Data file fncval.dat not found\n");
	fgets(txt,MAXSTR,fp);
	while (strncmp(txt,"Incomplete Gamma Function",25)) {
		fgets(txt,MAXSTR,fp);
		if (feof(fp)) nrerror("Data not found in fncval.dat\n");
	}
	fscanf(fp,"%d %*s",&nval);
	printf("\n%s\n",txt);
	printf("%4s %11s %14s %14s \n","a","x","actual","gammq(a,x)");
	for (i=1;i<=nval;i++) {
		fscanf(fp,"%f %f %f",&a,&x,&val);
		printf("%6.2f %12.6f %12.6f %12.6f\n",a,x,(1.0-val),gammq(a,x));
	}
	fclose(fp);
	return 0;
}
