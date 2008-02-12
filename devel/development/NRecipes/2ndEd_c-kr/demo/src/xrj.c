/* Driver for routine rj */

#include <stdio.h>
#include <string.h>
#include "nr.h"
#include "nrutil.h"

#define MAXSTR 80

main()
{
	char txt[MAXSTR];
	int i,nval;
	float p,val,x,y,z;
	FILE *fp;

	if ((fp = fopen("fncval.dat","r")) == NULL)
		nrerror("Data file fncval.dat not found\n");
	fgets(txt,MAXSTR,fp);
	while (strncmp(txt,"Elliptic Integral Third Kind RJ",31)) {
		fgets(txt,MAXSTR,fp);
		if (feof(fp)) nrerror("Data not found in fncval.dat\n");
	}
	fscanf(fp,"%d %*s",&nval);
	printf("\n%s\n",txt);
	printf("%7s %8s %8s %8s %16s %20s\n","x","y","z","p","actual","rj(x,y,z,p)");
	for (i=1;i<=nval;i++) {
		fscanf(fp,"%f %f %f %f %f",&x,&y,&z,&p,&val);
		printf("%8.2f %8.2f %8.2f %8.2f %18.6e %18.6e\n",x,y,z,p,val,rj(x,y,z,p));
	}
	fclose(fp);
	return 0;
}
