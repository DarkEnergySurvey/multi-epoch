/* Driver for routine scrsho */

#include <stdio.h>
#include "nr.h"

static float fx(x)
float x;
{
	return bessj0(x);
}

main()
{
	scrsho(fx);
	return 0;
}
