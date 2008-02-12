#include "imageproc.h"


main(argc,argv)
	int argc;
	char *argv[];
{
	char	filename[1000],runiddesc[500],imagename[1000],imagetype[100],
		band[100],tilename[100],nite[100],imageclass[100];
	int	ccd_number;
	void 	filename_resolve();


	if (argc<2) {
	  printf("testfilename <filename>\n");
	  exit(0);
	}
	sprintf(filename,argv[1]);
	
	filename_resolve(filename,imageclass,runiddesc,nite,tilename,
	  imagetype,imagename,band,&ccd_number);
	  
	printf("%s:\n",filename);
	printf("  imclass: %-6s runid: %-15s nite: %-15s imtype: %-8s\n",
	  imageclass,runiddesc,nite,imagetype);
	printf("  tilename: %-15s imname: %-20s band: %-6s ccdnum: %2d\n",
	  tilename,imagename,band,ccd_number);
	
}

