/* Driver for routine gammln */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nr.h"
#include "nrutil.h"

#define MAXSTR 80

main()
{
	char txt[MAXSTR];
	int i,nval;
	float actual,calc,x;
	FILE *fp;

	if ((fp = fopen("fncval.dat","r")) == NULL)
		nrerror("Data file fncval.dat not found\n");
	fgets(txt,MAXSTR,fp);
	while (strncmp(txt,"Gamma Function",14)) {
		fgets(txt,MAXSTR,fp);
		if (feof(fp)) nrerror("Data not found in fncval.dat\n");
	}
	fscanf(fp,"%d %*s",&nval);
	printf("\n%s\n",txt);
	printf("%10s %21s %21s\n","x","actual","gammln(x)");
	for (i=1;i<=nval;i++) {
		fscanf(fp,"%f %f",&x,&actual);
		if (x > 0.0) {
			calc=(x<1.0 ? gammln(x+1.0)-log(x) : gammln(x));
			printf("%12.2f %20.6f %20.6f\n",x,
				log(actual),calc);
		}
	}
	fclose(fp);
	return 0;
}
