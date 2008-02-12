/* image reading and writing subroutines */

/* currently assumes image data in first extension and doesn't bother */
/* to read other extensions */

#include "imageproc.h"


void printerror(status)
        int status;
{
        fits_report_error(stderr,status);
        exit(0);
}

