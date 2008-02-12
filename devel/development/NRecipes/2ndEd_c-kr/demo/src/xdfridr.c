/* Driver for routine dfridr */

#include <stdio.h>
#include <math.h>
#include "nr.h"
#include "nrutil.h"

float func(x)
float x;
{
	return tan(x);
}

main()
{
	float x,h,dx,err;

	printf("input x, h\n");
	while (scanf("%f %f",&x,&h) != EOF) {
		dx=dfridr(func,x,h,&err);
		printf("dfridr=%12.6f %12.6f %12.6f\n",dx,1.0/SQR(cos(x)),err);
	}

	return 0;
}
